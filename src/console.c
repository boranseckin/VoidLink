#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "console.h"
#include "network.h"
#include "utils.h"
#include "voidlink.h"

char console_buffer[CONSOLE_BUFFER_SIZE];
uint8_t console_buffer_offset;

// Split a message into parts.
void parse_message(char **parts, char *message) {
  uint8_t i = 0;
  parts[i] = strtok(message, " ");
  while (parts[i] != NULL) {
    parts[++i] = strtok(NULL, " ");
  }
}

// Parse a uid from a string.
void parse_uid(uid_t *uid, char *string) {
  sscanf(string, "%02hhx:%02hhx:%02hhx", &uid->bytes[0], &uid->bytes[1], &uid->bytes[2]);
}

// Try to parse a uid from a string or return broadcast id.
void parse_uid_or_broadcast(uid_t *uid, char *string) {
  if (string == NULL || strcmp(string, "broadcast") == 0) {
    *uid = get_broadcast_uid();
  } else {
    parse_uid(uid, string);
  }
}

// Handle the console input.
void handle_console_input() {
  char *parts[3];
  parse_message(parts, console_buffer);

  if (strcmp(parts[0], "hello") == 0) {
    message_t hello = new_hello_message();
    if (strcmp(parts[1], "ack") == 0) {
      hello.flags.ack_req = true;
    }
    try_transmit(hello);

  } else if (strcmp(parts[0], "ping") == 0) {
    uid_t dst;
    parse_uid_or_broadcast(&dst, parts[1]);
    try_transmit(new_ping_message(dst));

  } else if (strcmp(parts[0], "text") == 0) {
    if (parts[1] == NULL) {
      error("text message requires an id\n");
      return;
    }
    text_id_t id = atoi(parts[1]);
    uid_t dst;
    parse_uid_or_broadcast(&dst, parts[2]);
    try_transmit(new_text_message(dst, id));

  } else if (strcmp(parts[0], "request") == 0) {
    if (parts[1] == NULL) {
      error("request message requires a key\n");
      return;
    }
    info_key_t key = atoi(parts[1]);
    uid_t dst;
    parse_uid_or_broadcast(&dst, parts[2]);
    try_transmit(new_request_message(dst, key));

  } else if (strcmp(parts[0], "set") == 0) {
    if (strcmp(parts[1], "stop") == 0) {
      if (strcmp(parts[2], "true") == 0) {
        STOP_PROCESSING = true;
      } else if (strcmp(parts[2], "false") == 0) {
        STOP_PROCESSING = false;
      } else {
        error("set stop requires a boolean value\n");
      }
    } else if (strcmp(parts[1], "range") == 0) {
      if (strcmp(parts[2], "default") == 0) {
        set_range(DEFAULT);
      } else if (strcmp(parts[2], "fast") == 0) {
        set_range(FAST);
      } else if (strcmp(parts[2], "longrange") == 0) {
        set_range(LONGRANGE);
      } else {
        error("set range requires a valid range\n");
      }
    } else {
      error("unknown set command\n");
    }

  } else if (strcmp(parts[0], "get") == 0) {
    if (strcmp(parts[1], "messages") == 0) {
      print_message_history();
    } else if (strcmp(parts[1], "neighbours") == 0) {
      print_neighbours();
    } else if (strcmp(parts[1], "acks") == 0) {
      print_acks();
    } else if (strcmp(parts[1], "uptime") == 0) {
      info("uptime: %ds\n", to_ms_since_boot(get_absolute_time()) / 1000);
    } else {
      error("unknown get command\n");
    }

  } else {
    error("unknown command\n");
  }
}
