#ifndef _NETWORK_H
#define _NETWORK_H

#include <stdbool.h>
#include <stdint.h>

#include "pico/util/queue.h"

// Bump these versions according to the changes made.
#define VERSION_MAJOR 1
#define VERSION_MINOR 0
// Neighbour table
#define MAX_NEIGHBOURS 16
// Maximum number of messages to keep in history.
#define MAX_MESSAGE_HISTORY 16
// Timeout value for non-acked messages.
#define ACK_TIMEOUT 1000 * 30 // 30 seconds
// Number of tries before we give up on the message.
#define ACK_MAX_RETRIES 5
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

// Maximum value for the message id.
#define MAX_MID (1 << (sizeof(mid_t) * 8))

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
  // Indicates how many hops this message can travel (max 7 hops).
  uint8_t hop_limit : 3;
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

// Neighbour information.
typedef struct {
  uid_t uid;
  int8_t rssi;
  uint8_t version_major;
  uint8_t version_minor;
  absolute_time_t last_seen;
} neighbour_t;

// Neighbour table.
typedef struct {
  neighbour_t neighbours[MAX_NEIGHBOURS];
  uint8_t count;
} neighbour_table_t;

extern neighbour_table_t neighbour_table;

// Message structure.
// On 32-bit architecture of pico, the struct needs to be aligned to 4-byte words.
// The reserved data is for future expension.
typedef struct __attribute__((__packed__)) {
  uid_t dst;       // 3
  uid_t src;       // 3 6
  mid_t id;        // 1 7
  mtype_t mtype;   // 1 8
  uint64_t time;   // 8 16
  flags_t flags;   // 1 17
  uint8_t data[3]; // 3 20
} message_t;

typedef struct {
  message_t message;
  absolute_time_t time;
} message_history_t;

// Cyclic buffer of received messages.
extern message_history_t message_history[MAX_MESSAGE_HISTORY];
// Index of the next message to be added.
extern uint8_t message_history_head;
extern uint8_t message_history_count;

// Messages with timeout and retry values for ack.
// Entry is invalid if `timeout` == 0.
typedef struct {
  message_t message;
  absolute_time_t timeout;
  uint8_t retries;
} ack_t;

// Ack list to keep track of messages that need to be acked.
// Each message is indexed by its mid.
extern ack_t ack_list[MAX_MID];

void setup_network();

bool compare_messages(message_t *a, message_t *b);

message_t new_ack_message(uid_t dst, mid_t mid);
message_t new_hello_message();
message_t new_ping_message(uid_t dst);
message_t new_pong_message(uid_t dst);
message_t new_text_message(uid_t dst, text_id_t id);
message_t new_request_message(uid_t dst, info_key_t key);
message_t new_response_message(uid_t dst, info_key_t key, uint16_t value);
message_t new_raw_message(uid_t dst, uint8_t *data[3]);

void update_neighbour(uid_t uid, int8_t rssi, uint16_t version);
void print_neighbours();

bool check_message_history(message_t msg);
void print_message_history();

void add_ack(message_t *message);
void remove_ack(mid_t mid);
void check_ack_list();
void print_acks();

void try_transmit(message_t message);

void handle_message(message_history_t *message);

#endif // _NETWORK_H
