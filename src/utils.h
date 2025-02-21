#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

#include "sx126x.h"

// #define DEBUG

// only print if DEBUG is enabled
#ifdef DEBUG
#define debug(...) printf(__VA_ARGS__)
#else
#define debug(...)
#endif

#define error(...) printf(__VA_ARGS__)

static const char *STATUS[] = {
    "RESERVED",         "RFU",        "DATA_AVAILABLE", "CMD_TIMEOUT", "CMD_PROCESS_ERROR",
    "CMD_EXEC_FAILURE", "CMD_TX_DONE"};
static const char *CHIP_MODES[] = {"UNUSED", "RFU", "STBY_RC", "STBY_XOSC", "FS", "RX", "TX"};
static const char *ERRORS[] = {
    "RC64K_CALIBRATION", "RC13M_CALIBRATION", "PLL_CALIBRATION", "ADC_CALIBRATION",
    "IMG_CALIBRATION",   "XOSC_START",        "PLL_LOCK",        "PA_RAMP"};

// 16 predefined text messages
static const char *TEXT_MESSAGES[] = {
    "OK",   "NO",     "Over",         "Out", "Go ahead",       "Stand-by",      "Come in",
    "Copy", "Repeat", "Break, break", "SOS", "Good reception", "Bad reception", "Stay put",
    "Move",
};

// Predefined modulation parameters
static const sx126x_mod_params_lora_t MOD_PARAMS_DEFAULT = {
    .sf = SX126X_LORA_SF10,
    .bw = SX126X_LORA_BW_250,
    .cr = SX126X_LORA_CR_4_5,
    .ldro = 0,
};

static const sx126x_mod_params_lora_t MOD_PARAMS_FAST = {
    .sf = SX126X_LORA_SF7,
    .bw = SX126X_LORA_BW_250,
    .cr = SX126X_LORA_CR_4_5,
    .ldro = 0,
};

static const sx126x_mod_params_lora_t MOD_PARAMS_LONGRANGE = {
    .sf = SX126X_LORA_SF12,
    .bw = SX126X_LORA_BW_125,
    .cr = SX126X_LORA_CR_4_8,
    .ldro = 1, // low data-rate optimization is recommended for SF12 & 125kHz
};

void sx126x_check(const void *context);
void sx126x_print_decoded_irq(sx126x_irq_mask_t mask);

#endif // UTILS_H
