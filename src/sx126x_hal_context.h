#ifndef SX126X_HAL_CONTEXT_H
#define SX126X_HAL_CONTEXT_H

#include "pico_gpio.h"
#include "pico_spi.h"

typedef struct {
  spi_t spi;
  gpio_t nss;
  gpio_t reset;
  gpio_t busy;
  gpio_t dio1;
} sx126x_hal_context_t;

#endif // SX126X_HAL_CONTEXT_H