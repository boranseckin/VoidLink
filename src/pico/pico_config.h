#ifndef PICO_CONFIG_H
#define PICO_CONFIG_H

#define UART_PORT uart0

#define SPI_PORT spi0
#define PIN_MISO PICO_DEFAULT_SPI_RX_PIN
#define PIN_MOSI PICO_DEFAULT_SPI_TX_PIN
#define PIN_SCLK PICO_DEFAULT_SPI_SCK_PIN
#define PIN_NSS 17
#define PIN_RESET 20
#define PIN_BUSY 21
#define PIN_DIO1 22

#define PIN_BUTTON_NEXT 5
#define PIN_BUTTON_PREVIOUS 6
#define PIN_BUTTON_OK 4
#define PIN_BUTTON_BACK 3

#endif // PICO_CONFIG_H
