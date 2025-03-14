#include "hardware/timer.h"
#include "pico/multicore.h"
#include "pico/time.h"

#include "EPD_2in13_V4.h"
#include "GUI_Paint.h"

#include "network.h"
#include "screen.h"
#include "utils.h"

alarm_id_t alarm_id;

screen_t screen = SCREEN_IDLE;
display_t display = DISPLAY_HOME;

uint8_t image[IMAGE_SIZE];
uint8_t wakeup[IMAGE_SIZE];

uint8_t message_Cursor = 0;
uint8_t neighbour_Cursor = 0;
uint8_t home_Cursor = 0;
uint8_t settings_Cursor = 0;
uint8_t set_Info_Cursor = 2;
uint8_t msg_Action_Cursor = 0;
uint8_t temp_Cursor = 0;
uint8_t received_Cursor = 0;
uint8_t received_Page = 1;
uint8_t refresh_Counter = 0;
uint32_t display_Timeout = 10000;
volatile bool five_Seconds = true;

char saved_Messages[16][255 + 4];
char test[7];
int new_Messages[16];

uint32_t msg_Number = 0;
uint32_t new_Msg = 0;

//extern neighbour_table_t neighbour_table;

void setup_display() {
  DEV_Module_Init();
  EPD_2in13_V4_Init();
  EPD_2in13_V4_Clear();
  EPD_2in13_V4_Sleep();

  debug("display setup done\n");
}

void wakeup_Screen() {
  // Create a new display buffer
  Paint_NewImage(wakeup, EPD_2in13_V4_WIDTH, EPD_2in13_V4_HEIGHT, 90, WHITE);
  // Paint the whole frame white
  Paint_Clear(WHITE);

  // Draw message selection screen
  Paint_DrawString(50, 50, "VoidLink", &Font24, BLACK, WHITE);
  EPD_2in13_V4_Init_Fast();
  EPD_2in13_V4_Display_Fast(wakeup);
  busy_wait_ms(200);
}

// Broadcast msg screen
void msg_Screen() {
  // Create a new display buffer
  Paint_NewImage(image, EPD_2in13_V4_WIDTH, EPD_2in13_V4_HEIGHT, 90, WHITE);
  // Paint the whole frame white
  Paint_Clear(WHITE);

  // Draw message selection screen
  Paint_DrawString(0, 10, "Select a message:", &Font16, BLACK, WHITE);
  Paint_DrawString(20, 34, "1. Hello", &Font16, BLACK, WHITE);
  Paint_DrawString(20, 58, "2. Yes", &Font16, BLACK, WHITE);
  Paint_DrawString(20, 82, "3. No", &Font16, BLACK, WHITE);
  // Paint_DrawString(20, 106, "4. On my way!", &Font16, BLACK, WHITE);

  // Display cursor
  Paint_DrawString(0, 34, ">", &Font16, BLACK, WHITE);
  message_Cursor = 0;
}

void received_msg_Details() {
  // Create a new display buffer
  Paint_NewImage(image, EPD_2in13_V4_WIDTH, EPD_2in13_V4_HEIGHT, 90, WHITE);
  // Paint the whole frame white
  Paint_Clear(WHITE);
  // Draw message selection screen
  Paint_DrawString(0, 0, "Message Details:", &Font16, BLACK, WHITE);
  message_t *msg = &message_history[received_Cursor + ((received_Page - 1) * 3)];
  char *src = uid_to_string(msg->src);
  printf("- [%d]: %s %s %d\r\n", received_Cursor + ((received_Page - 1) * 3), src, MTYPE_STR[msg->mtype], msg->id);

  //Paint_DrawString(0, 25, saved_Messages[received_Cursor + ((received_Page - 1) * 3)], &Font16,
                   //BLACK, WHITE); // OLD STATIC TEST
  Paint_DrawString(0, 25, msg->data, &Font16,BLACK, WHITE);

  char from[20];
  sprintf(from, "From: %s",src); //Get node ID from network code
  Paint_DrawString(0, 75, from, &Font12, BLACK, WHITE);
  //Paint_DrawString(0, 75, "From: 11.22.33", &Font12, BLACK, WHITE); // OLD STATIC TEST

  char type[20];
  sprintf(type, "Msg Type: %s",MTYPE_STR[msg->mtype]); //Get node ID from network code
  Paint_DrawString(0, 55, type, &Font12, BLACK, WHITE);

  // sprintf(new_String, "%s mins ago",time_Stamp); //Get time_Stamp from network code
  Paint_DrawString(170, 75, "3 mins ago", &Font12, BLACK, WHITE); // replace with new_String
  if (msg_Action_Cursor == 0) {
    Paint_DrawString(20, 100, "Reply", &Font16, WHITE, BLACK);
    Paint_DrawString(150, 100, "Delete", &Font16, BLACK, WHITE);
  } else if (msg_Action_Cursor == 1) {
    Paint_DrawString(20, 100, "Reply", &Font16, BLACK, WHITE);
    Paint_DrawString(150, 100, "Delete", &Font16, WHITE, BLACK);
  }
}

