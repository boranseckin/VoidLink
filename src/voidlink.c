#include <stdio.h>
#include <string.h>

#include "hardware/adc.h"
#include "hardware/irq.h"
#include "hardware/regs/intctrl.h"
#include "hardware/timer.h"
#include "hardware/uart.h"
#include "pico/multicore.h"
#include "pico/rand.h"
#include "pico/time.h"
#include "pico/types.h"
#include "pico/util/queue.h"

#include "class/cdc/cdc_device.h"
#include "pico/stdio_usb.h"
#include "tusb.h"

#include "pico_config.h"
#include "sx126x.h"
#include "sx126x_hal_context.h"

#include "console.h"
#include "io.h"
#include "network.h"
#include "screen.h"
#include "utils.h"
#include "voidlink.h"

absolute_time_t last_tx_start;
absolute_time_t last_tx_delta;

// Debug flag to stop processing of received messages.
bool STOP_PROCESSING = false;

// Main state machine.
typedef enum {
  STATE_IDLE,
  STATE_TX,
  STATE_TX_DONE,
  STATE_RX,
} state_t;

// Current state of the main state machine.
static state_t state = STATE_IDLE;

typedef enum {
  CONSOLE_IDLE,
  CONSOLE_READY,
} console_t;

static console_t console = CONSOLE_IDLE;

static sx126x_hal_context_t context;
static sx126x_mod_params_lora_t mod_params;
static sx126x_pkt_params_lora_t packet_params;
static mod_params_t mod_params_local;

void set_range(mod_params_t param) {
  switch (param) {
  case DEFAULT:
    mod_params = MOD_PARAMS_DEFAULT;
    mod_params_local = DEFAULT;
    break;
  case FAST:
    mod_params = MOD_PARAMS_FAST;
    mod_params_local = FAST;
    break;
  case LONGRANGE:
    mod_params = MOD_PARAMS_LONGRANGE;
    mod_params_local = LONGRANGE;
    break;
  }

  sx126x_set_lora_mod_params(&context, &mod_params);

  debug("modulation parameters set to %s (ToA: %u ms)\n", MOD_PARAM_STR[param],
        get_time_on_air_in_ms());
}

uint32_t get_time_on_air_in_ms() {
  return sx126x_get_lora_time_on_air_in_ms(&packet_params, &mod_params);
}

void handle_tx_callback() {
  last_tx_delta = absolute_time_diff_us(last_tx_start, get_absolute_time());
  debug("last tx took %llu us\n", last_tx_delta);

  sx126x_chip_status_t status = {.chip_mode = 0, .cmd_status = 0};
  sx126x_get_status(&context, &status);
  if (status.cmd_status != SX126X_CMD_STATUS_CMD_TX_DONE) {
    error("tx callback status error (mode: %d | cmd: %d)\n", status.chip_mode, status.cmd_status);
    return;
  }

  if (state != STATE_TX) {
    error("TX IRQ triggered while not in TX state\n");
  }
  state = STATE_TX_DONE;
  debug("STATE = TX_DONE\n");
}

