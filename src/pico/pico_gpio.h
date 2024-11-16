#ifndef PICO_GPIO_H
#define PICO_GPIO_H

#include "hardware/gpio.h"

typedef enum {
  GPIO_PULL_NONE,
  GPIO_PULL_DOWN,
  GPIO_PULL_UP,
  GPIO_PULL_BOTH,
} gpio_pull_t;

typedef enum {
  GPIO_DIR_IN,
  GPIO_DIR_OUT,
} gpio_direction_t;

typedef struct {
  uint16_t pin;
  gpio_function_t function;
  gpio_direction_t direction;
  gpio_pull_t pull;
} gpio_t;

gpio_t pico_gpio_init(uint16_t pin, gpio_function_t function, gpio_direction_t direction,
                      gpio_pull_t pull, bool value);

void pico_gpio_set_handler(uint16_t pin, void *context);

void pico_gpio_set_interrupt(uint16_t pin, uint32_t event_mask, gpio_irq_callback_t irq_handler);

void pico_gpio_remove_interrupt(uint16_t pin);

void pico_gpio_write(uint16_t pin, bool value);

bool pico_gpio_read(uint16_t pin);

#endif // PICO_GPIO_H