/**
 * Network Protocol
 *
 * This protocol uses big-endian encoding for multi-byte fields.
 */

#include <pico/time.h>
#include <stdio.h>
#include <string.h>

#include "hardware/timer.h"
#include "pico/stdlib.h"

#include "network.h"
#include "utils.h"

uid_t MY_UID = {.bytes = {0x00, 0x00, 0x00}};
uid_t BROADCAST_UID = {.bytes = {0xFF, 0xFF, 0xFF}};

// Returns the unique id of the device.
uid_t get_uid() { return MY_UID; }

// Returns the broadcast id.
uid_t get_broadcast_uid() { return BROADCAST_UID; }

char *uid_to_string(uid_t uid) {
  static char str[9];
  sprintf(str, "%02X:%02X:%02X", uid.bytes[0], uid.bytes[1], uid.bytes[2]);
  return str;
}

bool is_my_uid(uid_t uid) { return memcmp(&uid, &MY_UID, sizeof(uid_t)) == 0; }
bool is_broadcast(uid_t uid) { return memcmp(&uid, &BROADCAST_UID, sizeof(uid_t)) == 0; }

// Returns the next message id.
mid_t get_mid() {
  mid_t ret = mid;
  mid.mid = (mid.mid + 1) % 255;
  return ret;
}

// Form a new ack message.
message_t new_ack_message(uid_t dst, mid_t mid) {
  message_t msg = {
      .dst = dst,
      .src = get_uid(),
      .id = get_mid(),
      .mtype = MTYPE_ACK,
      .flags = {.ack_req = false, .hop_limit = false},
      .data = {mid.mid, 0x00, 0x00},
  };
  return msg;
}

// Form a new hello message.
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

// Form a new ping message.
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

// Form a new pong message.
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

// Form a new text message.
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

// Form a new request message.
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

// Form a new response message.
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

// Form a new raw message.
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

// TODO: periodic cleanup of the neighbour table

// Check if a message is already received.
bool check_message_history(message_t msg) {
  for (int i = 0; i < MAX_MESSAGE_HISTORY; i++) {
    // TODO: consider the case where a node reboots and loses the mid counter
    if (memcmp(&message_history[i].src, &msg.src, sizeof(uid_t)) == 0 &&
        memcmp(&message_history[i].id, &msg.id, sizeof(mid_t)) == 0) {
      debug("message %d from %s already received\n", msg.id.mid, uid_to_string(msg.src));
      return true;
    }
  }

  message_history[message_history_head] = msg;
  message_history_head = (message_history_head + 1) % MAX_MESSAGE_HISTORY;
  debug("message %d from %s added to history\n", msg.id.mid, uid_to_string(msg.src));

  return false;
}
