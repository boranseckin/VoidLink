#include <stdio.h>

#include "sx126x.h"

const char *STATUS[] = {
    "RESERVED",         "RFU",        "DATA_AVAILABLE", "CMD_TIMEOUT", "CMD_PROCESS_ERROR",
    "CMD_EXEC_FAILURE", "CMD_TX_DONE"};
const char *CHIP_MODES[] = {"UNUSED", "RFU", "STBY_RC", "STBY_XOSC", "FS", "RX", "TX"};
const char *ERRORS[] = {
    "RC64K_CALIBRATION", "RC13M_CALIBRATION", "PLL_CALIBRATION", "ADC_CALIBRATION",
    "IMG_CALIBRATION",   "XOSC_START",        "PLL_LOCK",        "PA_RAMP"};

void sx126x_check(const void *context) {
  sx126x_chip_status_t status = {.chip_mode = 0, .cmd_status = 0};
  sx126x_get_status(context, &status);
  sx126x_errors_mask_t errors = 0;
  sx126x_get_device_errors(context, &errors);

  printf("mode: %s (%d) | cmd: %s (%d) | errors: ", CHIP_MODES[status.chip_mode], status.chip_mode,
         STATUS[status.cmd_status], status.cmd_status);

  if (errors == 0) {
    printf("none\n");
  } else {
    for (int i = 0; i < 8; i++) {
      if (errors & (1 << i)) {
        printf("%s (%d)", ERRORS[i], i);
      }
    }
    printf("\n");
  }
}