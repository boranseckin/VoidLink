#include <stdio.h>
#include <stdlib.h>

#include "hardware/flash.h"

// 16 predefined text messages
const char *text[] = {
    "OK",   "NO",     "Over",         "Out", "Go ahead",       "Stand-by",      "Come in",
    "Copy", "Repeat", "Break, break", "SOS", "Good reception", "Bad reception", "Stay put",
    "Move",
};

typedef enum {
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
static uid_t MY_UID = {.bytes = {0x00, 0x00, 0x00}};
static uid_t BROADCAST_UID = {.bytes = {0xFF, 0xFF, 0xFF}};

// Returns the unique id of the device.
uid_t get_uid() { return MY_UID; }

// Returns the broadcast id.
uid_t get_broadcast_uid() { return BROADCAST_UID; }

char *uid_to_string(uid_t uid) {
  static char str[9];
  sprintf(str, "%02X:%02X:%02X", uid.bytes[0], uid.bytes[1], uid.bytes[2]);
  return str;
}

// mid 4 bit int
typedef struct {
  uint8_t mid;
} mid_t;

static mid_t mid = {.mid = 0};

// Returns the next message id.
mid_t get_mid() {
  mid_t ret = mid;
  mid.mid = (mid.mid + 1) % 16;
  return ret;
}

// message type
typedef enum {
  MTYPE_ACK = 0,
  MTYPE_PING = 1,
  MTYPE_PONG = 2,
  MTYPE_TEXT = 3,
  MTYPE_REQ = 4,
  MTYPE_RES = 5,
  MTYPE_RAW = 6,
} mtype_t;

// flags
typedef struct {
  bool ack_req;
  bool hop_limit;
} flags_t;

typedef enum {
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
  uint8_t data[2];
} message_t;

// Form a new ack message.
message_t new_ack_message(uid_t dst, mid_t mid) {
  message_t msg = {
      .dst = dst,
      .src = get_uid(),
      .id = get_mid(),
      .mtype = MTYPE_ACK,
      .data = {mid.mid, 0x00},
  };
}

// Form a new ping message.
message_t new_ping_message(uid_t dst) {
  message_t msg = {
      .dst = dst,
      .src = get_uid(),
      .id = get_mid(),
      .mtype = MTYPE_PING,
      .data = {0x00, 0x00},
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
      .data = {0x00, 0x00},
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
      .data = {id, 0x00},
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
      .data = {key, 0x00},
  };
  return msg;
}

// Form a new response message.
message_t new_response_message(uid_t dst, info_key_t key, uint8_t value) {
  message_t msg = {
      .dst = dst,
      .src = get_uid(),
      .id = get_mid(),
      .mtype = MTYPE_RES,
      .data = {key, value},
  };
  return msg;
}

// Form a new raw message.
message_t new_raw_message(uid_t dst, uint8_t *data[2]) {
  message_t msg = {
      .dst = dst,
      .src = get_uid(),
      .id = get_mid(),
      .mtype = MTYPE_RAW,
  };
  memcpy(msg.data, data, 2);
  return msg;
}

void setup_network() {
  uint8_t bytes[8] = {0};
  flash_get_unique_id(bytes);
  MY_UID.bytes[0] = bytes[5];
  MY_UID.bytes[1] = bytes[6];
  MY_UID.bytes[2] = bytes[7];
}