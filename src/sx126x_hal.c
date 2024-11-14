#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "hardware/timer.h"
#include "pico/critical_section.h"

#include "sx126x_hal.h"
#include "sx126x_hal_context.h"

/**
 * @brief Wait until radio busy pin returns to 0
 */
void sx126x_hal_wait_on_busy(const void *context) {
  const sx126x_hal_context_t *sx126x_context = (const sx126x_hal_context_t *)context;
  bool gpio_state;

  do {
    gpio_state = pico_gpio_read(sx126x_context->busy.pin);
  } while (gpio_state == 1);
}

/**
 * Radio data transfer - write
 *
 * @remark Shall be implemented by the user
 *
 * @param [in] context          Radio implementation parameters
 * @param [in] command          Pointer to the buffer to be transmitted
 * @param [in] command_length   Buffer size to be transmitted
 * @param [in] data             Pointer to the buffer to be transmitted
 * @param [in] data_length      Buffer size to be transmitted
 *
 * @returns Operation status
 */
sx126x_hal_status_t sx126x_hal_write(const void *context, const uint8_t *command,
                                     const uint16_t command_length, const uint8_t *data,
                                     const uint16_t data_length) {
  const sx126x_hal_context_t *sx126x_context = (const sx126x_hal_context_t *)context;

  sx126x_hal_wait_on_busy(sx126x_context);

  pico_gpio_write(sx126x_context->nss.pin, 0);

  pico_spi_in_out(sx126x_context->spi.inst, command, NULL, command_length);
  pico_spi_in_out(sx126x_context->spi.inst, data, NULL, data_length);

  pico_gpio_write(sx126x_context->nss.pin, 1);

  return SX126X_HAL_STATUS_OK;
}

/**
 * Radio data transfer - read
 *
 * @remark Shall be implemented by the user
 *
 * @param [in] context          Radio implementation parameters
 * @param [in] command          Pointer to the buffer to be transmitted
 * @param [in] command_length   Buffer size to be transmitted
 * @param [in] data             Pointer to the buffer to be received
 * @param [in] data_length      Buffer size to be received
 *
 * @returns Operation status
 */
sx126x_hal_status_t sx126x_hal_read(const void *context, const uint8_t *command,
                                    const uint16_t command_length, uint8_t *data,
                                    const uint16_t data_length) {
  const sx126x_hal_context_t *sx126x_context = (const sx126x_hal_context_t *)context;

  sx126x_hal_wait_on_busy(sx126x_context);

  pico_gpio_write(sx126x_context->nss.pin, 0);

  pico_spi_in_out(sx126x_context->spi.inst, command, NULL, command_length);
  pico_spi_in_out(sx126x_context->spi.inst, NULL, data, data_length);

  pico_gpio_write(sx126x_context->nss.pin, 1);

  return SX126X_HAL_STATUS_OK;
}

/**
 * Reset the radio
 *
 * @remark Shall be implemented by the user
 *
 * @param [in] context Radio implementation parameters
 *
 * @returns Operation status
 */
sx126x_hal_status_t sx126x_hal_reset(const void *context) {
  const sx126x_hal_context_t *sx126x_context = (const sx126x_hal_context_t *)context;

  pico_gpio_write(sx126x_context->reset.pin, 0);
  busy_wait_ms(10);
  pico_gpio_write(sx126x_context->reset.pin, 1);

  return SX126X_HAL_STATUS_OK;
}

/**
 * Wake the radio up.
 *
 * @remark Shall be implemented by the user
 *
 * @param [in] context Radio implementation parameters
 *
 * @returns Operation status
 */
sx126x_hal_status_t sx126x_hal_wakeup(const void *context) {
  const sx126x_hal_context_t *sx126x_context = (const sx126x_hal_context_t *)context;

  critical_section_t crit;
  critical_section_init(&crit);
  critical_section_enter_blocking(&crit);

  pico_gpio_write(sx126x_context->nss.pin, 0);

  busy_wait_ms(10);

  pico_gpio_write(sx126x_context->nss.pin, 1);

  sx126x_hal_wait_on_busy(sx126x_context);

  critical_section_exit(&crit);
  critical_section_deinit(&crit);

  return SX126X_HAL_STATUS_OK;
}