void handle_rx_callback() {
  absolute_time_t rx_time = get_absolute_time();

  // Make sure the data available status is set before reading the rx buffer.
  sx126x_chip_status_t status = {.chip_mode = 0, .cmd_status = 0};
  sx126x_get_status(&context, &status);
  if (status.cmd_status != SX126X_CMD_STATUS_DATA_AVAILABLE) {
    error("rx status error (mode: %d | cmd: %d)\n", status.chip_mode, status.cmd_status);
    return;
  }

  // Get length and the start address of the received message.
  sx126x_rx_buffer_status_t buffer_status = {.buffer_start_pointer = 0, .pld_len_in_bytes = 0};
  sx126x_get_rx_buffer_status(&context, &buffer_status);
  debug("payload received: %d @ %d\n", buffer_status.pld_len_in_bytes,
        buffer_status.buffer_start_pointer);

  // Make sure the buffer has enough space to read the message.
  if (buffer_status.pld_len_in_bytes > sizeof(message_t)) {
    error("payload is bigger than the buffer (%d)\n", sizeof(message_t));
    return;
  }

  // Read and print the buffer.
  message_history_t rx_payload_buf = {.time = rx_time};
  sx126x_read_buffer(&context, buffer_status.buffer_start_pointer,
                     (uint8_t *)&rx_payload_buf.message, buffer_status.pld_len_in_bytes);

  debug("<-");
  for (int i = 0; i < buffer_status.pld_len_in_bytes; i++) {
    debug(" %02x", ((uint8_t *)&rx_payload_buf)[i]);
  }
  debug("\n");

  if (is_my_uid(rx_payload_buf.message.src)) {
    debug("message from myself\n");
    return;
  }

  // Get the packet status to learn the signal strength of the received message.
  sx126x_pkt_status_lora_t pkt_status = {0};
  sx126x_get_lora_pkt_status(&context, &pkt_status);

  // Update the neighbour table with the information from received message.
  // TODO: ignore rssi for hopped messages
  update_neighbour(rx_payload_buf.message.src, pkt_status.signal_rssi_pkt_in_dbm, 0);

  // Check if the received message is for us.
  if (!is_my_uid(rx_payload_buf.message.dst) && !is_broadcast(rx_payload_buf.message.dst)) {
    debug("message is not for me\n");

    // Check if the message has remaining hops.
    if (rx_payload_buf.message.flags.hop_limit > 0) {
      rx_payload_buf.message.flags.hop_limit--;
      debug("forwarding message (%d hops remaining)\n", rx_payload_buf.message.flags.hop_limit);
      if (queue_try_add(&tx_queue, &rx_payload_buf)) {
        debug("tx enqueue %d\n", rx_payload_buf.message.id);
      } else {
        error("tx queue is full\n");
      }
    } else {
      debug("not forwarding message, hop limit reached\n");
    }

    return;
  }

  // Add message to the receive queue.
  if (queue_try_add(&rx_queue, &rx_payload_buf)) {
    debug("rx enqueue %d\n", rx_payload_buf.message.id);
  } else {
    // TODO: maybe drop the oldest message instead
    error("rx queue is full, dropping message\n");
  }
}

// Callback function for everytime an interrupt is detected on DIO1.
void handle_dio1_callback(uint gpio, uint32_t events) {
  // Get the irq status to learn why the interrupt was triggered.
  // And clean the interrupt at the same time, since it is getting handled now.
  sx126x_irq_mask_t irq = 0;
  sx126x_get_and_clear_irq_status(&context, &irq);

  debug("%s\n", IRQ_STR[irq]);

  // TODO: handle multiple interrupts at the same time
  if (irq == SX126X_IRQ_TX_DONE) {
    handle_tx_callback();
  } else if (irq == SX126X_IRQ_RX_DONE) {
    handle_rx_callback();
  }
}

void handle_irq_callback(uint gpio, uint32_t events) {
  if (gpio == PIN_DIO1) {
    handle_dio1_callback(gpio, events);
  } else if (gpio == PIN_BUTTON_NEXT || gpio == PIN_BUTTON_OK || gpio == PIN_BUTTON_BACK || gpio == PIN_BUTTON_PREV || gpio == PIN_BUTTON_HOME || gpio == PIN_BUTTON_SLEEP) {
    handle_button_callback(gpio, events);
  }
}

// USB serial RX callback.
// printf does not work during this callback.
void tud_cdc_rx_cb(uint8_t itf) {
  if (tud_cdc_available()) {
    // Read into console buffer, starting from the last read position. This is important in case the
    // tty is sending one character at a time instead of the whole line.
    uint32_t count = tud_cdc_read(console_buffer + console_buffer_offset,
                                  sizeof(console_buffer) - console_buffer_offset);
    console_buffer_offset += count;

    // Echo back what we read.
    tud_cdc_write(console_buffer + console_buffer_offset - count, count);
    tud_cdc_write_flush();

    // Check if we received a carriage return
    for (int i = 0; i < count; i++) {
      if (console_buffer[console_buffer_offset - count + i] == '\r') {
        console_buffer[console_buffer_offset - count + i] = 0;

        // Reset buffer offset, so that next command read will start from the beginning.
        console_buffer_offset = 0;
        // Indicate that we have new command ready to process.
        console = CONSOLE_READY;

        tud_cdc_write_char('\n');
        tud_cdc_write_flush();

        break;
      }
    }
  }
}

