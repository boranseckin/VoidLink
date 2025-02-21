/**
 * Network Protocol
 *
 * This protocol uses big-endian encoding for multi-byte fields.
 */

#include <stdio.h>
#include <string.h>

#include "pico/time.h"
#include "pico/unique_id.h"
#include "pico/util/queue.h"

#include "network.h"
#include "utils.h"

queue_t tx_queue;
queue_t rx_queue;

// Unique identifier for this device.
// MUST be set in setup_network() before use.
static uid_t MY_UID = {.bytes = {0x00, 0x00, 0x00}};
// Broadcast identifier.
static const uid_t BROADCAST_UID = {.bytes = {0xFF, 0xFF, 0xFF}};

// Setup the network.
void setup_network() {
  // It seems trying to read the unique id too early causes a crash.
  sleep_ms(100);

  // Use last 3 bytes of the unique id as the device id.
  pico_unique_board_id_t board_id;
  pico_get_unique_board_id(&board_id);
  MY_UID.bytes[0] = board_id.id[5];
  MY_UID.bytes[1] = board_id.id[6];
  MY_UID.bytes[2] = board_id.id[7];

  // Setup the queues.
  queue_init(&tx_queue, sizeof(message_t), MESSAGE_QUEUE_SIZE);
  queue_init(&rx_queue, sizeof(message_t), MESSAGE_QUEUE_SIZE);

  debug("network setup done\n");
}

// Returns the unique id of the device.
uid_t get_uid() { return MY_UID; }

// Returns the broadcast id.
uid_t get_broadcast_uid() { return BROADCAST_UID; }

// Converts an uid to a string.
// It uses a shared buffer, so the string is only valid until the next call.
char *uid_to_string(uid_t uid) {
  static char str[9];
  sprintf(str, "%02X:%02X:%02X", uid.bytes[0], uid.bytes[1], uid.bytes[2]);
  return str;
}

// Check if an uid is the same as the device id.
bool is_my_uid(uid_t uid) { return memcmp(&uid, &MY_UID, sizeof(uid_t)) == 0; }
// Check if an uid is the broadcast id.
bool is_broadcast(uid_t uid) { return memcmp(&uid, &BROADCAST_UID, sizeof(uid_t)) == 0; }

// Maximum value for the message id.
#define MAX_MID 16

// Returns the next message id.
// Each call increments the id by one, wrapping around at MAX_MID.
mid_t get_mid() {
  static mid_t mid = 0;
  mid = (mid + 1) % MAX_MID;
  return mid;
}

// Forms a new ack message.
message_t new_ack_message(uid_t dst, mid_t mid) {
  message_t msg = {
      .dst = dst,
      .src = get_uid(),
      .id = get_mid(),
      .mtype = MTYPE_ACK,
      .flags = {.ack_req = false, .hop_limit = false},
      .data = {mid, 0x00, 0x00},
  };
  return msg;
}

// Forms a new hello message.
message_t new_hello_message() {
  message_t msg = {
      .dst = get_broadcast_uid(),
      .src = get_uid(),
      .id = get_mid(),
      .mtype = MTYPE_HELLO,
      .flags = {.ack_req = false, .hop_limit = false},
      .data = {0x00, 0x00, 0x00},
  };
  return msg;
}

// Forms a new ping message.
message_t new_ping_message(uid_t dst) {
  message_t msg = {
      .dst = dst,
      .src = get_uid(),
      .id = get_mid(),
      .mtype = MTYPE_PING,
      .flags = {.ack_req = false, .hop_limit = false},
      .data = {0x00, 0x00, 0x00},
  };
  return msg;
}

// Forms a new pong message.
message_t new_pong_message(uid_t dst) {
  message_t msg = {
      .dst = dst,
      .src = get_uid(),
      .id = get_mid(),
      .mtype = MTYPE_PONG,
      .flags = {.ack_req = false, .hop_limit = false},
      .data = {0x00, 0x00, 0x00},
  };
  return msg;
}

// Forms a new text message.
message_t new_text_message(uid_t dst, text_id_t id) {
  message_t msg = {
      .dst = dst,
      .src = get_uid(),
      .id = get_mid(),
      .mtype = MTYPE_TEXT,
      .flags = {.ack_req = false, .hop_limit = false},
      .data = {id, 0x00, 0x00},
  };
  return msg;
}

// Forms a new request message.
message_t new_request_message(uid_t dst, info_key_t key) {
  message_t msg = {
      .dst = dst,
      .src = get_uid(),
      .id = get_mid(),
      .mtype = MTYPE_REQ,
      .flags = {.ack_req = false, .hop_limit = false},
      .data = {key, 0x00, 0x00},
  };
  return msg;
}

