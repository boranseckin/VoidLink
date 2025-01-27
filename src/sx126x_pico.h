#ifndef SX126X_PICO_H
#define SX126X_PICO_H

void handle_tx_callback();
void handle_rx_callback();

void handle_dio1_callback(uint gpio, uint32_t events);
void handle_button_callback(uint gpio, uint32_t events);
void handle_irq_callback(uint gpio, uint32_t events);

void setup_io();
void setup_display();
void setup_sx126x();

void transmit_bytes(uint8_t *bytes, uint8_t length);
void transmit_string(char *string);
void transmit_packet(message_t *packet);

void receive_once();
void receive_cont();

#endif // SX126X_PICO_H