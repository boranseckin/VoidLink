#ifndef UTILS_H
#define UTILS_H

#include "sx126x.h"

#define DEBUG

// only print if DEBUG is enabled
#ifdef DEBUG
#define debug(...) printf(__VA_ARGS__)
#else
#define debug(...)
#endif

#define error(...) printf(__VA_ARGS__)

#define debug(...) printf(__VA_ARGS__)

static const char *STATUS[] = {
    "RESERVED",         "RFU",        "DATA_AVAILABLE", "CMD_TIMEOUT", "CMD_PROCESS_ERROR",
    "CMD_EXEC_FAILURE", "CMD_TX_DONE"};
static const char *CHIP_MODES[] = {"UNUSED", "RFU", "STBY_RC", "STBY_XOSC", "FS", "RX", "TX"};
static const char *ERRORS[] = {
    "RC64K_CALIBRATION", "RC13M_CALIBRATION", "PLL_CALIBRATION", "ADC_CALIBRATION",
    "IMG_CALIBRATION",   "XOSC_START",        "PLL_LOCK",        "PA_RAMP"};

void sx126x_check(const void *context);
void sx126x_print_decoded_irq(sx126x_irq_mask_t mask);

#endif // UTILS_H
