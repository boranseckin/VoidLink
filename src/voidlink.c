#include <pico/time.h>
#include <stdio.h>

#include "class/cdc/cdc_device.h"
#include "pico/multicore.h"
#include "pico/unique_id.h"
#include "tusb.h"

#include "pico_config.h"
#include "sx126x.h"
#include "sx126x_hal_context.h"

#include "EPD_2in13_V4.h"
#include "GUI_Paint.h"

#include "network.h"
#include "utils.h"
#include "voidlink.h"

static message_t rx_payload_buf;
static message_t tx_payload_buf;

// ((EPD_2in13_V4_WIDTH % 8 == 0) ? (EPD_2in13_V4_WIDTH / 8) : (EPD_2in13_V4_WIDTH / 8 + 1)) *
// EPD_2in13_V4_HEIGHT = 4080
#define IMAGE_SIZE 4080
static uint8_t image[IMAGE_SIZE];
static uint8_t cursor = 0;

// Main state machine
typedef enum {
  STATE_IDLE,
  STATE_TX_READY,
  STATE_TX,
  STATE_TX_DONE,
  STATE_RX,
} state_t;

static state_t state = STATE_IDLE;

// Screen state machine
typedef enum {
  SCREEN_IDLE,
  SCREEN_DRAW_READY,
  SCREEN_DRAW,
} screen_t;

static screen_t screen = SCREEN_IDLE;

static sx126x_hal_context_t context;

// Handle a received message.
void handle_message(message_t *incoming) {
  printf("message received from %s", uid_to_string(incoming->src));
  printf(" to %s\n", uid_to_string(incoming->dst));

  if (incoming->mtype == MTYPE_ACK) {
    printf("rx: ack: %d\n", incoming->data[0]);
  } else if (incoming->mtype == MTYPE_HELLO) {
    printf("rx: hello\n");
    tx_payload_buf = new_ack_message(incoming->src, incoming->id);
    state = STATE_TX_READY;
    return;
  } else if (incoming->mtype == MTYPE_PING) {
    printf("rx: ping\n");
    tx_payload_buf = new_pong_message(incoming->src);
    state = STATE_TX_READY;
    return;
  } else if (incoming->mtype == MTYPE_PONG) {
    printf("rx: pong\n");
  } else if (incoming->mtype == MTYPE_TEXT) {
    printf("rx: text: %s\n", text[incoming->data[0]]);
  } else if (incoming->mtype == MTYPE_REQ) {
    printf("rx: request: %d\n", incoming->data[0]);
    if (incoming->data[0] == INFO_VERSION) {
      tx_payload_buf =
          new_response_message(incoming->src, INFO_VERSION, VERSION_MAJOR << 8 | VERSION_MINOR);
    } else {
      tx_payload_buf = new_response_message(incoming->src, incoming->data[0], 0);
    }
    state = STATE_TX_READY;
    return;
  } else if (incoming->mtype == MTYPE_RES) {
    printf("rx: response: %d %d %d\n", incoming->data[0], incoming->data[1], incoming->data[2]);
  } else if (incoming->mtype == MTYPE_RAW) {
    printf("rx: raw: %d %d %d\n", incoming->data[0], incoming->data[1], incoming->data[2]);
  }

  state = STATE_RX;
}

void handle_tx_callback() {
  sx126x_chip_status_t status = {.chip_mode = 0, .cmd_status = 0};
  sx126x_get_status(&context, &status);
  if (status.cmd_status != SX126X_CMD_STATUS_CMD_TX_DONE) {
    error("tx status error (mode: %d | cmd: %d)\n", status.chip_mode, status.cmd_status);
    return;
  }

  debug("tx done\n");

  state = STATE_TX_DONE;
}

void handle_rx_callback() {
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
  sx126x_read_buffer(&context, buffer_status.buffer_start_pointer, (uint8_t *)&rx_payload_buf,
                     buffer_status.pld_len_in_bytes);

  debug("<-");
  for (int i = 0; i < buffer_status.pld_len_in_bytes; i++) {
    debug(" %d", ((uint8_t *)&rx_payload_buf)[i]);
  }
  debug("\n");

  // Draw the received message on the e-paper display.
  // add "rx: " to the beginning of the payload
  // Paint_ClearWindows(0, 106, 122, 122, WHITE);
  // char payload_buf_with_rx[buffer_status.pld_len_in_bytes + 4];
  // sprintf(payload_buf_with_rx, "rx: %s", rx_payload_buf);
  // Paint_DrawString(0, 106, (char *)payload_buf_with_rx, &Font16, BLACK, WHITE);
  // screen = SCREEN_DRAW_READY;

  // Get the packet status to learn the signal strength of the received message.
  sx126x_pkt_status_lora_t pkt_status = {0};
  sx126x_get_lora_pkt_status(&context, &pkt_status);

  // Update the neighbour table with the information from received message.
  update_neighbour(rx_payload_buf.src, pkt_status.signal_rssi_pkt_in_dbm);

  // Check if the received message is for us.
  if (!is_broadcast(rx_payload_buf.dst) && !is_my_uid(rx_payload_buf.dst)) {
    debug("message not for me\n");
    return;
  }

  // Update the message history with the received message.
  if (check_message_history(rx_payload_buf)) {
    return;
  }

  // Handle the received message.
  handle_message(&rx_payload_buf);
}

