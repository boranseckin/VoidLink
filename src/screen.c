#include "hardware/timer.h"
#include "pico/multicore.h"
#include "pico/time.h"

#include "EPD_2in13_V4.h"
#include "GUI_Paint.h"
#include "pico_config.h"

#include "network.h"
#include "screen.h"
#include "utils.h"
#include "voidlink.h"

alarm_id_t alarm_id;

screen_t screen = SCREEN_IDLE;
display_t display = DISPLAY_HOME;

uint8_t image[IMAGE_SIZE];
uint8_t wakeup[IMAGE_SIZE];

uint8_t message_Cursor = 0;
uint8_t send_to_Cursor = 0;
uint8_t neighbour_Cursor = 0;
uint8_t neighbour_Table_Cursor = 0;
uint8_t broadcast_Action_Cursor = 0;
uint8_t neighbour_Action_Cursor = 0;
uint8_t neighbour_Request_Cursor = 0;
uint8_t home_Cursor = 0;
uint8_t settings_Cursor = 0;
uint8_t set_Info_Cursor = 2;
uint8_t msg_Action_Cursor = 0;
uint8_t temp_Cursor = 0;
uint8_t received_Cursor = 0;
uint8_t received_Page = 1;
uint8_t msg_received_Page = 1;
uint8_t neighbour_received_Page = 1;
uint8_t refresh_Counter = 0;
uint32_t display_Timeout = 10000;
volatile bool five_Seconds = true;
uint8_t msg_Type = 0;

//char send_Message[16][255 + 4];
// add strings to send_Message
char send_Message[MAX_MSG_SEND][255 + 4] = {"Ok", "No", "Over", "Out", "Go Ahead", "Stand By", "Come In", "Copy", "Repeat", "Break Break", "SOS", "Good Reception", "Bad Reception", "Stay Put", "Move"};
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
// Who will you send to?
void send_To_Screen(){
  // Create a new display buffer
  Paint_NewImage(image, EPD_2in13_V4_WIDTH, EPD_2in13_V4_HEIGHT, 90, WHITE);
  // Paint the whole frame white
  Paint_Clear(WHITE);

  // Draw message selection screen
  if (msg_Type == 0){ //msg_Type = 0 is text message
    Paint_DrawString(15, 35, "Who to send to?", &Font16, BLACK, WHITE);
    Paint_DrawString(5, 5, "Sending Message:", &Font16, BLACK, WHITE);
    Paint_DrawString(180, 5, send_Message[send_to_Cursor + ((msg_received_Page - 1) * 3)], &Font16,
                         BLACK, WHITE);
  } else if (msg_Type == 1){ //msg_Type = 1 is request message
    Paint_DrawString(15, 35, "Who to request from?", &Font16, BLACK, WHITE);
    Paint_DrawString(0, 5, "Requesting Info:", &Font16, BLACK, WHITE);
    if (neighbour_Request_Cursor == 0) {
      Paint_DrawString(180, 5, "Version", &Font16,
                         BLACK, WHITE);
    } else if (neighbour_Request_Cursor == 1) {
      Paint_DrawString(180, 5, "Uptime", &Font16,
                         BLACK, WHITE);
    }
  } else if (msg_Type == 2){ //msg_Type = 2 is ping message
    Paint_DrawString(20, 35, "Who do you want to ping?", &Font12, BLACK, WHITE);
    Paint_DrawString(0, 5, "Sending ping.", &Font16, BLACK, WHITE);
  }
  if (neighbour_table.count>0){
    Paint_DrawString(190, 70, ">", &Font16, BLACK, WHITE);
    Paint_DrawString(20, 70, "<", &Font16, BLACK, WHITE);
  }
  char *src;

  if (send_to_Cursor == 0) {
    Paint_DrawString(45, 70, "Broadcast to all", &Font16, WHITE, BLACK);
  } else {
    neighbour_t *neighbour_Node = &neighbour_table.neighbours[send_to_Cursor - 1];
    src = uid_to_string(neighbour_Node->uid);
    Paint_DrawString(45, 70, src, &Font16, BLACK, WHITE);
  }
  Paint_DrawString(10, 105, "Press the OK button to transmit.", &Font12, BLACK, WHITE);
}