void received_Msgs() {
  // Create a new display buffer
  Paint_NewImage(image, EPD_2in13_V4_WIDTH, EPD_2in13_V4_HEIGHT, 90, WHITE);
  // Paint the whole frame white
  Paint_Clear(WHITE);

  // Paint_DrawString(0, 34, ">", &Font16, BLACK, WHITE);

  // Draw message selection screen
  Paint_DrawString(0, 10, "Received Messages:", &Font16, BLACK, WHITE);
  printf("msg number: %d\n", msg_Number);
  for (int i = 0; i < 3; i++) {
    // Display messages, sender and time received
    if (i + ((received_Page - 1) * 3) < msg_Number) {
      printf("Writing Message\n");
      message_t *msg = &message_history[i + ((received_Page - 1) * 3)];
      char *src = uid_to_string(msg->src);
      printf("- [%d]: %s %s %d\r\n", i + ((received_Page - 1) * 3), src, MTYPE_STR[msg->mtype], msg->id);
      
      Paint_DrawString(20, 34 + i * 24, saved_Messages[i + ((received_Page - 1) * 3)], &Font16,
                       BLACK, WHITE);
      //get the message history from the network.c file and print it here

      // sprintf(new_String, "From: %s",node_ID); //Get node ID from network code
      Paint_DrawString(20, 47 + i * 24, "From: 11.22.33", &Font12, BLACK,
                       WHITE); // replace with new_String
      // sprintf(new_String, "%s mins ago",time_Stamp); //Get time_Stamp from network code
      Paint_DrawString(130, 47 + i * 24, "3 mins ago", &Font12, BLACK,
                       WHITE); // replace with new_String
    }
    // Clear rest of screen if there aren't enough messages to fill it
    else {
      printf("Clearing Message\n");
      Paint_DrawRectangle(0, 34 + i * 24, 170, 47 + i * 24, WHITE, DOT_PIXEL_2X2, DRAW_FILL_FULL);
    }
    // Remove new message indicator if it was seen
    if (new_Messages[i + ((received_Page - 1) * 3)] == 1) {
      Paint_DrawString(150, 34 + i * 24, "*NEW*", &Font16, BLACK, WHITE);
      printf("New Message: %s\n",saved_Messages[i + ((received_Page - 1) * 3)]);
      if (received_Cursor == i) {
        new_Msg--;
        new_Messages[i + ((received_Page - 1) * 3)] = 0;
      }
    }
    for (int i = 0; i < 3; i++) {
      Paint_ClearWindows(0, 34 + i * 24, 20, 58 + i * 24, WHITE);
    }
    Paint_DrawString(0, 34 + received_Cursor * 24, ">", &Font16, BLACK, WHITE);
  }
  if (msg_Number == 0) {
    Paint_DrawString(0, 34, "No messages received.", &Font16, BLACK, WHITE);
  }
  // Display page indicators
  if (received_Page > 1) {
    Paint_DrawString(100, 30, "^", &Font12, BLACK, WHITE);
  }
  if (received_Page < ((float)msg_Number / 3)) {
    Paint_DrawString(100, 110, "v", &Font12, BLACK, WHITE);
  }
}

