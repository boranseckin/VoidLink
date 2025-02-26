#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

#include "network.h"
#include "sx126x.h"

#define DEBUG

// only print if DEBUG is enabled
#ifdef DEBUG
#define debug(...) printf(__VA_ARGS__)
#else
#define debug(...)
#endif

#define info(...) printf(__VA_ARGS__)

#define error(...) printf(__VA_ARGS__)

#define _PRINT_DEFINE(x) #x
#define PRINT_DEFINE(x) _PRINT_DEFINE(x)

// Predefined modulation parameters
typedef enum {
  DEFAULT,
  FAST,
  LONGRANGE,
} mod_params_t;

static const char *MOD_PARAM_STR[] = {
    [DEFAULT] = "DEFAULT",
    [FAST] = "FAST",
    [LONGRANGE] = "LONGRANGE",
};

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

static const char *TEXT_MESSAGE_STR[] = {
    [TEXT_OK] = "OK",
    [TEXT_NO] = "NO",
    [TEXT_OVER] = "OVER",
    [TEXT_OUT] = "OUT",
    [TEXT_GO_AHEAD] = "GO AHEAD",
    [TEXT_STAND_BY] = "STAND BY",
    [TEXT_COME_IN] = "COME IN",
    [TEXT_COPY] = "COPY",
    [TEXT_REPEAT] = "REPEAT",
    [TEXT_BREAK_BREAK] = "BREAK, BREAK",
    [TEXT_SOS] = "SOS",
    [TEXT_GOOD_RECEPTION] = "GOOD RECEPTION",
    [TEXT_BAD_RECEPTION] = "BAD RECEPTION",
    [TEXT_STAY_PUT] = "STAY PUT",
    [TEXT_MOVE] = "MOVE",
};

static const char *MTYPE_STR[] = {
    [MTYPE_ACK] = "ACK",   [MTYPE_HELLO] = "HELLO", [MTYPE_PING] = "PING", [MTYPE_PONG] = "PONG",
    [MTYPE_TEXT] = "TEXT", [MTYPE_REQ] = "REQ",     [MTYPE_RES] = "RES",   [MTYPE_RAW] = "RAW",
};

static const char *INFO_STR[] = {
    [INFO_VERSION] = "VERSION",
    [INFO_BATTERY] = "BATTERY",
    [INFO_UPTIME] = "UPTIME",
    [INFO_CALLSIGN] = "CALLSIGN",
};

static const char *IRQ_STR[] = {
    [SX126X_IRQ_NONE] = "NONE",
    [SX126X_IRQ_TX_DONE] = "TX_DONE",
    [SX126X_IRQ_RX_DONE] = "RX_DONE",
    [SX126X_IRQ_PREAMBLE_DETECTED] = "PREAMBLE_DETECTED",
    [SX126X_IRQ_SYNC_WORD_VALID] = "SYNC_WORD_VALID",
    [SX126X_IRQ_HEADER_VALID] = "HEADER_VALID",
    [SX126X_IRQ_HEADER_ERROR] = "HEADER_ERROR",
    [SX126X_IRQ_CRC_ERROR] = "CRC_ERROR",
    [SX126X_IRQ_CAD_DONE] = "CAD_DONE",
    [SX126X_IRQ_CAD_DETECTED] = "CAD_DETECTED",
    [SX126X_IRQ_TIMEOUT] = "TIMEOUT",
    [SX126X_IRQ_LR_FHSS_HOP] = "LR_FHSS_HOP",
};

#endif // UTILS_H