// UART serial RX callback
// Reads the character into the same buffer as USB.
void uart_rx_cb() {
  // static uint chars_rxed = 0;
  while (uart_is_readable(UART_PORT)) {
    uint8_t ch = uart_getc(UART_PORT);

    console_buffer[console_buffer_offset++] = ch;

    uart_putc(UART_PORT, ch);

    if (ch == '\r') {
      console_buffer[console_buffer_offset - 1] = '\0';
      console_buffer_offset = 0;
      console = CONSOLE_READY;

      uart_putc(UART_PORT, '\n');
    }
  }
}

void setup_io() {
  // Initialize the uart for printing from the pico.
  stdio_init_all();

  uart_set_fifo_enabled(UART_PORT, false);
  irq_set_exclusive_handler(UART_PORT == uart0 ? UART0_IRQ : UART1_IRQ, uart_rx_cb);
  irq_set_enabled(UART0_IRQ, true);
  uart_set_irqs_enabled(UART_PORT, true, false);

  // Wait until the usb is ready to transmit.
  // while (!tud_cdc_connected()) {
  //   sleep_ms(100);
  // }

  // Initialize the SPI interface of the pico (pins defined in the src/pico_config.h).
  spi_t spi = pico_spi_init(SPI_PORT, PIN_MISO, PIN_MOSI, PIN_SCLK, PIN_NSS);

  // Initialize other pins required to use the radio (pins defined in the src/pico_config.h).
  // `context` is used throughout the code to keep track of the configuration of our setup.
  context.spi = spi;
  context.nss = pico_gpio_init(PIN_NSS, GPIO_FUNC_SIO, GPIO_DIR_OUT, GPIO_PULL_NONE, 1);
  context.busy = pico_gpio_init(PIN_BUSY, GPIO_FUNC_SIO, GPIO_DIR_IN, GPIO_PULL_NONE, 0);
  context.reset = pico_gpio_init(PIN_RESET, GPIO_FUNC_SIO, GPIO_DIR_OUT, GPIO_PULL_NONE, 1);
  context.dio1 = pico_gpio_init(PIN_DIO1, GPIO_FUNC_SIO, GPIO_DIR_IN, GPIO_PULL_NONE, 0);

  // Enable the interrupts and set the callback function for DIO1 pin.
  pico_gpio_set_interrupt(context.dio1.pin, GPIO_IRQ_EDGE_RISE, &handle_irq_callback);

  // Initialize the buttons and set the callback function for each button.
  pico_gpio_init(PIN_BUTTON_NEXT, GPIO_FUNC_SIO, GPIO_DIR_IN, GPIO_PULL_UP, 1);
  pico_gpio_init(PIN_BUTTON_OK, GPIO_FUNC_SIO, GPIO_DIR_IN, GPIO_PULL_UP, 1);
  pico_gpio_init(PIN_BUTTON_BACK, GPIO_FUNC_SIO, GPIO_DIR_IN, GPIO_PULL_UP, 1);
  pico_gpio_init(PIN_BUTTON_PREV, GPIO_FUNC_SIO, GPIO_DIR_IN, GPIO_PULL_UP, 1);
  pico_gpio_init(PIN_BUTTON_HOME, GPIO_FUNC_SIO, GPIO_DIR_IN, GPIO_PULL_UP, 1);
  pico_gpio_init(PIN_BUTTON_SLEEP, GPIO_FUNC_SIO, GPIO_DIR_IN, GPIO_PULL_UP, 1);
  pico_gpio_set_interrupt(PIN_BUTTON_NEXT, GPIO_IRQ_EDGE_FALL, &handle_irq_callback);
  pico_gpio_set_interrupt(PIN_BUTTON_OK, GPIO_IRQ_EDGE_FALL, &handle_irq_callback);
  pico_gpio_set_interrupt(PIN_BUTTON_BACK, GPIO_IRQ_EDGE_FALL, &handle_irq_callback);
  pico_gpio_set_interrupt(PIN_BUTTON_PREV, GPIO_IRQ_EDGE_FALL, &handle_irq_callback);
  pico_gpio_set_interrupt(PIN_BUTTON_HOME, GPIO_IRQ_EDGE_FALL, &handle_irq_callback);
  pico_gpio_set_interrupt(PIN_BUTTON_SLEEP, GPIO_IRQ_EDGE_FALL, &handle_irq_callback);

  // Initialize ADC for battery.
  adc_init();
  adc_gpio_init(PIN_BATTERY_ADC);
  adc_select_input(0);

  // Blink the onboard LED to signify setup is done.
  gpio_init(PIN_STATUS_LED);
  gpio_set_dir(PIN_STATUS_LED, GPIO_OUT);
  gpio_put(PIN_STATUS_LED, 1);
  sleep_ms(500);
  gpio_put(PIN_STATUS_LED, 0);
  sleep_ms(500);
  gpio_put(PIN_STATUS_LED, 1);

  debug("io setup done\n");
}