void msg_Screen() {
  // Create a new display buffer
  Paint_NewImage(image, EPD_2in13_V4_WIDTH, EPD_2in13_V4_HEIGHT, 90, WHITE);
  // Paint the whole frame white
  Paint_Clear(WHITE);

  // Draw message selection screen
  for (int i = 0; i < 3; i++) {
    // Display messages, sender and time received
    if (i + ((msg_received_Page - 1) * 3) < MAX_MSG_SEND) {
      Paint_DrawString(20, 34 + i * 24, send_Message[i + ((msg_received_Page - 1) * 3)], &Font16,
                       BLACK, WHITE);
    }
    // Clear rest of screen if there aren't enough messages to fill it
    else {
      printf("Clearing Message\n");
      Paint_DrawRectangle(0, 34 + i * 24, 170, 47 + i * 24, WHITE, DOT_PIXEL_2X2, DRAW_FILL_FULL);
    }
    for (int i = 0; i < 3; i++) {
      Paint_ClearWindows(0, 34 + i * 24, 20, 58 + i * 24, WHITE);
    }
    Paint_DrawString(0, 34 + message_Cursor * 24, ">", &Font16, BLACK, WHITE);
  }
  // Display page indicators
  if (msg_received_Page > 1) {
    Paint_DrawString(100, 30, "^", &Font12, BLACK, WHITE);
  }
  if (msg_received_Page < ((float)MAX_MSG_SEND / 3)) {
    Paint_DrawString(100, 110, "v", &Font12, BLACK, WHITE);
  }
}