// Forms a new response message.
message_t new_response_message(uid_t dst, info_key_t key, uint16_t value) {
  message_t msg = {
      .dst = dst,
      .src = get_uid(),
      .id = get_mid(),
      .mtype = MTYPE_RES,
      .flags = {.ack_req = false, .hop_limit = false},
      .data = {key, (value >> 8) & 0xFF, (value & 0xFF)},
  };
  return msg;
}

// Forms a new raw message.
message_t new_raw_message(uid_t dst, uint8_t *data[3]) {
  message_t msg = {
      .dst = dst,
      .src = get_uid(),
      .id = get_mid(),
      .mtype = MTYPE_RAW,
      .flags = {.ack_req = false, .hop_limit = false},
  };
  memcpy(msg.data, data, 3);
  return msg;
}

// Neighbour table
#define MAX_NEIGHBOURS 16

// Neighbour information.
// TODO: add fields to save information about the neighbour (e.g. version)
typedef struct {
  uid_t uid;
  int8_t rssi;
  absolute_time_t last_seen;
} neighbour_t;

// Neighbour table.
typedef struct {
  neighbour_t neighbours[MAX_NEIGHBOURS];
  uint8_t count;
} neighbour_table_t;

static neighbour_table_t neighbour_table = {.neighbours = {0}, .count = 0};

// Update (or add) a neighbour to the table.
void update_neighbour(uid_t uid, int8_t rssi) {
  for (int i = 0; i < neighbour_table.count; i++) {
    if (memcmp(&neighbour_table.neighbours[i].uid, &uid, sizeof(uid_t)) == 0) {
      neighbour_table.neighbours[i].rssi = rssi;
      neighbour_table.neighbours[i].last_seen = get_absolute_time();
      debug("neighbour %s updated (rssi: %d, ts: %dms)\n", uid_to_string(uid), rssi,
            to_ms_since_boot(neighbour_table.neighbours[neighbour_table.count - 1].last_seen));
      return;
    }
  }

  if (neighbour_table.count >= MAX_NEIGHBOURS) {
    // TODO: periodic cleanup of the neighbour table
    error("neighbour table full\n");
    return;
  }

  neighbour_table.neighbours[neighbour_table.count].uid = uid;
  neighbour_table.neighbours[neighbour_table.count].rssi = rssi;
  neighbour_table.neighbours[neighbour_table.count].last_seen = get_absolute_time();
  neighbour_table.count++;
  debug("neighbour %s added (rssi: %d, ts: %dms)\n", uid_to_string(uid), rssi,
        to_ms_since_boot(neighbour_table.neighbours[neighbour_table.count - 1].last_seen));
}

// Get the neighbours as a string.
void get_neighbours(char *buffer) {
  for (int i = 0; i < neighbour_table.count; i++) {
    neighbour_t *neighbour = &neighbour_table.neighbours[i];
    char *uid = uid_to_string(neighbour->uid);
    sprintf(buffer, "%s: %d dBm, %d ms\r\n", uid, neighbour->rssi,
            to_ms_since_boot(neighbour->last_seen));
    buffer += strlen(buffer);
  }
}

// Maximum number of messages to keep in history.
#define MAX_MESSAGE_HISTORY 16

// Cyclic buffer of received messages.
static message_t message_history[MAX_MESSAGE_HISTORY] = {0};
// Index of the next message to be added.
static uint8_t message_history_head = 0;

// Check if a message is already received.
// If not, add it to the history.
bool check_message_history(message_t msg) {
  for (int i = 0; i < MAX_MESSAGE_HISTORY; i++) {
    // TODO: consider the case where a node reboots and loses the mid counter
    if (memcmp(&message_history[i].src, &msg.src, sizeof(uid_t)) == 0 &&
        memcmp(&message_history[i].id, &msg.id, sizeof(mid_t)) == 0) {
      debug("message %d from %s already received\n", msg.id, uid_to_string(msg.src));
      return true;
    }
  }

  message_history[message_history_head] = msg;
  message_history_head = (message_history_head + 1) % MAX_MESSAGE_HISTORY;
  debug("message %d from %s added to history\n", msg.id, uid_to_string(msg.src));

  return false;
}

// Get the message history as a string.
void get_message_history(char *buffer) {
  for (int i = 0; i < MAX_MESSAGE_HISTORY; i++) {
    if (message_history[i].src.bytes[0] == 0) {
      continue;
    }
    message_t *msg = &message_history[i];
    char *src = uid_to_string(msg->src);
    sprintf(buffer, "%d: [%s] %d\r\n", i, src, msg->mtype);
    buffer += strlen(buffer);
  }
}

