/**
 * Network Protocol
 *
 * This protocol uses big-endian encoding for multi-byte fields.
 */

#include <stdio.h>
#include <string.h>

#include "pico/rand.h"
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
// Set neighbour table to empty.
neighbour_table_t neighbour_table = {.neighbours = {0}, .count = 0};

static mid_t MID = 0;

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

  // Set starting message id to a random value.
  // This is done to reduce the risk of getting our message ignored due to resetting the mid counter
  // back to 0 on restart. There is still a chance to randomly pick the same number where we left
  // off but this good tradeoff to keep the message small.
  MID = get_rand_32() & 0xFF;

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

// Returns the next message id.
// Each call increments the id by one, wrapping around at MAX_MID.
mid_t get_mid() {
  MID = (MID + 1) % MAX_MID;
  return MID;
}

// Compare two messages.
// Returns true if they are the same.
bool compare_messages(message_t *a, message_t *b) {
  return memcmp(&(a->src), &(b->src), sizeof(uid_t)) && a->id == b->id;
}

// Forms a new ack message.
message_t new_ack_message(uid_t dst, mid_t mid) {
  message_t msg = {
      .dst = dst,
      .src = get_uid(),
      .id = get_mid(),
      .mtype = MTYPE_ACK,
      .flags = {.ack_req = false, .hop_limit = 0},
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
      .flags = {.ack_req = false, .hop_limit = 0},
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
      .flags = {.ack_req = false, .hop_limit = 3},
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
      .flags = {.ack_req = false, .hop_limit = 0},
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
      .flags = {.ack_req = false, .hop_limit = 0},
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
      .flags = {.ack_req = false, .hop_limit = 0},
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
      .flags = {.ack_req = false, .hop_limit = 0},
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
      .flags = {.ack_req = false, .hop_limit = 0},
  };
  memcpy(msg.data, data, 3);
  return msg;
}

// Update (or add) a neighbour to the table.
void update_neighbour(uid_t uid, int8_t rssi, uint16_t version) {
  // Check if we can find the neighbour in the list
  int8_t index = -1;
  for (int i = 0; i < neighbour_table.count; i++) {
    if (memcmp(&neighbour_table.neighbours[i].uid, &uid, sizeof(uid_t)) == 0) {
      index = i;
      break;
    }
  }

  // If not found, try to add a new entry
  if (index == -1) {
    // Check if we have space for a new neighbour
    if (neighbour_table.count >= MAX_NEIGHBOURS) {
      // TODO: periodic cleanup of the neighbour table
      error("neighbour table full\n");
      return;
    }

    index = neighbour_table.count++;
  }

  neighbour_table.neighbours[index].uid = uid;
  if (rssi != 0) {
    neighbour_table.neighbours[index].rssi = rssi;
  }
  if (version != 0) {
    neighbour_table.neighbours[index].version_major = version >> 8;
    neighbour_table.neighbours[index].version_minor = version & 0xFF;
  }
  neighbour_table.neighbours[index].last_seen = get_absolute_time();

  debug("neighbour %s updated (rssi: %d, ts: %dms, vs: %d.%d)\n", uid_to_string(uid), rssi,
        to_ms_since_boot(neighbour_table.neighbours[index].last_seen), version >> 8,
        version & 0xFF);
}

// Print the neighbours list.
void print_neighbours() {
  for (int i = 0; i < neighbour_table.count; i++) {
    neighbour_t *neighbour = &neighbour_table.neighbours[i];
    if (neighbour->uid.bytes[0] == 0 && neighbour->uid.bytes[1] == 0 &&
        neighbour->uid.bytes[2] == 0) {
      continue;
    }
    char *uid = uid_to_string(neighbour->uid);
    printf("- [%s]: %ddBm, %dms, v%d.%d\r\n", uid, neighbour->rssi,
           to_ms_since_boot(neighbour->last_seen), neighbour->version_major,
           neighbour->version_minor);
  }
}

