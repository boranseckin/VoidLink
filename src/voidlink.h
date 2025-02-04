#ifndef VOIDLINK_H
#define VOIDLINK_H

void handle_message(message_t *incoming);

void handle_tx_callback();
void handle_rx_callback();

void handle_dio1_callback(uint gpio, uint32_t events);
void handle_button_callback(uint gpio, uint32_t events);
void handle_irq_callback(uint gpio, uint32_t events);

void setup_io();
void setup_display();
void setup_sx126x();
void setup_network();

void transmit_bytes(uint8_t *bytes, uint8_t length);
void transmit_string(char *string);
void transmit_packet(message_t *packet);

void receive_once();
void receive_cont();

void core1_entry();

#endif // VOIDLINK_H