#include <stdio.h>

#include "hardware/uart.h"
#include "pico/stdlib.h"

#include "pico_config.h"
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

  while (true) {
    printf("Hello, world!\n");
    sleep_ms(1000);
  }
}