void setup_sx126x() {
  // Try to read a register with a known reset value from the radio.
  // If this fails, either SPI connection is not setup correctly or the radio is not responding.
  uint8_t reg = 0;
  sx126x_read_register(&context, 0x0740, &reg, 1);
  if (reg != 0x14) {
    error("sanity check failed: %d\n", reg);
    while (true) {
      tight_loop_contents();
    }
  }

  // The radio is using TCXO. The DIO3 pin needs to be setup as a voltage source for the TCXO.
  // Before this step, the radio will fail to start the XOSC. This is expected so, we clear the
  // errors.
  sx126x_clear_device_errors(&context);
#ifndef PIN_CONFIG_v2
  sx126x_set_dio3_as_tcxo_ctrl(&context, SX126X_TCXO_CTRL_1_7V, 5 << 6);

  // With the TCXO correctly configured now, re-calibrate all the clock on the chip.
  sx126x_cal_mask_t calibration_mask = 0x7F;
  sx126x_cal(&context, calibration_mask);
#endif

  // Make sure the calibration succeeded.
  sx126x_errors_mask_t errors = 0;
  sx126x_chip_status_t status = {.chip_mode = 0, .cmd_status = 0};
  sx126x_get_status(&context, &status);
  sx126x_get_device_errors(&context, &errors);
  if (!(status.chip_mode == SX126X_CHIP_MODE_STBY_RC &&
        status.cmd_status == SX126X_CMD_STATUS_RFU && errors == 0)) {
    error("calibration failed\n");
    while (true) {
      tight_loop_contents();
    }
  }

  // Setup the radio for LORA at 915MHz.
  sx126x_set_standby(&context, SX126X_STANDBY_CFG_RC);
  sx126x_set_pkt_type(&context, SX126X_PKT_TYPE_LORA);
  sx126x_set_rf_freq(&context, 915000000);

  // Setup power amplifier settings for TX.
  sx126x_pa_cfg_params_t pa_cfg = {
      .pa_duty_cycle = 0x04,
      .hp_max = 0x07,
      .device_sel = 0x00,
      .pa_lut = 0x01,
  };
  sx126x_set_pa_cfg(&context, &pa_cfg);
  sx126x_set_tx_params(&context, 0x16, SX126X_RAMP_40_US);

  // Initialize modulation parameters.
  mod_params = MOD_PARAMS_DEFAULT;

  // Setup the modulation parameters for LORA.
  sx126x_set_lora_mod_params(&context, &mod_params);

  // Setup the packet parameters for LORA.
  packet_params.preamble_len_in_symb = 0x10;
  packet_params.header_type = SX126X_LORA_PKT_IMPLICIT;
  packet_params.pld_len_in_bytes = sizeof(message_t);
  packet_params.crc_is_on = true;
  packet_params.invert_iq_is_on = false;

  sx126x_set_lora_pkt_params(&context, &packet_params);

  // Setup the DIO1 pin to trigger for all interrupts.
  sx126x_set_dio_irq_params(&context, 0xFFFF, 0xFFFF, 0x0000, 0x0000);

  debug("sx126x setup done\n");
}

