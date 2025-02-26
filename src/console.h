#ifndef _CONSOLE_H
#define _CONSOLE_H

#include <stdint.h>

#include "network.h"

#define CONSOLE_BUFFER_SIZE 128

extern char console_buffer[CONSOLE_BUFFER_SIZE];
extern uint8_t console_buffer_offset;

void parse_message(char **parts, char *message);
void parse_uid(uid_t *uid, char *string);
void parse_uid_or_broadcast(uid_t *uid, char *string);
void handle_console_input();

#endif
