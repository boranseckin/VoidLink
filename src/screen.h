#ifndef SCREEN_H
#define SCREEN_H

// Screen state machine
typedef enum {
  SCREEN_IDLE,
  SCREEN_DRAW_READY,
  SCREEN_DRAW,
} screen_t;

static screen_t screen = SCREEN_IDLE;

// display screens state machine
typedef enum {
  DISPLAY_HOME,
  DISPLAY_MSG,
  DISPLAY_RXMSG,
  DISPLAY_RXMSG_DETAILS,
  DISPLAY_NEIGHBOURS,
  DISPLAY_SETTINGS,
  DISPLAY_SETTINGS_INFO,
} display_t;

static display_t display = DISPLAY_HOME;

// ((EPD_2in13_V4_WIDTH % 8 == 0) ? (EPD_2in13_V4_WIDTH / 8) : (EPD_2in13_V4_WIDTH / 8 + 1)) *
// EPD_2in13_V4_HEIGHT = 4080
#define IMAGE_SIZE 4080
static uint8_t image[IMAGE_SIZE];
static uint8_t wakeup[IMAGE_SIZE];

static uint8_t message_Cursor = 0;
static uint8_t home_Cursor = 0;
static uint8_t settings_Cursor = 0;
static uint8_t set_Info_Cursor = 2;
static uint8_t msg_Action_Cursor = 0;
static uint8_t temp_Cursor;
static uint8_t received_Cursor = 0;
static uint8_t received_Page = 1;
static uint8_t refresh_Counter = 0;
static uint32_t display_Timeout = 10000;
volatile bool five_Seconds = true;

// character array to store received messages
char saved_Messages[16][255 + 4];
char test[7];
int new_Messages[16];

// number of messages received
uint32_t msg_Number = 0;
uint32_t new_Msg = 0;

void wakeup_Screen();
void msg_Screen();
void received_msg_Details();
void received_Msgs();
void neighbours_Screen();
void home_Screen();
void settings_Info();
void settings_Screen();

void set_flag_and_reset_alarm();

void screen_draw_loop();

#endif