void print_hello() {
  printf("__      __   _     _ _      _       _     \n"
         "\\ \\    / /  (_)   | | |    (_)     | |    \n"
         " \\ \\  / /__  _  __| | |     _ _ __ | | __ \n"
         "  \\ \\/ / _ \\| |/ _` | |    | | '_ \\| |/ / \n"
         "   \\  / (_) | | (_| | |____| | | | |   <  \n"
         "    \\/ \\___/|_|\\__,_|______|_|_| |_|_|\\_\\ \n"
         "     version: %d.%d       %s is ready\n",
         VERSION_MAJOR, VERSION_MINOR, uid_to_string(get_uid()));
}

// Reads the ADC connected to the power rail and returns a voltage value.
float read_voltage() {
#ifndef PIN_CONFIG_v2
  return 0;
#endif
  // 12-bit quantization with 3.3 volt as max
  const static float conversion_factor = 3.3f / (1 << 12);
  return adc_read() * conversion_factor;
}

// Transmit bytes over the radio.
void transmit_bytes(uint8_t *bytes, uint8_t length) {
  if (length > 255) {
    error("payload too large\n");
    return;
  }

  // Set the radio to standby mode, in case we are in receive.
  sx126x_set_standby(&context, SX126X_STANDBY_CFG_RC);

  // Write the payload to the radio's buffer.
  sx126x_write_buffer(&context, 0, bytes, length);

  // Setup the packet parameters for the transmission. Only length is needed to be updated.
  sx126x_pkt_params_lora_t packet_params = {
      .preamble_len_in_symb = 0x10,
      .header_type = SX126X_LORA_PKT_EXPLICIT,
      .pld_len_in_bytes = length,
      .crc_is_on = true,
      .invert_iq_is_on = false,
  };
  sx126x_set_lora_pkt_params(&context, &packet_params);

  debug("tx: %d bytes\n", length);

  last_tx_start = get_absolute_time();

  // Start the transmission.
  sx126x_set_tx(&context, 0x0);

  sx126x_chip_status_t status = {.chip_mode = 0, .cmd_status = 0};
  sx126x_get_status(&context, &status);
  if (status.chip_mode != SX126X_CHIP_MODE_TX && status.cmd_status != SX126X_CMD_STATUS_RFU) {
    error("tx status error (mode: %d | cmd: %d)\n", status.chip_mode, status.cmd_status);
  }
}

// Transmit a string over the radio. Must be null terminated.
void transmit_string(char *string) { transmit_bytes((uint8_t *)string, strlen(string)); }

void transmit_packet(message_history_t *packet) {
  // Wait for a random amount of time.
  uint32_t timeout = get_rand_32();

  if (mod_params_local == FAST) {
    timeout %= 300;
  } else if (mod_params_local == LONGRANGE) {
    timeout %= 1000;
  } else {
    timeout %= 500;
  }

  debug("sleeping for %d ms\n", timeout);
  sleep_ms(timeout);

  // This is the time spent in the tx_queue for this packet.
  // This is not the absolute tx delta, since `transmit_bytes` function also takes time configuring
  // the transceiver. That part gets accounted for on the receiver side.
  int64_t tx_delta = absolute_time_diff_us(packet->time, get_absolute_time());
  debug("tx queue delta %llu us\n", tx_delta);

  if (packet->message.mtype == MTYPE_PING) {
    // For pings, set the current time as the time field.
    packet->message.time = get_absolute_time();
  } else if (packet->message.mtype == MTYPE_PONG) {
    // For pongs, add time spent in tx_queue.
    // This moves the reference to the future, making the difference smaller.
    packet->message.time = packet->message.time + tx_delta;
  }

  debug("->");
  for (int i = 0; i < sizeof(message_t); i++) {
    debug(" %02x", ((uint8_t *)packet)[i]);
  }
  debug("\n");

  debug("message sent from %s", uid_to_string(packet->message.src));
  debug(" to %s\n", uid_to_string(packet->message.dst));

  transmit_bytes((uint8_t *)packet, sizeof(message_t));
}