void received_msg_Details() {
  // Create a new display buffer
  Paint_NewImage(image, EPD_2in13_V4_WIDTH, EPD_2in13_V4_HEIGHT, 90, WHITE);
  // Paint the whole frame white
  Paint_Clear(WHITE);
  // Draw message selection screen
  Paint_DrawString(0, 0, "Message Details:", &Font16, BLACK, WHITE);
  message_t *msg = &message_history[received_Cursor + ((received_Page - 1) * 3)].message;
  char *src = uid_to_string(msg->src);
  printf("- [%d]: %s %s %d\r\n", received_Cursor + ((received_Page - 1) * 3), src, MTYPE_STR[msg->mtype], msg->id);

  //Paint_DrawString(0, 25, saved_Messages[received_Cursor + ((received_Page - 1) * 3)], &Font16,
                   //BLACK, WHITE); // OLD STATIC TEST
  char data[255];
  sprintf(data, "Data: %s", TEXT_MESSAGE_STR[msg->data[0]]);
  Paint_DrawString(0, 25, data, &Font16,BLACK, WHITE);

  char from[20];
  sprintf(from, "From: %s",src); //Get node ID from network code
  Paint_DrawString(0, 75, from, &Font12, BLACK, WHITE);
  //Paint_DrawString(0, 75, "From: 11.22.33", &Font12, BLACK, WHITE); // OLD STATIC TEST

  char type[20];
  sprintf(type, "Msg Type: %s",MTYPE_STR[msg->mtype]); //Get node ID from network code
  Paint_DrawString(0, 55, type, &Font12, BLACK, WHITE);
  char buff[64];
  int time = absolute_time_diff_us(message_history[received_Cursor + ((received_Page - 1) * 3)].time,get_absolute_time())/1000/1000;
  if (time > 60) {
    time = time / 60;
    sprintf(buff, "%dm ago",time);
  } else if (time > 60 * 60) {
    time = (float)time / 60 / 60;
    sprintf(buff, "%dh ago",time);
  }else {
    sprintf(buff, "%ds ago",time);
  }
  //sprintf(buff, "%ds ago",time); //Get time_Stamp from network code
  Paint_DrawString(170, 75, buff, &Font12, BLACK, WHITE); // replace with new_String

  if (msg_Action_Cursor == 0) {
    Paint_DrawString(15, 100, "Text Reply", &Font16, WHITE, BLACK);
    Paint_DrawString(130, 100, "Neighbours", &Font16, BLACK, WHITE);
  } else if (msg_Action_Cursor == 1) {
    Paint_DrawString(15, 100, "Text Reply", &Font16, BLACK, WHITE);
    Paint_DrawString(130, 100, "Neighbours", &Font16, WHITE, BLACK);
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
  printf("msg number: %d\n", message_history_count);
  for (int i = 0; i < 3; i++) {
    // Display messages, sender and time received
    if (i + ((received_Page - 1) * 3) < message_history_count) {
      printf("Writing Message\n");
      message_t *msg = &message_history[i + ((received_Page - 1) * 3)].message;
      char *src = uid_to_string(msg->src);
      printf("- [%d]: %s %s %d\r\n", i + ((received_Page - 1) * 3), src, MTYPE_STR[msg->mtype], msg->id);
      
      //Paint_DrawString(20, 34 + i * 24, saved_Messages[i + ((received_Page - 1) * 3)], &Font16,
                       //BLACK, WHITE);
      Paint_DrawString(20, 34 + i * 24, MTYPE_STR[msg->mtype], &Font16,
                       BLACK, WHITE);
      //get the message history from the network.c file and print it here
      char buff [64];
      sprintf(buff, "From: %s",src); //Get node ID from network code
      Paint_DrawString(20, 47 + i * 24, buff, &Font12, BLACK,
                       WHITE); // replace with new_String

      int time = absolute_time_diff_us(message_history[i + ((received_Page - 1) * 3)].time,get_absolute_time())/1000/1000;
      if (time > 60) {
        time = time / 60;
        sprintf(buff, "%dm ago",time);
      } else if (time > 60 * 60) {
        time = (float)time / 60 / 60;
        sprintf(buff, "%dh ago",time);
      }else {
        sprintf(buff, "%ds ago",time);
      }
      Paint_DrawString(130, 47 + i * 24, buff, &Font12, BLACK,
                       WHITE);
    }
    // Clear rest of screen if there aren't enough messages to fill it
    else {
      printf("Clearing Message\n");
      Paint_DrawRectangle(0, 34 + i * 24, 170, 47 + i * 24, WHITE, DOT_PIXEL_2X2, DRAW_FILL_FULL);
    }
    // Remove new message indicator if it was seen
    if (new_Messages[i + ((received_Page - 1) * 3)] == 1) {
      Paint_DrawString(150, 34 + i * 24, "*NEW*", &Font16, BLACK, WHITE);
      if (received_Cursor == i) {
        new_Msg--;
        new_Messages[i + ((received_Page - 1) * 3)] = 0;
      }
    }
    for (int i = 0; i < 3; i++) {
      Paint_ClearWindows(0, 34 + i * 24, 20, 58 + i * 24, WHITE);
    }
    if (neighbour_table.count > 0){
      Paint_DrawString(0, 34 + received_Cursor * 24, ">", &Font16, BLACK, WHITE);
    }
  }
  if (message_history_count == 0) {
    Paint_DrawString(0, 34, "No messages received.", &Font16, BLACK, WHITE);
  }
  // Display page indicators
  if (received_Page > 1) {
    Paint_DrawString(100, 30, "^", &Font12, BLACK, WHITE);
  }
  if (received_Page < ((float)message_history_count / 3)) {
    Paint_DrawString(100, 110, "v", &Font12, BLACK, WHITE);
  }
}

void broadcast() {
  // Create a new display buffer
  //Paint_NewImage(image, EPD_2in13_V4_WIDTH, EPD_2in13_V4_HEIGHT, 90, WHITE);
  Paint_SelectImage(image);
  // Paint the whole frame white
  Paint_Clear(WHITE);
  // Draw message selection screen
  Paint_DrawString(0, 0, "Select action to broadcast:", &Font12, BLACK, WHITE);
  Paint_DrawString(0, 40, "You will broadcast to all neighbours within range.", &Font12, BLACK, WHITE);
  char buffer[64];
  sprintf(buffer, "Current known neighbours: %d.",neighbour_table.count);
  Paint_DrawString(0, 60, buffer, &Font12, BLACK, WHITE);

  if (broadcast_Action_Cursor == 0) {
    Paint_DrawString(5, 100, "Text", &Font16, WHITE, BLACK);
    Paint_DrawString(85, 100, "Ping", &Font16, BLACK, WHITE);
    Paint_DrawString(165, 100, "Request", &Font16, BLACK, WHITE);
  } else if (broadcast_Action_Cursor == 1) {
    Paint_DrawString(5, 100, "Text", &Font16, BLACK, WHITE);
    Paint_DrawString(85, 100, "Ping", &Font16, WHITE, BLACK);
    Paint_DrawString(165, 100, "Request", &Font16, BLACK, WHITE);
  } else if (broadcast_Action_Cursor == 2) {
    Paint_DrawString(5, 100, "Text", &Font16, BLACK, WHITE);
    Paint_DrawString(85, 100, "Ping", &Font16, BLACK, WHITE);
    Paint_DrawString(165, 100, "Request", &Font16, WHITE, BLACK);
  }
}

void neighbours_Action() {
  printf("ACTION.\n");
  // Create a new display buffer
  //Paint_NewImage(image, EPD_2in13_V4_WIDTH, EPD_2in13_V4_HEIGHT, 90, WHITE);
  Paint_SelectImage(image);
  // Paint the whole frame white
  Paint_Clear(WHITE);
  // Draw message selection screen
  Paint_DrawString(0, 0, "Neighbour Details:", &Font16, BLACK, WHITE);
  if (neighbour_table.count == 0){
    Paint_DrawString(0, 25, "No Neighbours Found.  Selected Action will broadcast.", &Font16, BLACK, WHITE);
  } else{
    printf("%d\n",neighbour_Table_Cursor + ((neighbour_received_Page - 1) * 3));
    neighbour_t *neighbour_Node_Action = &neighbour_table.neighbours[neighbour_Table_Cursor + ((neighbour_received_Page - 1) * 3)];
    char *src = uid_to_string(neighbour_Node_Action->uid);
    //Paint_DrawString(0, 25, src, &Font16,BLACK, WHITE);
    char buffer[64];
    sprintf(buffer, "Neighbour: %s",src);
    Paint_DrawString(0, 25, buffer, &Font16, BLACK, WHITE);
  // Update last seen and change units based on time
    int time_diff = absolute_time_diff_us(neighbour_Node_Action->last_seen,get_absolute_time())/1000/1000;
    if (time_diff > 60) {
      time_diff = time_diff / 60;
      sprintf(buffer, "Last Seen: %dm",time_diff);
    } else if (time_diff > 60 * 60) {
      time_diff = (float)time_diff / 60 / 60;
      sprintf(buffer, "Last Seen: %dh",time_diff);
    }else {
      sprintf(buffer, "Last Seen: %ds",time_diff);
    }
    Paint_DrawString(0, 55, buffer, &Font12, BLACK, WHITE);

    if (neighbour_Node_Action->version_major == 0 && neighbour_Node_Action->version_minor == 0){
      sprintf(buffer, "Version: Unk");
    } else {
      sprintf(buffer, "Version: %d.%d",neighbour_Node_Action->version_major,neighbour_Node_Action->version_minor);
    }
    Paint_DrawString(165, 55, buffer, &Font12, BLACK, WHITE);

    sprintf(buffer, "RSSI: %d",neighbour_Node_Action->rssi);
    Paint_DrawString(0, 75, buffer, &Font12, BLACK, WHITE);
  }
  if (neighbour_Action_Cursor == 0) {
    Paint_DrawString(5, 100, "Text", &Font16, WHITE, BLACK);
    Paint_DrawString(85, 100, "Ping", &Font16, BLACK, WHITE);
    Paint_DrawString(165, 100, "Request", &Font16, BLACK, WHITE);
  } else if (neighbour_Action_Cursor == 1) {
    Paint_DrawString(5, 100, "Text", &Font16, BLACK, WHITE);
    Paint_DrawString(85, 100, "Ping", &Font16, WHITE, BLACK);
    Paint_DrawString(165, 100, "Request", &Font16, BLACK, WHITE);
  } else if (neighbour_Action_Cursor == 2) {
    Paint_DrawString(5, 100, "Text", &Font16, BLACK, WHITE);
    Paint_DrawString(85, 100, "Ping", &Font16, BLACK, WHITE);
    Paint_DrawString(165, 100, "Request", &Font16, WHITE, BLACK);
  }
}

void neighbours_Table() {
  // Create a new display buffer
  Paint_NewImage(image, EPD_2in13_V4_WIDTH, EPD_2in13_V4_HEIGHT, 90, WHITE);
  // Paint the whole frame white
  Paint_Clear(WHITE);
  // Draw message selection screen
  Paint_DrawString(0, 5, "Neighbours Table:", &Font16, BLACK, WHITE);
  printf("neighbour table count: %d\n", neighbour_table.count);
  for (int i = 0; i < 3; i++) {
    // Display messages, sender and time received
    if (i + ((neighbour_received_Page - 1) * 3) < neighbour_table.count) {
      neighbour_t *neighbour_Node = &neighbour_table.neighbours[i + ((neighbour_received_Page - 1) * 3)];
      char *src = uid_to_string(neighbour_Node->uid);
      //printf("- [%d]: %s %s %d\r\n", i + ((received_Page - 1) * 3), src, MTYPE_STR[msg->mtype], msg->id);
      
      Paint_DrawString(20, 34 + i * 24, src, &Font16,
                       BLACK, WHITE);
    }
    // Clear rest of screen if there aren't enough messages to fill it
    else {
      printf("Clearing Message\n");
      Paint_DrawRectangle(0, 34 + i * 24, 170, 47 + i * 24, WHITE, DOT_PIXEL_2X2, DRAW_FILL_FULL);
    }
    for (int i = 0; i < 3; i++) {
      Paint_ClearWindows(0, 34 + i * 24, 20, 58 + i * 24, WHITE);
    }
    if (neighbour_table.count > 0){
      Paint_DrawString(0, 34 + neighbour_Table_Cursor * 24, ">", &Font16, BLACK, WHITE);
    }
  }
  if (neighbour_table.count == 0) {
    Paint_DrawString(0, 34, "No Neighbours Found.", &Font16, BLACK, WHITE);
  }
  // Display page indicators
  if (neighbour_received_Page > 1) {
    Paint_DrawString(100, 30, "^", &Font12, BLACK, WHITE);
  }
  if (neighbour_received_Page < ((float)neighbour_table.count / 3)) {
    Paint_DrawString(100, 110, "v", &Font12, BLACK, WHITE);
  }
}

void neighbours_Request(){
  printf("Requesting Neighbours/n");
  // Create a new display buffer
  Paint_NewImage(image, EPD_2in13_V4_WIDTH, EPD_2in13_V4_HEIGHT, 90, WHITE);
  // Paint the whole frame white
  Paint_Clear(WHITE);
  Paint_DrawString(0, 5, "Request:", &Font16, BLACK, WHITE);

  // Draw message selection screen
  if (neighbour_Request_Cursor == 0) {
    // Version selected
    Paint_DrawRectangle(15, 29, 210, 65, BLACK, DOT_PIXEL_2X2, DRAW_FILL_FULL);
    Paint_DrawString(27, 39, "Version", &Font16, WHITE, WHITE);
    Paint_DrawRectangle(15, 71, 210, 107, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawString(22, 81, "Uptime", &Font16, BLACK, WHITE);
  }
  if (neighbour_Request_Cursor == 1) {
    // Uptime selected
    Paint_DrawRectangle(15, 29, 210, 65, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawString(27, 39, "Version", &Font16, BLACK, WHITE);
    Paint_DrawRectangle(15, 71, 210, 107, BLACK, DOT_PIXEL_2X2, DRAW_FILL_FULL);
    Paint_DrawString(22, 81, "Uptime", &Font16, WHITE, WHITE);
  }
}

void neighbours_Screen() {
  // Create a new display buffer
  Paint_NewImage(image, EPD_2in13_V4_WIDTH, EPD_2in13_V4_HEIGHT, 90, WHITE);
  // Paint the whole frame white
  Paint_Clear(WHITE);

  // Draw message selection screen
  if (neighbour_Cursor == 0) {
    // Neighbours table selected
    Paint_DrawRectangle(15, 29, 210, 65, BLACK, DOT_PIXEL_2X2, DRAW_FILL_FULL);
    Paint_DrawString(27, 39, "View Neighbours", &Font16, WHITE, WHITE);
    Paint_DrawRectangle(15, 71, 210, 107, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawString(22, 81, "Broadcast To All", &Font16, BLACK, WHITE);
  }
  if (neighbour_Cursor == 1) {
    // msg screen selected
    Paint_DrawRectangle(15, 29, 210, 65, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawString(27, 39, "View Neighbours", &Font16, BLACK, WHITE);
    Paint_DrawRectangle(15, 71, 210, 107, BLACK, DOT_PIXEL_2X2, DRAW_FILL_FULL);
    Paint_DrawString(22, 81, "Broadcast To All", &Font16, WHITE, WHITE);
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
  #ifdef PIN_CONFIG_v2
  // Draw Battery %
  char bat[12];
  sprintf(bat, "%.1f%%", (read_voltage())/3.3*100);
  Paint_DrawString(185, 0, bat, &Font12, BLACK, WHITE);
  #endif

  #ifndef PIN_CONFIG_v2
  // Draw Battery %
  Paint_DrawString(185, 0, "100%%", &Font12, BLACK, WHITE);
  #endif
  Paint_DrawRectangle(182, 0, 230, 15, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);

  // Draw version number
  char version[12];
  char neighbour_Count[64];
  sprintf(version, "V/ink v%d.%d", VERSION_MAJOR, VERSION_MINOR);
  Paint_DrawString(0, 0, version, &Font12, BLACK, WHITE);
  Paint_DrawRectangle(0, 0, 75, 15, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);

  // Draw nearby nodes, replace with network code
  //Paint_DrawString(90, 0, "Nearby Nodes", &Font12, BLACK, WHITE);
  // convert neighbour_table.count to string
  sprintf(neighbour_Count, "%d Nearby Nodes", neighbour_table.count);
  Paint_DrawString(80, 0, neighbour_Count, &Font12, BLACK, WHITE);
  //printf("Found Nodes: %d\n",neighbour_table.count);
  Paint_DrawRectangle(75, 0, 182, 15, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
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

void go_to_Sleep(){
  five_Seconds = true;
  cancel_alarm(alarm_id);
  Paint_DrawString(225, 0, "SLP", &Font12, WHITE, BLACK);
  EPD_2in13_V4_Display_Partial(image);
  EPD_2in13_V4_Sleep();
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
