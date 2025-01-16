#include <stdio.h>

#include "hardware/uart.h"
#include "pico/stdlib.h"
#include "tusb.h"

#include "pico_config.h"
#include "sx126x.h"
#include "sx126x_debug.h"
#include "sx126x_hal_context.h"

static sx126x_hal_context_t context;

// Callback function for everytime an interrupt is detected on DIO1.
void dio1_callback(uint gpio, uint32_t events) {
  printf("IRQ: ");

  // Get the irq status to learn why the interrupt was triggered.
  // And clean the interrupt at the same time, since it is getting handled now.
  sx126x_irq_mask_t irq = 0;
  sx126x_get_and_clear_irq_status(&context, &irq);
  if (irq == SX126X_IRQ_TX_DONE) {
    printf("TX_DONE\n");

    sx126x_chip_status_t status = {.chip_mode = 0, .cmd_status = 0};
    sx126x_get_status(&context, &status);
    if (!(status.chip_mode == SX126X_CHIP_MODE_STBY_RC &&
          status.cmd_status == SX126X_CMD_STATUS_CMD_TX_DONE)) {
      printf("status error (mode: %d | cmd: %d)\n", status.chip_mode, status.cmd_status);
      return;
    }
  } else if (irq == SX126X_IRQ_RX_DONE) {
    printf("RX_DONE\n");
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
  pico_gpio_set_interrupt(context.dio1.pin, GPIO_IRQ_EDGE_RISE, &dio1_callback);

  // Light up the onboard LED to signify setup is done.
  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
  gpio_put(PICO_DEFAULT_LED_PIN, 1);

  sleep_ms(1000);
  printf("Pico Lora\n");

  // Try to read a register with a known reset value from the radio.
  // If this fails, either SPI connection is not setup correctly or the radio is not responding.
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

  // The radio is using TCXO. The DIO3 pin needs to be setup as a voltage source to the TCXO.
  // Before this step, the radio will fail to start the XOSC, which is expected so we clear the
  // errors.
  sx126x_clear_device_errors(&context);
  sx126x_set_dio3_as_tcxo_ctrl(&context, SX126X_TCXO_CTRL_1_7V, 5 << 6);

  // With the TCXO correctly configured now, re-calibrate all the clock on the chip.
  sx126x_cal_mask_t calibration_mask = 0x7F;
  sx126x_cal(&context, calibration_mask);

  // Make sure the calibration succeeded.
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

  // Setup the radio for TX
  sx126x_set_standby(&context, SX126X_STANDBY_CFG_RC);
  sx126x_set_pkt_type(&context, SX126X_PKT_TYPE_LORA);
  sx126x_set_rf_freq(&context, 915000000);

  sx126x_pa_cfg_params_t pa_cfg = {
      .pa_duty_cycle = 0x04,
      .hp_max = 0x07,
      .device_sel = 0x00,
      .pa_lut = 0x01,
  };
  sx126x_set_pa_cfg(&context, &pa_cfg);
  sx126x_set_tx_params(&context, 0x16, SX126X_RAMP_40_US);

  char data[] = "rx: 4 8 16 48";

  sx126x_write_buffer(&context, 0, data, strlen(data));

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
      .pld_len_in_bytes = strlen(data),
      .crc_is_on = true,
      .invert_iq_is_on = false,
  };
  sx126x_set_lora_pkt_params(&context, &packet_params);

  sx126x_set_dio_irq_params(&context, 0xFFFF, 0xFFFF, 0x0000, 0x0000);

  // uint8_t write_reg_data = 0x34;
  // sx126x_write_register(&context, 0x0740, &write_reg_data, 1);
  // write_reg_data = 0x44;
  // sx126x_write_register(&context, 0x0741, &write_reg_data, 1);

  while (true) {
    // tight_loop_contents();
    sx126x_set_tx(&context, 0x0);
    sx126x_check(&context);
    printf("TX started\n");
    sleep_ms(8000);
  }
}
