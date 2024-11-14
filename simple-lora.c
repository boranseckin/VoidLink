#include <stdio.h>

#include "hardware/uart.h"
#include "pico/stdlib.h"

#include "pico_config.h"
#include "sx126x.h"
#include "sx126x_hal_context.h"

int main() {
  stdio_init_all();

  spi_t spi = pico_spi_init(SPI_PORT, PIN_MISO, PIN_MOSI, PIN_SCLK, PIN_NSS);

  const sx126x_hal_context_t context = {
      .spi = spi,
      .nss = pico_gpio_init(PIN_NSS, GPIO_FUNC_SIO, GPIO_DIR_OUT, GPIO_PULL_NONE, 1),
      .busy = pico_gpio_init(PIN_BUSY, GPIO_FUNC_SIO, GPIO_DIR_IN, GPIO_PULL_NONE, 0),
      .reset = pico_gpio_init(PIN_RESET, GPIO_FUNC_SIO, GPIO_DIR_OUT, GPIO_PULL_NONE, 1),
      .dio1 = pico_gpio_init(PIN_DIO1, GPIO_FUNC_SIO, GPIO_DIR_IN, GPIO_PULL_NONE, 0),
  };

  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
  gpio_put(PICO_DEFAULT_LED_PIN, 1);

  printf("Pico Lora\n");

  uint8_t reg = 0;
  sx126x_read_register(&context, 0x0740, reg, 1);
  if (reg == 0x14) {
    printf("sanity check passed\n");
  } else {
    printf("sanity check failed: %d\n", reg);
    while (1) {
      tight_loop_contents();
    }
  }

  while (true) {
    printf("Hello, world!\n");
    sleep_ms(1000);
  }
}
