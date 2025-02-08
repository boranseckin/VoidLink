#include <stdio.h>

#include "sx126x.h"

#include "utils.h"

void sx126x_check(const void *context) {
  sx126x_chip_status_t status = {.chip_mode = 0, .cmd_status = 0};
  sx126x_get_status(context, &status);
  sx126x_errors_mask_t errors = 0;
  sx126x_get_device_errors(context, &errors);

  debug("mode: %s (%d) | cmd: %s (%d) | errors: ", CHIP_MODES[status.chip_mode], status.chip_mode,
        STATUS[status.cmd_status], status.cmd_status);

  if (errors == 0) {
    debug("none\n");
  } else {
    for (int i = 0; i < 8; i++) {
      if (errors & (1 << i)) {
        debug("%s (%d)", ERRORS[i], i);
      }
    }
    debug("\n");
  }
}

void sx126x_print_decoded_irq(sx126x_irq_mask_t mask) {
  if (mask == SX126X_IRQ_TX_DONE) {
    debug("IRQ: TX_DONE\n");
  } else if (mask == SX126X_IRQ_RX_DONE) {
    debug("IRQ: RX_DONE\n");
  } else if (mask == SX126X_IRQ_PREAMBLE_DETECTED) {
    debug("IRQ: PREAMBLE_DETECTED\n");
  } else if (mask == SX126X_IRQ_SYNC_WORD_VALID) {
    debug("IRQ: SYNC_WORD_VALID\n");
  } else if (mask == SX126X_IRQ_HEADER_VALID) {
    debug("IRQ: HEADER_VALID\n");
  } else if (mask == SX126X_IRQ_CRC_ERROR) {
    debug("IRQ: CRC_ERROR\n");
  } else if (mask == SX126X_IRQ_CAD_DONE) {
    debug("IRQ: CAD_ERROR\n");
  } else if (mask == SX126X_IRQ_CAD_DETECTED) {
    debug("IRQ: CAD_DETECTED\n");
  } else if (mask == SX126X_IRQ_TIMEOUT) {
    debug("IRQ: TIMEOUT\n");
  } else if (mask == SX126X_IRQ_LR_FHSS_HOP) {
    debug("IRQ: LR_FHSS_HOP\n");
  } else {
    debug("IRQ: unknown or multiple irqs\n");
  }
}
