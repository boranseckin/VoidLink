#ifndef NETWORK_H
#define NETWORK_H

#include <stdbool.h>
#include <stdint.h>

#include <hardware/timer.h>

// Bump these versions according to the changes made.
#define VERSION_MAJOR 0
#define VERSION_MINOR 1

// 16 predefined text messages
static const char *text[] = {
    "OK",   "NO",     "Over",         "Out", "Go ahead",       "Stand-by",      "Come in",
    "Copy", "Repeat", "Break, break", "SOS", "Good reception", "Bad reception", "Stay put",
    "Move",
};

typedef enum __attribute__((__packed__)) {
  TEXT_OK = 0,
  TEXT_NO = 1,
  TEXT_OVER = 2,
  TEXT_OUT = 3,
  TEXT_GO_AHEAD = 4,
  TEXT_STAND_BY = 5,
  TEXT_COME_IN = 6,
  TEXT_COPY = 7,
  TEXT_REPEAT = 8,
  TEXT_BREAK_BREAK = 9,
  TEXT_SOS = 10,
  TEXT_GOOD_RECEPTION = 11,
  TEXT_BAD_RECEPTION = 12,
  TEXT_STAY_PUT = 13,
  TEXT_MOVE = 14,
} text_id_t;

// uid 24 bit int
typedef struct {
  uint8_t bytes[3];
} uid_t;

// must call setup first
extern uid_t MY_UID;
extern uid_t BROADCAST_UID;

uid_t get_uid();
uid_t get_broadcast_uid();

char *uid_to_string(uid_t uid);

bool is_my_uid(uid_t uid);
bool is_broadcast(uid_t uid);

// mid 4 bit int
typedef struct {
  uint8_t mid;
} mid_t;

static mid_t mid = {.mid = 0};

// Returns the next message id.
mid_t get_mid();

// message type
typedef enum __attribute__((__packed__)) {
  MTYPE_ACK = 0,
  MTYPE_HELLO = 1,
  MTYPE_PING = 2,
  MTYPE_PONG = 3,
  MTYPE_TEXT = 4,
  MTYPE_REQ = 5,
  MTYPE_RES = 6,
  MTYPE_RAW = 7,
} mtype_t;

// flags
typedef struct {
  bool ack_req : 1;
  bool hop_limit : 1;
} flags_t;

typedef enum __attribute__((__packed__)) {
  INFO_VERSION = 0,
  INFO_BATTERY = 1,
  INFO_UPTIME = 2,
  INFO_CALLSIGN = 3,
} info_key_t;

// inforamtion
typedef struct {
  info_key_t key;
  uint8_t value;
} info_t;

// message
typedef struct {
  uid_t dst;
  uid_t src;
  mid_t id;
  mtype_t mtype;
  flags_t flags;
  uint8_t data[3];
} message_t;

message_t new_ack_message(uid_t dst, mid_t mid);
message_t new_hello_message();
message_t new_ping_message(uid_t dst);
message_t new_pong_message(uid_t dst);
message_t new_text_message(uid_t dst, text_id_t id);
message_t new_request_message(uid_t dst, info_key_t key);
message_t new_response_message(uid_t dst, info_key_t key, uint16_t value);
message_t new_raw_message(uid_t dst, uint8_t *data[3]);

// Neighbour table
#define MAX_NEIGHBOURS 16

typedef struct {
  uid_t uid;
  int8_t rssi;
  absolute_time_t last_seen;
} neighbour_t;

typedef struct {
  neighbour_t neighbours[MAX_NEIGHBOURS];
  uint8_t count;
} neighbour_table_t;

static neighbour_table_t neighbour_table = {.neighbours = {0}, .count = 0};

// Update (or add) a neighbour to the table.
void update_neighbour(uid_t uid, int8_t rssi);

// Keep track of received message ids and src.
#define MAX_MESSAGE_HISTORY 16

// TODO: handle reboot and mid resets, maybe add salt to message

// cyclic buffer of received messages
static message_t message_history[MAX_MESSAGE_HISTORY] = {0};
static uint8_t message_history_head = 0;

// Check if a message is already received.
bool check_message_history(message_t msg);

#endif // NETWORK_H
