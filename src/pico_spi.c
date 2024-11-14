#include "pico_spi.h"

spi_t pico_spi_init(spi_inst_t *inst, uint8_t miso, uint8_t mosi, uint8_t sclk, uint8_t nss) {
  spi_t spi = {
      .inst = inst,
      .miso = {.pin = miso, .function = GPIO_FUNC_SPI},
      .mosi = {.pin = mosi, .function = GPIO_FUNC_SPI},
      .sclk = {.pin = sclk, .function = GPIO_FUNC_SPI},
  };

  spi_init(spi.inst, 10 * 1000 * 1000);
  spi_set_format(spi.inst, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
  gpio_set_function(spi.mosi.pin, GPIO_FUNC_SPI);
  gpio_set_function(spi.miso.pin, GPIO_FUNC_SPI);
  gpio_set_function(spi.sclk.pin, GPIO_FUNC_SPI);

  return spi;
}

void pico_spi_in_out(spi_inst_t *spi, const uint8_t *out, uint8_t *in, size_t len) {
  uint8_t dummy_buf[len];
  spi_write_read_blocking(spi, (out != NULL) ? out : dummy_buf, (in != NULL) ? in : dummy_buf, len);
}