// Callback function for everytime an interrupt is detected on DIO1.
void handle_dio1_callback(uint gpio, uint32_t events) {
  // Get the irq status to learn why the interrupt was triggered.
  // And clean the interrupt at the same time, since it is getting handled now.
  sx126x_irq_mask_t irq = 0;
  sx126x_get_and_clear_irq_status(&context, &irq);

  sx126x_print_decoded_irq(irq);

  // TODO: handle multiple interrupts at the same time
  if (irq == SX126X_IRQ_TX_DONE) {
    handle_tx_callback();
  } else if (irq == SX126X_IRQ_RX_DONE) {
    handle_rx_callback();
  }
}

void handle_button_callback(uint gpio, uint32_t events) {
  if (screen != SCREEN_IDLE || state != STATE_RX)
    return;

  if (gpio == PIN_BUTTON_NEXT) {
    cursor = (cursor + 1) % 3;
    for (int i = 0; i < 3; i++) {
      Paint_ClearWindows(0, 34 + i * 24, 20, 58 + i * 24, WHITE);
    }
    Paint_DrawString(0, 34 + cursor * 24, ">", &Font16, BLACK, WHITE);
    screen = SCREEN_DRAW_READY;
  } else if (gpio == PIN_BUTTON_OK) {
    if (cursor == 0) {
      tx_payload_buf = new_ping_message(get_broadcast_uid());
    } else if (cursor == 1) {
      tx_payload_buf = new_text_message(get_broadcast_uid(), TEXT_COPY);
    } else if (cursor == 2) {
      tx_payload_buf = new_request_message(get_broadcast_uid(), INFO_VERSION);
    }
    state = STATE_TX_READY;
  } else if (gpio == PIN_BUTTON_BACK) {
  }
}

void handle_irq_callback(uint gpio, uint32_t events) {
  if (gpio == PIN_DIO1) {
    handle_dio1_callback(gpio, events);
  } else if (gpio == PIN_BUTTON_NEXT || gpio == PIN_BUTTON_OK || gpio == PIN_BUTTON_BACK) {
    handle_button_callback(gpio, events);
  }
}

// USB uart RX callback
void tud_cdc_rx_cb(uint8_t itf) {
  // keep reading into a static buffer until carriage return
  static char buf[64];
  static uint8_t len = 0;

  while (tud_cdc_available()) {
    char c = tud_cdc_read_char();
    if (c == '\r') {
      buf[len] = 0;
      len = 0;

      if (strncmp(buf, "ping", 4) == 0) {
        if (strlen(buf) == 4) {
          tx_payload_buf = new_ping_message(get_broadcast_uid());
        } else {
          uid_t dst = {.bytes = {0x00, 0x00, 0x00}};
          sscanf(buf + 5, "%02hhx:%02hhx:%02hhx", &dst.bytes[0], &dst.bytes[1], &dst.bytes[2]);
          tx_payload_buf = new_ping_message(dst);
        }
        state = STATE_TX_READY;
      }

      tud_cdc_write_char('\n');
      tud_cdc_write_flush();
    } else {
      buf[len++] = c;
      tud_cdc_write_char(c);
      tud_cdc_write_flush();
    }
  }
}

void setup_io() {
  // Initialize the uart for printing from the pico.
  stdio_init_all();

  // Wait until the usb is ready to transmit.
  while (!tud_cdc_connected()) {
    sleep_ms(100);
  }

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
  pico_gpio_set_interrupt(PIN_BUTTON_NEXT, GPIO_IRQ_EDGE_FALL, &handle_irq_callback);
  pico_gpio_set_interrupt(PIN_BUTTON_OK, GPIO_IRQ_EDGE_FALL, &handle_irq_callback);
  pico_gpio_set_interrupt(PIN_BUTTON_BACK, GPIO_IRQ_EDGE_FALL, &handle_irq_callback);

  // Light up the onboard LED to signify setup is done.
  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
  gpio_put(PICO_DEFAULT_LED_PIN, 1);

  debug("io setup done\n");
}

