#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"

#include "io.h"
#include "pico_config.h"
#include "screen.h"
#include "utils.h"
#include "voidlink.h"

void handle_button_callback(uint gpio, uint32_t events) {
  // if (screen != SCREEN_IDLE || (state != STATE_IDLE && state != STATE_RX))
  if (screen != SCREEN_IDLE)
    return;

  if (gpio == PIN_BUTTON_NEXT) { // add switch cases for each state
    switch (display) {
    case DISPLAY_HOME:
      printf("On Home Screen.\n"); // For testing purposes
      // Add selection drawings for home screen
      home_Cursor = (home_Cursor + 1) % 3;
      home_Screen();
      printf("new msgs: %d\n", new_Msg); // For testing purposes
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_RXMSG:
      printf("On Received Messages Screen.\n"); // For testing purposes
      // Reset cursor if moving to new page
      if (received_Cursor == 2 && (message_history_count - (received_Page - 1) * 3) > 3) {
        received_Page++;
        received_Cursor = 0;
      } else {
        received_Cursor = (received_Cursor + 1) % (message_history_count - (received_Page - 1) * 3);
      }
      // Display cursor
      received_Msgs();
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_RXMSG_DETAILS:
      msg_Action_Cursor = (msg_Action_Cursor + 1) % 2;
      received_msg_Details();
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_SEND_TO:
      send_to_Cursor = (send_to_Cursor + 1) % (neighbour_table.count+1);
      send_To_Screen();
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_MSG:
      // printf("On Message Screen.\n"); // For testing purposes
      // message_Cursor = (message_Cursor + 1) % 3;
      if (message_Cursor == 2 && (MAX_MSG_SEND - (msg_received_Page - 1) * 3) > 3) {
        msg_received_Page++;
        message_Cursor = 0;
      } else {
        message_Cursor = (message_Cursor + 1) % (MAX_MSG_SEND - (msg_received_Page - 1) * 3);
      }
      // Display cursor
      msg_Screen();
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_SETTINGS:
      printf("On Settings Screen.\n"); // For testing purposes
      // Add selection drawings for settings screen
      settings_Cursor = (settings_Cursor + 1) % 2;
      settings_Screen();
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_SETTINGS_INFO:
      printf("On Settings Info Screen.\n"); // For testing purposes
      // Add selection drawings for settings screen
      set_Info_Cursor = (set_Info_Cursor + 1) % 6;
      settings_Info();
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_NEIGHBOURS_TABLE:
      printf("On Neighbours Table.\n"); //For testing purposes
      if (neighbour_Table_Cursor == 2 && (neighbour_table.count - (neighbour_Table_Cursor - 1) * 3) > 3) {
        neighbour_received_Page++;
        neighbour_Table_Cursor = 0;
      } else {
        if (neighbour_table.count == 0){
          neighbour_Table_Cursor = 0;
        } else {
        neighbour_Table_Cursor = (neighbour_Table_Cursor + 1) % (neighbour_table.count - (neighbour_Table_Cursor - 1) * 3);
        }
      }
      neighbours_Table();
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_NEIGHBOURS_ACTION:
      neighbour_Action_Cursor = (neighbour_Action_Cursor + 1) % 3;
      neighbours_Action();
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_NEIGHBOURS_REQUEST:
      neighbour_Request_Cursor = (neighbour_Request_Cursor + 1) % 2;
      neighbours_Request();
      screen = SCREEN_DRAW_READY;
    break;

    case DISPLAY_NEIGHBOURS:
      printf("On Neighbours Screen.\n"); // For testing purposes
      // Add selection drawings for neighbours screen
      neighbour_Cursor = (neighbour_Cursor + 1) % 2;
      neighbours_Screen();
      screen = SCREEN_DRAW_READY;
      break;
    }

  } else if (gpio == PIN_BUTTON_PREV) { // add switch cases for each state
    switch (display) {
    case DISPLAY_HOME:
      printf("On Home Screen.\n"); // For testing purposes
      // Add selection drawings for home screen
      home_Cursor = (home_Cursor - 1 + 3) % 3;
      home_Screen();
      printf("new msgs: %d\n", new_Msg); // For testing purposes
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_RXMSG:
      printf("On Received Messages Screen.\n"); // For testing purposes
      // Reset cursor if moving to new page
      if (received_Cursor == 0 && received_Page >= 1) {
        received_Page--;
        received_Cursor = 2;
      } else {
        received_Cursor = (received_Cursor - 1 + (message_history_count - (received_Page - 1) * 3)) % (message_history_count - (received_Page - 1) * 3);
      }
      // Display cursor
      received_Msgs();
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_RXMSG_DETAILS:
      msg_Action_Cursor = (msg_Action_Cursor - 1 + 2) % 2;
      received_msg_Details();
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_SEND_TO:
      send_to_Cursor = (send_to_Cursor - 1 + (neighbour_table.count+1)) % (neighbour_table.count+1);
      send_To_Screen();
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_MSG:
      // printf("On Message Screen.\n"); // For testing purposes
      // message_Cursor = (message_Cursor + 1) % 3;
      // Reset cursor if moving to new page
      if (message_Cursor == 0 && msg_received_Page >= 1) {
        msg_received_Page--;
        message_Cursor = 2;
      } else {
        message_Cursor = (message_Cursor - 1 + (MAX_MSG_SEND - (msg_received_Page - 1) * 3)) % (MAX_MSG_SEND - (msg_received_Page - 1) * 3);
      }
      // Display cursor
      msg_Screen();
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_SETTINGS:
      printf("On Settings Screen.\n"); // For testing purposes
      // Add selection drawings for settings screen
      settings_Cursor = (settings_Cursor - 1 + 2) % 2;
      settings_Screen();
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_SETTINGS_INFO:
      printf("On Settings Info Screen.\n"); // For testing purposes
      // Add selection drawings for settings screen
      set_Info_Cursor = (set_Info_Cursor - 1 + 6) % 6;
      settings_Info();
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_NEIGHBOURS_TABLE:
      printf("On Neighbours Table.\n"); //For testing purposes
      if (neighbour_Table_Cursor == 0 && neighbour_received_Page >= 1) {
        neighbour_received_Page--;
        neighbour_Table_Cursor = 2;
      } else {
        if (neighbour_table.count == 0){
          neighbour_Table_Cursor = 0;
        } else {
        neighbour_Table_Cursor = (neighbour_Table_Cursor - 1 + (neighbour_table.count - (neighbour_received_Page - 1) * 3)) % (neighbour_table.count - (neighbour_received_Page - 1) * 3);
        }
      }
      neighbours_Table();
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_NEIGHBOURS_ACTION:
      neighbour_Action_Cursor = (neighbour_Action_Cursor - 1 + 3) % 3;
      neighbours_Action();
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_NEIGHBOURS_REQUEST:
      neighbour_Request_Cursor = (neighbour_Request_Cursor - 1 + 2) % 2;
      neighbours_Request();
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_NEIGHBOURS:
      printf("On Neighbours Screen.\n"); // For testing purposes
      // Add selection drawings for neighbours screen
      neighbour_Cursor = (neighbour_Cursor - 1 + 2) % 2;
      neighbours_Screen();
      screen = SCREEN_DRAW_READY;
      break;
    }

  } else if (gpio == PIN_BUTTON_OK) {

    switch (display) {
    case DISPLAY_HOME:
      // printf("On Home Screen."); //For testing purposes
      //  Add selection drawings for home screen
      if (home_Cursor == 0) { // Go to msg screen
        display = DISPLAY_RXMSG;
        received_Cursor = 0;
        received_Msgs();
        refresh_Counter = 15;
        screen = SCREEN_DRAW_READY;
      }

      if (home_Cursor == 1) { // Go to settings screen
        display = DISPLAY_SETTINGS;
        settings_Screen();
        refresh_Counter = 15;
        screen = SCREEN_DRAW_READY;
      }

      if (home_Cursor == 2) { // Go to neighbours screen
        neighbours_Screen();
        display = DISPLAY_NEIGHBOURS;
        neighbour_Cursor = 0;
        refresh_Counter = 15;
        screen = SCREEN_DRAW_READY;
      }

      break;

    case DISPLAY_RXMSG:
      // Add selection drawings for received messages screen
      received_msg_Details();
      display = DISPLAY_RXMSG_DETAILS;
      msg_Action_Cursor = 0;
      refresh_Counter = 15;
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_RXMSG_DETAILS:
      if (msg_Action_Cursor == 0) {
        // Reply to message
        display = DISPLAY_MSG;
        msg_Screen(); // Needs to change, needs to get to node actions screen.
        refresh_Counter = 15;
        screen = SCREEN_DRAW_READY;
      }

      if (msg_Action_Cursor == 1) { // Go to Neighbours screen
        neighbours_Table();
        display = DISPLAY_NEIGHBOURS_TABLE;
        refresh_Counter = 15;
        screen = SCREEN_DRAW_READY;
      }
      break;

    case DISPLAY_SEND_TO:
      uid_t dst;
      info_key_t key;
      text_id_t id;
      if (send_to_Cursor == 1) {
        dst = neighbour_table.neighbours[send_to_Cursor - 1].uid;
      } else {
        dst = get_broadcast_uid();
      }
      if (msg_Type == 0){
        id = (text_id_t)(message_Cursor + ((msg_received_Page - 1) * 3));
        try_transmit(new_text_message(dst, id));
      } else if (msg_Type == 1){
        if (neighbour_Request_Cursor == 0){ // Version
          key = 0;
        } else if (neighbour_Request_Cursor == 1){ // Uptime
          key = 2;
        }
        try_transmit(new_request_message(dst, key));
      } else if (msg_Type == 2){
        try_transmit(new_ping_message(dst));
      }
      // Possibly add animation to show message is being sent

      break;

    case DISPLAY_MSG:
      // printf("Send Msg."); //For testing purposes
      // state = STATE_TX_READY;
      msg_Type = 0;
      send_To_Screen();
      display = DISPLAY_SEND_TO;
      refresh_Counter = 15;
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_SETTINGS:
      //printf("On Settings Screen."); //For testing purposes
      //  Add selection drawings for settings screen
      temp_Cursor = set_Info_Cursor;
      display = DISPLAY_SETTINGS_INFO;
      settings_Info();
      // refresh_Counter = 15;
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_SETTINGS_INFO:
      // Set display timeout
      settings_Screen();
      display = DISPLAY_SETTINGS;
      refresh_Counter = 15;
      screen = SCREEN_DRAW_READY;
      if (settings_Cursor == 0){
        if (set_Info_Cursor == 0) {
          display_Timeout = 0;
        }
        if (set_Info_Cursor == 1) {
          display_Timeout = 5000;
        }
        if (set_Info_Cursor == 2) {
          display_Timeout = 10000;
        }
        if (set_Info_Cursor == 3) {
          display_Timeout = 30000;
        }
        if (set_Info_Cursor == 4) {
          display_Timeout = 60000;
        }
        if (set_Info_Cursor == 5) {
          display_Timeout = 120000;
        }
      }
      else if (settings_Cursor == 1){
        if (set_Info_Cursor == 0) {
          set_range(DEFAULT);
        }
        if (set_Info_Cursor == 1) {
          set_range(FAST);
        }
        if (set_Info_Cursor > 1) {
          set_range(LONGRANGE);
        }
      }
      break;

    case DISPLAY_NEIGHBOURS_TABLE:
      if (neighbour_table.count > 0){
        printf("On Neighbours Table.\n"); //For testing purposes
        neighbours_Action();
        display = DISPLAY_NEIGHBOURS_ACTION;
        neighbour_Action_Cursor = 0;
        refresh_Counter = 15;
        screen = SCREEN_DRAW_READY;
      }
      break;

    case DISPLAY_NEIGHBOURS_ACTION:
      if (neighbour_Action_Cursor == 0) {
        // Send a text msg
        printf("text.\n");
        msg_Screen();
        display = DISPLAY_MSG;
        refresh_Counter = 15;
        screen = SCREEN_DRAW_READY;
      } else if (neighbour_Action_Cursor == 1) {
        // Send Ping message
        printf("ping.\n");
        //try_transmit(new_ping_message(neighbour_table.neighbours[neighbour_Table_Cursor + ((neighbour_received_Page - 1) * 3)].uid));
        msg_Type = 2;
        send_To_Screen();
        display = DISPLAY_SEND_TO;
        refresh_Counter = 15;
        screen = SCREEN_DRAW_READY;
      } else if (neighbour_Action_Cursor == 2) {
        // Send Request message
        printf("request.\n");
        neighbours_Request();
        display = DISPLAY_NEIGHBOURS_REQUEST;
        refresh_Counter = 15;
        screen = SCREEN_DRAW_READY;
      }
      break;

    case DISPLAY_NEIGHBOURS_REQUEST:
      msg_Type = 1;
      send_To_Screen();
      display = DISPLAY_SEND_TO;
      refresh_Counter = 15;
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_NEIGHBOURS:
      printf("On Neighbours Screen."); //For testing purposes
      //  Add selection drawings for neighbours screen
      if (neighbour_Cursor == 0) {
        // Go to neighbours table screen
        neighbours_Table();
        display = DISPLAY_NEIGHBOURS_TABLE;
        refresh_Counter = 15;
        screen = SCREEN_DRAW_READY;
      }
     // Go to msg screen
      if (neighbour_Cursor == 1) {
        // Broadcast
        display = DISPLAY_MSG;
        msg_Screen();
        refresh_Counter = 15;
        screen = SCREEN_DRAW_READY;
      }
      break;
    }

  } else if (gpio == PIN_BUTTON_BACK) {

    switch (display) {
    case DISPLAY_HOME:
      printf("Already at home\n"); // For testing purposes
      // Add selection drawings for home screen
      //sprintf(test, "TEST %d", message_history_count);
      //strcpy(saved_Messages[message_history_count], test);
      //new_Messages[message_history_count] = 1;
      //msg_Number++;
      //new_Msg++;
      screen = SCREEN_DRAW_READY;

      break;

    case DISPLAY_RXMSG:
      // Add selection drawings for received messages screen
      printf("Going home.\n"); // For testing purposes
      display = DISPLAY_HOME;
      received_Page = 1;
      home_Screen();
      refresh_Counter = 15;
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_RXMSG_DETAILS:
      // Add selection drawings for received messages screen
      display = DISPLAY_RXMSG;
      received_Msgs();
      refresh_Counter = 15;
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_SEND_TO:
      if (msg_Type == 0){
        display = DISPLAY_MSG;
        msg_Screen();
      } else if (msg_Type == 1){
        display = DISPLAY_NEIGHBOURS_REQUEST;
        neighbours_Request();
      } else if (msg_Type == 2){
        display = DISPLAY_NEIGHBOURS_ACTION;
        neighbours_Action();
      }
      refresh_Counter = 15;
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_MSG:
      // printf("Send Msg."); //For testing purposes
      display = DISPLAY_NEIGHBOURS_ACTION;
      msg_received_Page = 1;
      neighbours_Action();
      refresh_Counter = 15;
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_SETTINGS:
      // printf("On Settings Screen."); //For testing purposes
      //  Add selection drawings for settings screen
      display = DISPLAY_HOME;
      home_Screen();
      refresh_Counter = 15;
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_SETTINGS_INFO:
      set_Info_Cursor = temp_Cursor;
      settings_Screen();
      display = DISPLAY_SETTINGS;
      refresh_Counter = 15;
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_NEIGHBOURS_TABLE:
      display = DISPLAY_NEIGHBOURS;
      neighbour_received_Page = 1;
      neighbours_Screen();
      refresh_Counter = 15;
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_NEIGHBOURS_ACTION:
      display = DISPLAY_NEIGHBOURS_TABLE;
      neighbours_Table();
      refresh_Counter = 15;
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_NEIGHBOURS_REQUEST:
      display = DISPLAY_NEIGHBOURS_ACTION;
      neighbours_Action();
      refresh_Counter = 15;
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_NEIGHBOURS:
      display = DISPLAY_HOME;
      home_Screen();
      refresh_Counter = 15;
      screen = SCREEN_DRAW_READY;
      break;
    }
  } else if (gpio == PIN_BUTTON_HOME) {

    switch (display) {
    case DISPLAY_HOME:
      printf("Already at home\n"); // For testing purposes
      break;

    case DISPLAY_RXMSG:
      // Add selection drawings for received messages screen
      printf("Going home.\n"); // For testing purposes
      display = DISPLAY_HOME;
      received_Page = 1;
      home_Screen();
      refresh_Counter = 15;
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_RXMSG_DETAILS:
      // Add selection drawings for received messages screen
      display = DISPLAY_HOME;
      received_Page = 1;
      home_Screen();
      refresh_Counter = 15;
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_SEND_TO:
      display = DISPLAY_HOME;
      msg_received_Page = 1;
      received_Page = 1;
      neighbour_received_Page = 1;
      home_Screen();
      refresh_Counter = 15;
      screen = SCREEN_DRAW_READY;
 
      break;

    case DISPLAY_MSG:
      // printf("Send Msg."); //For testing purposes
      display = DISPLAY_HOME;
      msg_received_Page = 1;
      home_Screen();
      refresh_Counter = 15;
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_SETTINGS:
      // printf("On Settings Screen."); //For testing purposes
      //  Add selection drawings for settings screen
      display = DISPLAY_HOME;
      home_Screen();
      refresh_Counter = 15;
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_SETTINGS_INFO:
      set_Info_Cursor = temp_Cursor;
      home_Screen();
      display = DISPLAY_HOME;
      refresh_Counter = 15;
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_NEIGHBOURS_TABLE:
      display = DISPLAY_HOME;
      neighbour_received_Page = 1;
      home_Screen();
      refresh_Counter = 15;
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_NEIGHBOURS_ACTION:
      display = DISPLAY_HOME;
      neighbour_received_Page = 1;
      home_Screen();
      refresh_Counter = 15;
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_NEIGHBOURS_REQUEST:
      display = DISPLAY_HOME;
      neighbour_received_Page = 1;
      home_Screen();
      refresh_Counter = 15;
      screen = SCREEN_DRAW_READY;
      break;

    case DISPLAY_NEIGHBOURS:
      display = DISPLAY_HOME;
      home_Screen();
      refresh_Counter = 15;
      screen = SCREEN_DRAW_READY;
      break;
    }
  } else if (gpio == PIN_BUTTON_SLEEP) {
    if (five_Seconds == true){ // Currently Sleeping, wake up
      screen = SCREEN_DRAW_READY;
    }
    if (five_Seconds == false){ // Currently awake, Go To Sleep
      go_to_Sleep();
    }
  }
}