void neighbours_Screen() {
  // Create a new display buffer
  Paint_NewImage(image, EPD_2in13_V4_WIDTH, EPD_2in13_V4_HEIGHT, 90, WHITE);
  // Paint the whole frame white
  Paint_Clear(WHITE);

  // Draw message selection screen
  if (neighbour_Cursor == 0) {
    // select next screen
    Paint_DrawRectangle(15, 29, 170, 69, BLACK, DOT_PIXEL_2X2, DRAW_FILL_FULL);
    Paint_DrawString(20, 34, "Neighbours", &Font16, WHITE, WHITE);
    Paint_DrawRectangle(15, 71, 170, 111, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawString(20, 106, "Broadcast", &Font16, BLACK, WHITE);
  }
  if (neighbour_Cursor == 1) {
    // select next screen
    Paint_DrawRectangle(15, 29, 170, 69, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawString(20, 34, "Neighbours", &Font16, BLACK, WHITE);
    Paint_DrawRectangle(15, 71, 170, 111, BLACK, DOT_PIXEL_2X2, DRAW_FILL_FULL);
    Paint_DrawString(20, 106, "Broadcast", &Font16, WHITE, WHITE);
  }
}

void home_Screen() {
  // Create a new display buffer
  Paint_NewImage(image, EPD_2in13_V4_WIDTH, EPD_2in13_V4_HEIGHT, 90,
                 WHITE); // Just do for first time setup
  // Paint_SelectImage(image); //Do this for every other time
  //  Paint the whole frame white
  Paint_Clear(WHITE); // Just do for first time setup
  // SETTINGS SELECTED
  if (home_Cursor == 1) {
    // select next screen
    Paint_Clear(WHITE);
    Paint_DrawRectangle(50, 50, 80, 80, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawString(57, 57, "M", &Font20, BLACK, WHITE);
    Paint_DrawString(37, 85, "Messages", &Font12, WHITE, WHITE);
    Paint_DrawRectangle(150, 50, 180, 80, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawString(157, 57, "N", &Font20, BLACK, WHITE);
    Paint_DrawString(132, 85, "Neighbours", &Font12, WHITE, WHITE);
    Paint_DrawRectangle(100, 50, 130, 80, BLACK, DOT_PIXEL_2X2, DRAW_FILL_FULL);
    Paint_DrawString(107, 57, "S", &Font20, WHITE, WHITE);
    Paint_DrawString(86, 85, "Settings", &Font12, BLACK, WHITE);
  }
  // MESSAGES SELECTED
  if (home_Cursor == 0) {
    // select next screen
    Paint_Clear(WHITE);
    Paint_DrawRectangle(100, 50, 130, 80, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawString(107, 57, "S", &Font20, BLACK, WHITE);
    Paint_DrawString(83, 85, "Settings", &Font12, WHITE, WHITE);
    Paint_DrawRectangle(150, 50, 180, 80, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawString(157, 57, "N", &Font20, BLACK, WHITE);
    Paint_DrawString(132, 85, "Neighbours", &Font12, WHITE, WHITE);
    Paint_DrawRectangle(50, 50, 80, 80, BLACK, DOT_PIXEL_2X2, DRAW_FILL_FULL);
    Paint_DrawString(57, 57, "M", &Font20, WHITE, WHITE);
    Paint_DrawString(37, 85, "Messages", &Font12, BLACK, WHITE);
  }
  // NEIGHBOURS SELECTED
  if (home_Cursor == 2) {
    Paint_Clear(WHITE);
    Paint_DrawRectangle(50, 50, 80, 80, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawString(57, 57, "M", &Font20, BLACK, WHITE);
    Paint_DrawString(37, 85, "Messages", &Font12, WHITE, WHITE);
    Paint_DrawRectangle(100, 50, 130, 80, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawString(107, 57, "S", &Font20, BLACK, WHITE);
    Paint_DrawString(83, 85, "Settings", &Font12, WHITE, WHITE);
    Paint_DrawRectangle(150, 50, 180, 80, BLACK, DOT_PIXEL_2X2, DRAW_FILL_FULL);
    Paint_DrawString(157, 57, "N", &Font20, WHITE, WHITE);
    Paint_DrawString(132, 85, "Neighbours", &Font12, BLACK, WHITE);
  }
  if (new_Msg > 0) {
    if (new_Msg == 1) {
      Paint_DrawString(95, 110, " new message", &Font12, BLACK, WHITE);
    } else {
      Paint_DrawString(95, 110, " new messages", &Font12, BLACK, WHITE);
    }
    Paint_DrawNum(90, 110, new_Msg, &Font12, WHITE, BLACK);
  }

  // Draw version number
  char version[12];
  char neighbour_Count[2];
  sprintf(version, "V/ink v%d.%d", VERSION_MAJOR, VERSION_MINOR);
  Paint_DrawString(0, 0, version, &Font12, BLACK, WHITE);

  // Draw nearby nodes, replace with network code
  Paint_DrawString(125, 0, "Nearby Nodes", &Font12, BLACK, WHITE);
  // convert neighbour_table.count to string
  sprintf(neighbour_Count, "%d", neighbour_table.count);
  Paint_DrawString(110, 0, neighbour_Count, &Font12, BLACK, WHITE);
  //printf("Found Nodes: %d\n",neighbour_table.count);
}

void settings_Info() {
  Paint_SelectImage(image);
  if (settings_Cursor == 0) {
    // clear screen
    Paint_ClearWindows(170, 34, 300, 50, WHITE);
    // refresh display
    EPD_2in13_V4_Display_Partial(image);
    Paint_DrawString(200, 26, "^", &Font12, BLACK, WHITE);
    Paint_DrawString(200, 44, "v", &Font12, BLACK, WHITE);
    if (set_Info_Cursor == 0) {
      Paint_DrawString(170, 34, "  Never", &Font12, BLACK, WHITE);
    } else if (set_Info_Cursor == 1) {
      Paint_DrawString(170, 34, "5 Seconds", &Font12, BLACK, WHITE);
    } else if (set_Info_Cursor == 2) {
      Paint_DrawString(170, 34, "10 Seconds", &Font12, BLACK, WHITE);
    } else if (set_Info_Cursor == 3) {
      Paint_DrawString(170, 34, "30 Seconds", &Font12, BLACK, WHITE);
    } else if (set_Info_Cursor == 4) {
      Paint_DrawString(170, 34, "1 Minute", &Font12, BLACK, WHITE);
    } else if (set_Info_Cursor == 5) {
      Paint_DrawString(170, 34, "2 Minutes", &Font12, BLACK, WHITE);
    }
  }

  if (settings_Cursor == 1) {
    // clear screen
    Paint_ClearWindows(170, 63, 300, 100, WHITE);
    // refresh display
    EPD_2in13_V4_Display_Partial(image);
    Paint_DrawString(200, 55, "^", &Font12, BLACK, WHITE);
    Paint_DrawString(200, 73, "v", &Font12, BLACK, WHITE);
    if (set_Info_Cursor == 0) {
      Paint_DrawString(170, 63, " Default", &Font12, BLACK, WHITE);
    } else if (set_Info_Cursor == 1) {
      Paint_DrawString(170, 63, "   Fast", &Font12, BLACK, WHITE);
    } else if (set_Info_Cursor>1) {
      Paint_DrawString(170, 63, "Long Range", &Font12, BLACK, WHITE);
      set_Info_Cursor = 5;
    }
  }
}

void settings_Screen() {
  // Create a new display buffer
  Paint_NewImage(image, EPD_2in13_V4_WIDTH, EPD_2in13_V4_HEIGHT, 90, WHITE);
  // Paint the whole frame white
  Paint_Clear(WHITE);

  // Draw message selection screen
  Paint_DrawString(0, 5, "Settings:", &Font16, BLACK, WHITE);

  if (settings_Cursor == 0) {
    Paint_DrawRectangle(5, 29, 155, 55, BLACK, DOT_PIXEL_2X2, DRAW_FILL_FULL);
    Paint_DrawString(10, 34, "Sleep Timeout", &Font16, WHITE, WHITE);
    Paint_DrawRectangle(5, 58, 155, 84, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawString(10, 63, "Mode", &Font16, BLACK, WHITE);
  }

  if (settings_Cursor == 1) {
    Paint_DrawRectangle(5, 29, 155, 55, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawString(10, 34, "Sleep Timeout", &Font16, BLACK, WHITE);
    Paint_DrawRectangle(5, 58, 155, 84, BLACK, DOT_PIXEL_2X2, DRAW_FILL_FULL);
    Paint_DrawString(10, 63, "Mode", &Font16, WHITE, WHITE);
  }
  // Paint_DrawString(5, 82, "Mode", &Font16, BLACK, WHITE);

  // Display cursor
  // Paint_DrawString(0, 34, ">", &Font16, BLACK, WHITE);
}

int64_t alarm_callback(alarm_id_t id, void *user_data) {
  five_Seconds = true;
  printf("%d Seconds of inactivity, sleeping display.\n", display_Timeout / 1000);
  Paint_SelectImage(image);
  // partial display refresh
  Paint_DrawString(225, 0, "SLP", &Font12, WHITE, BLACK);
  EPD_2in13_V4_Display_Partial(image);
  busy_wait_ms(100);
  EPD_2in13_V4_Sleep();
  busy_wait_ms(100);
  return 0; // Returning 0 cancels the alarm
}

// Function to reset the alarm when the flag is set
void set_flag_and_reset_alarm() {
  printf("Flag set! Resetting alarm.\n");
  // Cancel the previous alarm
  cancel_alarm(alarm_id);
  // Reset flag and start a new alarm
  five_Seconds = false;
  alarm_id = add_alarm_in_ms(display_Timeout, alarm_callback, NULL, false);
}

void screen_draw_loop() {
  while (true) {
    uint32_t flag = multicore_fifo_pop_blocking();

    printf("five_Seconds: %d\n", five_Seconds);

    if (flag == 0) {
      // Fast refresh on display wakup
      if (five_Seconds) {
        printf("Waking display.\n");
        EPD_2in13_V4_Init_Fast();
        Paint_ClearWindows(250, 0, 350, 20, WHITE);
        EPD_2in13_V4_Display_Fast(image);
      }
      if (refresh_Counter == 15) {
        EPD_2in13_V4_Display_Base(image);
        refresh_Counter = 0;
      } else {
        EPD_2in13_V4_Display_Partial(image);
        refresh_Counter++;
      }
      // printf("Updating Image\n");
      sleep_ms(100);
      //busy_wait_ms(100);
      // Set alarm to sleep display after x seconds of inactivity
      set_flag_and_reset_alarm();
      multicore_fifo_push_blocking_inline(0);
    }
  }
}
