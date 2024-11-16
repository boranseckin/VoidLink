#ifndef PICO_SPI_H
#define PICO_SPI_H

#include "hardware/spi.h"

#include "pico_gpio.h"

typedef struct {
  spi_inst_t *inst;
  gpio_t miso;
  gpio_t mosi;
  gpio_t sclk;
} spi_t;

spi_t pico_spi_init(spi_inst_t *inst, uint8_t miso, uint8_t mosi, uint8_t sclk, uint8_t nss);

void pico_spi_in_out(spi_inst_t *spi, const uint8_t *out, uint8_t *in, size_t len);

#endif // PICO_SPI_H