#ifndef _VOIDLINK_H
#define _VOIDLINK_H

#include <stdint.h>

#include "pico/types.h"

#include "network.h"
#include "utils.h"

extern absolute_time_t last_tx_start;
extern absolute_time_t last_tx_delta;

extern bool STOP_PROCESSING;

void set_range(mod_params_t param);
uint32_t get_time_on_air_in_ms();

void handle_tx_callback();
void handle_rx_callback();

void handle_dio1_callback(uint gpio, uint32_t events);
void handle_button_callback(uint gpio, uint32_t events);
void handle_irq_callback(uint gpio, uint32_t events);

void setup_io();
void setup_display();
void setup_sx126x();

float read_voltage();

void transmit_bytes(uint8_t *bytes, uint8_t length);
void transmit_string(char *string);
void transmit_packet(message_history_t *packet);

void receive_once();
void receive_cont();

void core1_entry();

#endif // _VOIDLINK_H
