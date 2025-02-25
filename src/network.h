#ifndef NETWORK_H
#define NETWORK_H

#include <stdbool.h>
#include <stdint.h>

#include "pico/util/queue.h"

// Bump these versions according to the changes made.
#define VERSION_MAJOR 0
#define VERSION_MINOR 1

// Maximum number of messages that can be buffered in the queue (both rx and tx).
#define MESSAGE_QUEUE_SIZE 8
// Outgoing message queue.
extern queue_t tx_queue;
// Incoming message queue.
extern queue_t rx_queue;

// 16 predefined text messages.
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

// Unique identifier for a device.
// Constructed from the last 3 bytes of the unique id of the pico.
typedef struct {
  uint8_t bytes[3];
} uid_t;

uid_t get_uid();
uid_t get_broadcast_uid();

char *uid_to_string(uid_t uid);

bool is_my_uid(uid_t uid);
bool is_broadcast(uid_t uid);

// Message ID.
typedef uint8_t mid_t;

// Message salt.
typedef uint8_t salt_t;

// Message types.
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

// Message flags.
typedef struct {
  // Indicates if an ACK is requested for this message.
  bool ack_req : 1;
  // Indicates how many hops this message can travel.
  bool hop_limit : 1;
} flags_t;

// Information types.
typedef enum __attribute__((__packed__)) {
  INFO_VERSION = 0,
  INFO_BATTERY = 1,
  INFO_UPTIME = 2,
  INFO_CALLSIGN = 3,
} info_key_t;

// Information payload.
typedef struct {
  info_key_t key;
  uint8_t value;
} info_t;

// Message structure.
// On 32-bit arch of pico, this struct takes at least 4 words to allign properly.
// For that reason, 3-bytes of extra data is already wasted so they are reserved for future use.
typedef struct {
  uid_t dst;
  uid_t src;
  mid_t id;
  salt_t salt;
  mtype_t mtype;
  flags_t flags;
  uint8_t reserved[3];
  uint8_t data[3];
} message_t;

void setup_network();

message_t new_ack_message(uid_t dst, mid_t mid);
message_t new_hello_message();
message_t new_ping_message(uid_t dst);
message_t new_pong_message(uid_t dst);
message_t new_text_message(uid_t dst, text_id_t id);
message_t new_request_message(uid_t dst, info_key_t key);
message_t new_response_message(uid_t dst, info_key_t key, uint16_t value);
message_t new_raw_message(uid_t dst, uint8_t *data[3]);

void update_neighbour(uid_t uid, int8_t rssi);
void get_neighbours(char *buffer);

bool check_message_history(message_t msg);
void get_message_history(char *buffer);

void add_ack(message_t *message);
void remove_ack(mid_t mid);
void check_ack_list();
void get_acks(char *buffer);

void try_transmit(message_t message);

void handle_message(message_t *incoming);

#endif // NETWORK_H