void receive_once() {
  sx126x_pkt_params_lora_t packet_params = {
      .preamble_len_in_symb = 0x10,
      .header_type = SX126X_LORA_PKT_EXPLICIT,
      .pld_len_in_bytes = 0x20,
      .crc_is_on = true,
      .invert_iq_is_on = false,
  };
  sx126x_set_lora_pkt_params(&context, &packet_params);
  sx126x_set_rx(&context, 0);
}

void receive_cont() {
  sx126x_pkt_params_lora_t packet_params = {
      .preamble_len_in_symb = 0x10,
      .header_type = SX126X_LORA_PKT_EXPLICIT,
      .pld_len_in_bytes = 0x20,
      .crc_is_on = true,
      .invert_iq_is_on = false,
  };
  sx126x_set_lora_pkt_params(&context, &packet_params);
  sx126x_set_rx_with_timeout_in_rtc_step(&context, SX126X_RX_CONTINUOUS);
}

void core1_entry() {
  wakeup_Screen();
  home_Screen();
  screen = SCREEN_DRAW_READY;

  screen_draw_loop();
}

int main() {
  message_history_t message;

  setup_io();
  setup_display();
  setup_sx126x();
  setup_network();

  memset(new_Messages, 0, sizeof(new_Messages));

  multicore_launch_core1(core1_entry);

  print_hello();

  try_transmit(new_hello_message());

  while (true) {
    // Process one previously received message.
    if (!STOP_PROCESSING && queue_try_remove(&rx_queue, &message)) {
      debug("rx dequeue %d\n", message.message.id);
      // Update the message history with the received message.
      // If the message is already received, ignore it.
      if (!check_message_history(message.message)) {
        handle_message(&message);

        if (message.message.flags.ack_req) {
          debug("sending ack\n");
          try_transmit(new_ack_message(message.message.src, message.message.id));
        }
      }
    }

    // Transmit one message if we are not already transmitting.
    if (state != STATE_TX && queue_try_remove(&tx_queue, &message)) {
      debug("tx dequeue %d\n", message.message.id);

      if (message.message.flags.ack_req) {
        // Add the message to the ack list to keep track of it.
        add_ack(&message.message);
      }

      // Set TX state before calling transmit in case IRQ triggers before we finish.
      // Otherwise, IRQ can overtake the control flow and set the state to TX_DONE,
      // which we would overwrite back to TX.
      state = STATE_TX;
      debug("STATE = TX\n");
      transmit_packet(&message);
    }

    // If we are not actively transmitting, receive instead.
    if (state == STATE_IDLE || state == STATE_TX_DONE) {
      state = STATE_RX;
      debug("STATE = RX\n");
      receive_cont();
    }

    if (screen == SCREEN_DRAW_READY) {
      multicore_fifo_push_blocking_inline(0);
      screen = SCREEN_DRAW;
    } else if (screen == SCREEN_DRAW) {
      if (multicore_fifo_rvalid()) {
        multicore_fifo_pop_blocking_inline();
        screen = SCREEN_IDLE;
      }
    }

    // Check the ack list for timeouts.
    check_ack_list();

    // USB uart RX callback job
    tud_task();

    // Process incoming console message
    if (console == CONSOLE_READY) {
      handle_console_input();
      console = CONSOLE_IDLE;
    }
  }
}
