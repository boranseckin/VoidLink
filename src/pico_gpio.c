#include "pico_gpio.h"

gpio_t pico_gpio_init(uint16_t pin, gpio_function_t function, gpio_direction_t direction,
                      gpio_pull_t pull, bool value) {

  gpio_t gpio = {
      .pin = pin,
      .function = function,
      .direction = direction,
      .pull = pull,
  };

  gpio_set_function(gpio.pin, gpio.function);
  gpio_set_dir(gpio.pin, gpio.direction);

  if (gpio.pull == GPIO_PULL_NONE) {
    gpio_disable_pulls(gpio.pin);
  } else {
    gpio_set_pulls(gpio.pin, gpio.pull == GPIO_PULL_UP || gpio.pull == GPIO_PULL_BOTH,
                   gpio.pull == GPIO_PULL_DOWN || gpio.pull == GPIO_PULL_BOTH);
  }

  if (gpio.direction == GPIO_DIR_OUT) {
    pico_gpio_write(&gpio, value);
  }

  return gpio;
}

void pico_gpio_write(uint16_t pin, bool value) { gpio_put(pin, value); }

bool pico_gpio_read(uint16_t pin) { return gpio_get(pin); }

void pico_gpio_set_interrupt(uint16_t pin, uint32_t event_mask, gpio_irq_callback_t irq_handler) {
  gpio_set_irq_enabled_with_callback(pin, GPIO_IRQ_EDGE_RISE, true, irq_handler);
}

void pico_gpio_remove_interrupt(uint16_t pin) {
  gpio_set_irq_enabled(pin, GPIO_IRQ_EDGE_RISE, false);
}