// Messages with timeout values for ack.
// TODO: max_retries
typedef struct {
  message_t message;
  absolute_time_t timeout;
} ack_t;

// Timeout value for non-acked messages.
#define ACK_TIMEOUT 1000 * 60 // 1 minute

// Ack list to keep track of messages that need to be acked.
// Each message is indexed by its mid.
ack_t ack_list[MAX_MID] = {0};

// Add an ack to the list.
void add_ack(message_t *message) {
  ack_t *ack = &ack_list[message->id];
  ack->message = *message;
  ack->timeout = make_timeout_time_ms(ACK_TIMEOUT);
  debug("ack added %d\n", message->id);
}

// Remove an ack from the list, marking it as acked.
void remove_ack(mid_t mid) {
  ack_list[mid].timeout = 0;
  debug("ack removed %d\n", mid);
}

// Check the ack list for timed out messages.
// If an ack timed out, retransmit the corresponding message.
void check_ack_list() {
  for (int i = 0; i < MAX_MID; i++) {
    ack_t *ack = &ack_list[i];
    if (ack->timeout == 0 || ack->timeout > get_absolute_time()) {
      continue;
    }

    // Add the message back to the transmit queue
    debug("ack timeout %d\n", ack->message.id);
    if (queue_try_add(&tx_queue, &ack->message)) {
      debug("tx enqueue (from ack timeout) %d\n", ack->message.id);
      ack->timeout = make_timeout_time_ms(ACK_TIMEOUT);
    } else {
      error("tx queue is full (from ack timeout)\n");
    }
  }
}

// Get the ack list as a string.
void get_acks(char *buffer) {
  for (int i = 0; i < MAX_MID; i++) {
    ack_t *ack = &ack_list[i];
    if (ack->timeout == 0) {
      continue;
    }
    char *dst = uid_to_string(ack->message.dst);
    sprintf(buffer, "%d: [%s] %d (%llu sec)\r\n", i, dst, ack->message.mtype,
            absolute_time_diff_us(get_absolute_time(), ack->timeout) / 1000 / 1000);
    buffer += strlen(buffer);
  }
}

// Try to add a message to the transit queue to be sent.
void try_transmit(message_t message) {
  if (queue_try_add(&tx_queue, &message)) {
    debug("tx enqueue %d\n", message.id);
  } else {
    error("tx queue is full\n");
  }
}

/**
 * Process a received message.
 *
 * ACK: remove related message from the ack list
 * HELLO: send an ACK if requested
 * PING: send a PONG
 * PONG: do nothing
 * TEXT: print the text
 * REQ: send a RES
 * RES: update the neighbour table
 * RAW: do nothing
 */
void handle_message(message_t *incoming) {
  printf("message received from %s", uid_to_string(incoming->src));
  printf(" to %s\n", uid_to_string(incoming->dst));

  if (incoming->mtype == MTYPE_ACK) {
    printf("rx: ack: %d\n", incoming->data[0]);
    mid_t mid = {incoming->data[0]};
    remove_ack(mid);
  } else if (incoming->mtype == MTYPE_HELLO) {
    printf("rx: hello\n");
    if (incoming->flags.ack_req) {
      try_transmit(new_ack_message(incoming->src, incoming->id));
    }
  } else if (incoming->mtype == MTYPE_PING) {
    printf("rx: ping\n");
    try_transmit(new_pong_message(incoming->src));
  } else if (incoming->mtype == MTYPE_PONG) {
    printf("rx: pong\n");
  } else if (incoming->mtype == MTYPE_TEXT) {
    printf("rx: text: %s\n", TEXT_MESSAGE_STR[incoming->data[0]]);
  } else if (incoming->mtype == MTYPE_REQ) {
    printf("rx: request: %d\n", incoming->data[0]);
    if (incoming->data[0] == INFO_VERSION) {
      try_transmit(
          new_response_message(incoming->src, INFO_VERSION, VERSION_MAJOR << 8 | VERSION_MINOR));
    } else if (incoming->data[0] == INFO_UPTIME) {
      try_transmit(
          new_response_message(incoming->src, INFO_UPTIME, to_ms_since_boot(get_absolute_time())));
    } else {
      try_transmit(new_response_message(incoming->src, incoming->data[0], 0));
    }
  } else if (incoming->mtype == MTYPE_RES) {
    printf("rx: response: %d %d %d\n", incoming->data[0], incoming->data[1], incoming->data[2]);
  } else if (incoming->mtype == MTYPE_RAW) {
    printf("rx: raw: %d %d %d\n", incoming->data[0], incoming->data[1], incoming->data[2]);
  }
}
