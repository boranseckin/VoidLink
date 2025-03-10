#ifndef _IO_H
#define _IO_H

#include <stdint.h>

#include "pico/stdlib.h"

void handle_button_callback(uint gpio, uint32_t events);

#endif // _IO_H