void setup_display() {
  DEV_Module_Init();
  EPD_2in13_V4_Init();
  EPD_2in13_V4_Clear();
  EPD_2in13_V4_Sleep();

  debug("display setup done\n");
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
  sx126x_set_dio3_as_tcxo_ctrl(&context, SX126X_TCXO_CTRL_1_7V, 5 << 6);

  // With the TCXO correctly configured now, re-calibrate all the clock on the chip.
  sx126x_cal_mask_t calibration_mask = 0x7F;
  sx126x_cal(&context, calibration_mask);

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

  // Setup the modulation parameters for LORA.
  sx126x_mod_params_lora_t mod_params = {
      .sf = SX126X_LORA_SF7,
      .bw = SX126X_LORA_BW_250,
      .cr = SX126X_LORA_CR_4_5,
      .ldro = 0,
  };
  sx126x_set_lora_mod_params(&context, &mod_params);

  // Setup the packet parameters for LORA.
  sx126x_pkt_params_lora_t packet_params = {
      .preamble_len_in_symb = 0x10,
      .header_type = SX126X_LORA_PKT_EXPLICIT,
      .pld_len_in_bytes = 0x20,
      .crc_is_on = true,
      .invert_iq_is_on = false,
  };
  sx126x_set_lora_pkt_params(&context, &packet_params);

  // Setup the DIO1 pin to trigger for all interrupts.
  sx126x_set_dio_irq_params(&context, 0xFFFF, 0xFFFF, 0x0000, 0x0000);

  debug("sx126x setup done\n");
}

void setup_network() {
  // it seems trying to read the unique id too early causes a crash
  sleep_ms(100);

  pico_unique_board_id_t board_id;
  pico_get_unique_board_id(&board_id);
  MY_UID.bytes[0] = board_id.id[5];
  MY_UID.bytes[1] = board_id.id[6];
  MY_UID.bytes[2] = board_id.id[7];

  debug("network setup done\n");
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

  // Start the transmission.
  sx126x_set_tx(&context, 0x0);
  sx126x_check(&context);
  state = STATE_TX;
}

// Transmit a string over the radio. Must be null terminated.
void transmit_string(char *string) { transmit_bytes((uint8_t *)string, strlen(string)); }

void transmit_packet(message_t *packet) {
  debug("->");
  for (int i = 0; i < sizeof(message_t); i++) {
    debug(" %d", ((uint8_t *)packet)[i]);
  }
  debug("\n");

  debug("message sent from %s", uid_to_string(packet->src));
  debug(" to %s\n", uid_to_string(packet->dst));

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
  debug("core1 started\n");

  // Create a new display buffer
  Paint_NewImage(image, EPD_2in13_V4_WIDTH, EPD_2in13_V4_HEIGHT, 90, WHITE);
  // Paint the whole frame white
  Paint_Clear(WHITE);

  // Draw message selection screen
  Paint_DrawString(0, 10, "Select a message:", &Font16, BLACK, WHITE);
  Paint_DrawString(20, 34, "1. Send Ping", &Font16, BLACK, WHITE);
  Paint_DrawString(20, 58, "2. Send \"COPY\"", &Font16, BLACK, WHITE);
  Paint_DrawString(20, 82, "3. Request version", &Font16, BLACK, WHITE);

  // Display cursor
  Paint_DrawString(0, 34, ">", &Font16, BLACK, WHITE);

  screen = SCREEN_DRAW_READY;

  while (true) {
    uint32_t flag = multicore_fifo_pop_blocking();

    if (flag == 0) {
      EPD_2in13_V4_Init();
      EPD_2in13_V4_Display_Base(image);
      EPD_2in13_V4_Sleep();
      sleep_ms(100);
      multicore_fifo_push_blocking_inline(0);
    }
  }
}

int main() {
  sx126x_errors_mask_t errors = 0;
  sx126x_chip_status_t status = {.chip_mode = 0, .cmd_status = 0};

  setup_io();
  setup_display();
  setup_sx126x();
  setup_network();

  multicore_launch_core1(core1_entry);

  print_hello();

  while (true) {
    if (state == STATE_TX_READY) {
      transmit_packet(&tx_payload_buf);
    } else if (state == STATE_TX_DONE) {
      state = STATE_IDLE;
    } else if (state == STATE_IDLE) {
      receive_cont();
      state = STATE_RX;
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

    // USB uart RX callback job
    tud_task();
  }
}
