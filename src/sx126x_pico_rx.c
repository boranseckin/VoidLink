#include <stdio.h>

#include "hardware/uart.h"
#include "pico/stdlib.h"
#include "tusb.h"

#include "pico_config.h"
#include "sx126x.h"
#include "sx126x_debug.h"
#include "sx126x_hal_context.h"

#define PAYLOAD_BUFFER_SIZE 512

static uint8_t payload_buf[PAYLOAD_BUFFER_SIZE];
static sx126x_rx_buffer_status_t buffer_status;

static sx126x_hal_context_t context;

void dio1_callback(uint gpio, uint32_t events) {
  printf("IRQ: ");

  sx126x_irq_mask_t irq = 0;
  sx126x_get_and_clear_irq_status(&context, &irq);
  if (irq == SX126X_IRQ_TX_DONE) {
    printf("TX_DONE\n");
  } else if (irq == SX126X_IRQ_RX_DONE) {
    printf("RX_DONE\n");

    // Make sure the data available status is set before reading the rx buffer.
    sx126x_chip_status_t status = {.chip_mode = 0, .cmd_status = 0};
    sx126x_get_status(&context, &status);
    if (status.cmd_status != SX126X_CMD_STATUS_DATA_AVAILABLE) {
      printf("status error (mode: %d | cmd: %d)\n", status.chip_mode, status.cmd_status);
      return;
    }

    // Get length and the start address of the received message.
    sx126x_get_rx_buffer_status(&context, &buffer_status);
    printf("payload received: %d @ %d\n", buffer_status.pld_len_in_bytes,
           buffer_status.buffer_start_pointer);

    // Make sure the buffer has enough space to read the message.
    if (buffer_status.pld_len_in_bytes > PAYLOAD_BUFFER_SIZE) {
      printf("payload is bigger than the buffer (%d)\n", PAYLOAD_BUFFER_SIZE);
      return;
    }

    // Read and print the buffer.
    sx126x_read_buffer(&context, buffer_status.buffer_start_pointer, payload_buf,
                       buffer_status.pld_len_in_bytes);

    printf("->");
    for (int i = 0; i < buffer_status.pld_len_in_bytes; i++) {
      printf(" %d", payload_buf[i]);
    }
    printf("\n");
  } else if (irq == SX126X_IRQ_PREAMBLE_DETECTED) {
    printf("PREAMBLE_DETECTED\n");
  } else if (irq == SX126X_IRQ_SYNC_WORD_VALID) {
    printf("SYNC_WORD_VALID\n");
  } else if (irq == SX126X_IRQ_HEADER_VALID) {
    printf("HEADER_VALID\n");
  } else if (irq == SX126X_IRQ_CRC_ERROR) {
    printf("CRC_ERROR\n");
  } else if (irq == SX126X_IRQ_CAD_DONE) {
    printf("CAD_ERROR\n");
  } else if (irq == SX126X_IRQ_CAD_DETECTED) {
    printf("CAD_DETECTED\n");
  } else if (irq == SX126X_IRQ_TIMEOUT) {
    printf("TIMEOUT\n");
  } else if (irq == SX126X_IRQ_LR_FHSS_HOP) {
    printf("LR_FHSS_HOP\n");
  } else {
    printf("unknown or multiple irqs\n");
  }
}

int main() {
  sx126x_errors_mask_t errors = 0;
  sx126x_chip_status_t status = {.chip_mode = 0, .cmd_status = 0};

  memset(payload_buf, 0, PAYLOAD_BUFFER_SIZE);

  stdio_init_all();

  while (!tud_cdc_connected()) {
    sleep_ms(100);
  }

  spi_t spi = pico_spi_init(SPI_PORT, PIN_MISO, PIN_MOSI, PIN_SCLK, PIN_NSS);

  context.spi = spi;
  context.nss = pico_gpio_init(PIN_NSS, GPIO_FUNC_SIO, GPIO_DIR_OUT, GPIO_PULL_NONE, 1);
  context.busy = pico_gpio_init(PIN_BUSY, GPIO_FUNC_SIO, GPIO_DIR_IN, GPIO_PULL_NONE, 0);
  context.reset = pico_gpio_init(PIN_RESET, GPIO_FUNC_SIO, GPIO_DIR_OUT, GPIO_PULL_NONE, 1);
  context.dio1 = pico_gpio_init(PIN_DIO1, GPIO_FUNC_SIO, GPIO_DIR_IN, GPIO_PULL_NONE, 0);

  pico_gpio_set_interrupt(context.dio1.pin, GPIO_IRQ_EDGE_RISE, &dio1_callback);

  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
  gpio_put(PICO_DEFAULT_LED_PIN, 1);

  sleep_ms(1000);
  printf("Pico Lora\n");

  uint8_t reg = 0;
  sx126x_read_register(&context, 0x0740, &reg, 1);
  if (reg == 0x14) {
    printf("sanity check passed\n");
  } else {
    printf("sanity check failed: %d\n", reg);
    while (true) {
      tight_loop_contents();
    }
  }

  sx126x_clear_device_errors(&context);

  sx126x_set_dio3_as_tcxo_ctrl(&context, SX126X_TCXO_CTRL_1_7V, 5 << 6);

  sx126x_cal_mask_t calibration_mask = 0x7F;
  sx126x_cal(&context, calibration_mask);

  sx126x_get_status(&context, &status);
  sx126x_get_device_errors(&context, &errors);
  if (status.chip_mode == SX126X_CHIP_MODE_STBY_RC && status.cmd_status == SX126X_CMD_STATUS_RFU &&
      errors == 0) {
    printf("calibration done\n");
  } else {
    printf("calibration failed\n");
    while (true) {
      tight_loop_contents();
    }
  }

  sx126x_set_standby(&context, SX126X_STANDBY_CFG_RC);
  sx126x_set_pkt_type(&context, SX126X_PKT_TYPE_LORA);
  sx126x_set_rf_freq(&context, 915000000);

  sx126x_mod_params_lora_t mod_params = {
      .sf = SX126X_LORA_SF7,
      .bw = SX126X_LORA_BW_250,
      .cr = SX126X_LORA_CR_4_5,
      .ldro = 0,
  };
  sx126x_set_lora_mod_params(&context, &mod_params);

  sx126x_pkt_params_lora_t packet_params = {
      .preamble_len_in_symb = 0x10,
      .header_type = SX126X_LORA_PKT_EXPLICIT,
      .pld_len_in_bytes = 0x4,
      .crc_is_on = true,
      .invert_iq_is_on = false,
  };
  sx126x_set_lora_pkt_params(&context, &packet_params);

  sx126x_set_dio_irq_params(&context, 0xFFFF, 0xFFFF, 0x0000, 0x0000);

  // uint8_t write_reg_data = 0x34;
  // sx126x_write_register(&context, 0x0740, &write_reg_data, 1);
  // write_reg_data = 0x44;
  // sx126x_write_register(&context, 0x0741, &write_reg_data, 1);

  // continuous receive
  // set_rx only accepts timeout in ms, so we use the RTC step function to achieve continuous
  sx126x_set_rx_with_timeout_in_rtc_step(&context, SX126X_RX_CONTINUOUS);
  sx126x_check(&context);
  printf("RX started\n");

  while (true) {
    tight_loop_contents();
  }
}