// Cyclic buffer of received messages.
message_history_t message_history[MAX_MESSAGE_HISTORY] = {0};
// Index of the next message to be added.
uint8_t message_history_head = 0;

// Check if a message is already received.
// If not, add it to the history.
bool check_message_history(message_t msg) {
  for (int i = 0; i < MAX_MESSAGE_HISTORY; i++) {
    if (compare_messages(&message_history[i].message, &msg)) {
      debug("message %d from %s already received\n", msg.id, uid_to_string(msg.src));
      return true;
    }
  }

  message_history[message_history_head].message = msg;
  message_history[message_history_head].time = get_absolute_time();
  message_history_head = (message_history_head + 1) % MAX_MESSAGE_HISTORY;
  debug("message %d from %s added to history\n", msg.id, uid_to_string(msg.src));

  return false;
}

// Print the message history.
void print_message_history() {
  for (int i = 0; i < MAX_MESSAGE_HISTORY; i++) {
    if (message_history[i].message.src.bytes[0] == 0) {
      continue;
    }
    message_t *msg = &message_history[i].message;
    char *src = uid_to_string(msg->src);
    printf("- [%d]: %s %s %d (%ds)\r\n", i, src, MTYPE_STR[msg->mtype], msg->id,
           to_ms_since_boot(message_history[i].time) / 1000);
  }
}

// Ack list to keep track of messages that need to be acked.
// Each message is indexed by its mid.
// TODO: probably use a linked list here
ack_t ack_list[MAX_MID] = {0};

// Add an ack to the list.
void add_ack(message_t *message) {
  ack_t *ack = &ack_list[message->id];

  // Check if this is a new ack entry.
  if (ack->timeout == 0) {
    ack->retries = ACK_MAX_RETRIES;
  } else {
    ack->retries--;
  }

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
    debug("ack timeout %d (%d retries left)\n", ack->message.id, ack->retries);

    if (ack->retries < 1) {
      debug("maximum number of retries (%d) reached for ack, dropping", ACK_MAX_RETRIES);
      remove_ack(ack->message.id);
      continue;
    }

    if (queue_try_add(&tx_queue, &ack->message)) {
      debug("tx enqueue (from ack timeout) %d\n", ack->message.id);
    } else {
      error("tx queue is full (from ack timeout)\n");
    }
  }
}

// Print the ack list.
void print_acks() {
  for (int i = 0; i < MAX_MID; i++) {
    ack_t *ack = &ack_list[i];
    if (ack->timeout == 0) {
      continue;
    }
    char *dst = uid_to_string(ack->message.dst);
    printf("- [%d]: %s %s (%d/%d retries | %llusec)\r\n", i, dst, MTYPE_STR[ack->message.mtype],
           ack->retries, ACK_MAX_RETRIES,
           absolute_time_diff_us(get_absolute_time(), ack->timeout) / 1000 / 1000);
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
      // Convert time in milliseconds to seconds.
      uint32_t time_in_sec = to_ms_since_boot(get_absolute_time()) / 1000;
      // Only send the lower 16 bits to save space.
      try_transmit(new_response_message(incoming->src, INFO_UPTIME, time_in_sec & 0xFFFF));
    } else {
      try_transmit(new_response_message(incoming->src, incoming->data[0], 0));
    }
  } else if (incoming->mtype == MTYPE_RES) {
    printf("rx: response: %d %d %d\n", incoming->data[0], incoming->data[1], incoming->data[2]);
    if (incoming->data[0] == INFO_VERSION) {
      printf("version: %d.%d\n", incoming->data[1], incoming->data[2]);
      update_neighbour(incoming->src, 0, (incoming->data[1] << 8) | incoming->data[2]);
    } else if (incoming->data[0] == INFO_UPTIME) {
      printf("uptime: %d\n", (incoming->data[1] << 8) | incoming->data[2]);
    }
  } else if (incoming->mtype == MTYPE_RAW) {
    printf("rx: raw: %d %d %d\n", incoming->data[0], incoming->data[1], incoming->data[2]);
  }
}
