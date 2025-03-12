#ifndef _SCREEN_H
#define _SCREEN_H

#include <stdbool.h>
#include <stdint.h>
#include "network.h"

// Screen state machine
typedef enum {
  SCREEN_IDLE,
  SCREEN_DRAW_READY,
  SCREEN_DRAW,
} screen_t;

extern screen_t screen;

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

extern display_t display;

// ((EPD_2in13_V4_WIDTH % 8 == 0) ? (EPD_2in13_V4_WIDTH / 8) : (EPD_2in13_V4_WIDTH / 8 + 1)) *
// EPD_2in13_V4_HEIGHT = 4080
#define IMAGE_SIZE 4080
extern uint8_t image[IMAGE_SIZE];
extern uint8_t wakeup[IMAGE_SIZE];

extern uint8_t message_Cursor;
extern uint8_t neighbour_Cursor;
extern uint8_t home_Cursor;
extern uint8_t settings_Cursor;
extern uint8_t set_Info_Cursor;
extern uint8_t msg_Action_Cursor;
extern uint8_t temp_Cursor;
extern uint8_t received_Cursor;
extern uint8_t received_Page;
extern uint8_t refresh_Counter;
extern uint32_t display_Timeout;
extern volatile bool five_Seconds;

// character array to store received messages
extern char saved_Messages[16][255 + 4];
extern char test[7];
extern int new_Messages[16];

// number of messages received
extern uint32_t msg_Number;
extern uint32_t new_Msg;

// Bring in neighbour table
extern neighbour_table_t neighbour_table;

void setup_display();

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

#endif // _SCREEN_H
