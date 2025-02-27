#include <stdio.h>

#include "hardware/uart.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "tusb.h"

#include "pico_config.h"
#include "sx126x.h"
#include "sx126x_debug.h"
#include "sx126x_hal_context.h"

#include "EPD_2in13_V4.h"
#include "GUI_Paint.h"
#include "hardware/timer.h"

#define PAYLOAD_BUFFER_SIZE 255

static uint8_t payload_buf[PAYLOAD_BUFFER_SIZE];

// ((EPD_2in13_V4_WIDTH % 8 == 0) ? (EPD_2in13_V4_WIDTH / 8) : (EPD_2in13_V4_WIDTH / 8 + 1)) *
// EPD_2in13_V4_HEIGHT = 4080
#define IMAGE_SIZE 4080
static uint8_t image[IMAGE_SIZE];
static uint8_t wakeup[IMAGE_SIZE];
static uint8_t message_Cursor = 0;
static uint8_t home_Cursor = 0;
static uint8_t settings_Cursor = 0;
static uint8_t set_Info_Cursor = 2;
static uint8_t temp_Cursor;
static uint8_t received_Cursor = 0;
int received_Page = 1;
alarm_id_t alarm_id;
int refresh_Counter = 0;
volatile bool five_Seconds = true;
static uint32_t display_Timeout = 10000;
//character array to store received messages
char saved_Messages[16][PAYLOAD_BUFFER_SIZE + 4];
char test [7];
int new_Messages [16];
//number of messages received
int32_t msg_Number = 0;
int32_t new_Msg = 0;

static char *messages[] = {
    "Hello",
    "World",
    "Pico!",
};

// Main state machine
typedef enum {
  STATE_IDLE,
  STATE_TX_READY,
  STATE_TX,
  STATE_TX_DONE,
  STATE_RX,
} state_t;

static state_t state = STATE_IDLE;

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
  DISPLAY_NEIGHBOURS,
  DISPLAY_SETTINGS,
  DISPLAY_SETTINGS_INFO,
} display_t;

static display_t display = DISPLAY_HOME;

static sx126x_hal_context_t context;

void handle_tx_callback() {
  sx126x_chip_status_t status = {.chip_mode = 0, .cmd_status = 0};
  sx126x_get_status(&context, &status);
  if (status.cmd_status != SX126X_CMD_STATUS_CMD_TX_DONE) {
    printf("tx status error (mode: %d | cmd: %d)\n", status.chip_mode, status.cmd_status);
    return;
  }

  printf("payload sent\n");

  state = STATE_TX_DONE;
}

void handle_rx_callback() {
  // Make sure the data available status is set before reading the rx buffer.
  sx126x_chip_status_t status = {.chip_mode = 0, .cmd_status = 0};
  sx126x_get_status(&context, &status);
  if (status.cmd_status != SX126X_CMD_STATUS_DATA_AVAILABLE) {
    printf("rx status error (mode: %d | cmd: %d)\n", status.chip_mode, status.cmd_status);
    return;
  }

  // Get length and the start address of the received message.
  sx126x_rx_buffer_status_t buffer_status = {.buffer_start_pointer = 0, .pld_len_in_bytes = 0};
  sx126x_get_rx_buffer_status(&context, &buffer_status);
  printf("payload received: %d @ %d\n", buffer_status.pld_len_in_bytes,
         buffer_status.buffer_start_pointer);

  // Make sure the buffer has enough space to read the message.
  if (buffer_status.pld_len_in_bytes > PAYLOAD_BUFFER_SIZE) {
    printf("payload is bigger than the buffer (%d)\n", PAYLOAD_BUFFER_SIZE);
    return;
  }

  // Read and print the buffer.
  sx126x_read_buffer(&context, buffer_status.buffer_start_pointer, payload_buf,
                     buffer_status.pld_len_in_bytes);

  printf("->");
  for (int i = 0; i < buffer_status.pld_len_in_bytes; i++) {
    printf(" %d", payload_buf[i]);
  }
  printf("\n");

  // Draw the received message on the e-paper display.
  // add "rx: " to the beginning of the payload
  //Paint_ClearWindows(0, 106, 122, 122, WHITE);
  char payload_buf_with_rx[buffer_status.pld_len_in_bytes + 4];
  sprintf(payload_buf_with_rx, "rx: %s", payload_buf);
  //add received message to saved messages array
  //saved_Messages[msg_Number][buffer_status.pld_len_in_bytes + 4] = payload_buf;
  strcpy(saved_Messages[msg_Number], (char *) payload_buf);
  //Paint_DrawString_EN(0, 106, (char *)payload_buf_with_rx, &Font16, BLACK, WHITE);
  //increment message number (will be replaced with network code varaible)
  new_Messages[msg_Number] = 1;
  msg_Number++;
  new_Msg++;
  // Trigger an LED blink to signify a message has been received.

  screen = SCREEN_DRAW_READY;
  state = STATE_RX;
}

// Callback function for everytime an interrupt is detected on DIO1.
void handle_dio1_callback(uint gpio, uint32_t events) {
  // Get the irq status to learn why the interrupt was triggered.
  // And clean the interrupt at the same time, since it is getting handled now.
  sx126x_irq_mask_t irq = 0;
  sx126x_get_and_clear_irq_status(&context, &irq);

  sx126x_print_decoded_irq(irq);

  if (irq == SX126X_IRQ_TX_DONE) {
    handle_tx_callback();
  } else if (irq == SX126X_IRQ_RX_DONE) {
    handle_rx_callback();
  }
}

void msg_Screen(){
  // Create a new display buffer
  Paint_NewImage(image, EPD_2in13_V4_WIDTH, EPD_2in13_V4_HEIGHT, 90, WHITE);
  // Paint the whole frame white
  Paint_Clear(WHITE);

  // Draw message selection screen
  Paint_DrawString_EN(0, 10, "Select a message:", &Font16, BLACK, WHITE);
  Paint_DrawString_EN(20, 34, "1. Hello", &Font16, BLACK, WHITE);
  Paint_DrawString_EN(20, 58, "2. Yes", &Font16, BLACK, WHITE);
  Paint_DrawString_EN(20, 82, "3. No", &Font16, BLACK, WHITE);
  //Paint_DrawString_EN(20, 106, "4. On my way!", &Font16, BLACK, WHITE);

  // Display cursor
  Paint_DrawString_EN(0, 34, ">", &Font16, BLACK, WHITE);
  message_Cursor = 0;
}

void received_Msgs(){
  // Create a new display buffer
  Paint_NewImage(image, EPD_2in13_V4_WIDTH, EPD_2in13_V4_HEIGHT, 90, WHITE);
  // Paint the whole frame white
  Paint_Clear(WHITE);

  //Paint_DrawString_EN(0, 34, ">", &Font16, BLACK, WHITE);

  // Draw message selection screen
  Paint_DrawString_EN(0, 10, "Received Messages:", &Font16, BLACK, WHITE);
  printf("msg number: %d\n",msg_Number);
  for (int i = 0; i < 3; i++){
    //Display messages, sender and time received
    if (i+((received_Page-1)*3) < msg_Number) {
    printf("Writing Message\n");
      Paint_DrawString_EN(20, 34 + i * 24, saved_Messages[i+((received_Page-1)*3)], &Font16, BLACK, WHITE);
      //sprintf(new_String, "From: %s",node_ID); //Get node ID from network code
      Paint_DrawString_EN(20, 47 + i * 24, "From: 11.22.33", &Font12, BLACK, WHITE);//replace with new_String
      //sprintf(new_String, "%s mins ago",time_Stamp); //Get time_Stamp from network code
      Paint_DrawString_EN(130, 47 + i * 24, "3 mins ago", &Font12, BLACK, WHITE);//replace with new_String
    } 
    //Clear rest of screen if there aren't enough messages to fill it
    else {
      printf("Clearing Message\n");
      Paint_DrawRectangle(0,34 + i * 24,170,47 + i * 24,WHITE,DOT_PIXEL_2X2,DRAW_FILL_FULL);
    }
    //Remove new message indicator if it was seen
    if (new_Messages[i+((received_Page-1)*3)] == 1){
      new_Msg--;
      new_Messages[i+((received_Page-1)*3)] = 0;
    }
    for (int i = 0; i < 3; i++) {
      Paint_ClearWindows(0, 34 + i * 24, 20, 58 + i * 24, WHITE);
      }
    Paint_DrawString_EN(0, 34 + received_Cursor * 24, ">", &Font16, BLACK, WHITE);
  }
  if (msg_Number == 0){
    Paint_DrawString_EN(0, 34, "No messages received.", &Font16, BLACK, WHITE);
  }
  // Display page indicators
  if (received_Page>1){
    Paint_DrawString_EN(100, 30, "^", &Font12, BLACK, WHITE);
  }
  if (received_Page < ((float)msg_Number/3)){
    Paint_DrawString_EN(100, 110, "v", &Font12, BLACK, WHITE);
  }
}

void neighbours_Screen(){
  // Create a new display buffer
  Paint_NewImage(image, EPD_2in13_V4_WIDTH, EPD_2in13_V4_HEIGHT, 90, WHITE);
  // Paint the whole frame white
  Paint_Clear(WHITE);

  // Draw message selection screen
  Paint_DrawRectangle(15,29,150,49,BLACK,DOT_PIXEL_2X2,DRAW_FILL_EMPTY);
  Paint_DrawString_EN(20, 34, "Neighbours", &Font16, BLACK, WHITE);
  Paint_DrawRectangle(15,101,150,121,BLACK,DOT_PIXEL_2X2,DRAW_FILL_EMPTY);
  Paint_DrawString_EN(20, 106, "Broadcast", &Font16, BLACK, WHITE);
}

void home_Screen(){
  // Create a new display buffer
  Paint_NewImage(image, EPD_2in13_V4_WIDTH, EPD_2in13_V4_HEIGHT, 90, WHITE); //Just do for first time setup
  //Paint_SelectImage(image); //Do this for every other time
  // Paint the whole frame white
  Paint_Clear(WHITE); //Just do for first time setup

  //SETTINGS SELECTED
  if (home_Cursor==1) {
    //select next screen
    Paint_Clear(WHITE);
    Paint_DrawRectangle(50,50,80,80,BLACK,DOT_PIXEL_2X2,DRAW_FILL_EMPTY);
    Paint_DrawString_EN(57, 57, "M", &Font20, BLACK, WHITE);
    Paint_DrawString_EN(37, 85, "Messages", &Font12, WHITE, WHITE);
    Paint_DrawRectangle(100,50,130,80,BLACK,DOT_PIXEL_2X2,DRAW_FILL_FULL);
    Paint_DrawString_EN(107, 57, "S", &Font20, WHITE, WHITE);
    Paint_DrawString_EN(86, 85, "Settings", &Font12, BLACK, WHITE);
    Paint_DrawRectangle(150,50,180,80,BLACK,DOT_PIXEL_2X2,DRAW_FILL_EMPTY);
    Paint_DrawString_EN(157, 57, "N", &Font20, BLACK, WHITE);
    Paint_DrawString_EN(132, 85, "Neighbours", &Font12, WHITE, WHITE);
  }
  //MESSAGES SELECTED
  if (home_Cursor==0) {
    //select next screen
    Paint_Clear(WHITE);
    Paint_DrawRectangle(50,50,80,80,BLACK,DOT_PIXEL_2X2,DRAW_FILL_FULL);
    Paint_DrawString_EN(57, 57, "M", &Font20, WHITE, WHITE);
    Paint_DrawString_EN(37, 85, "Messages", &Font12, BLACK, WHITE);
    Paint_DrawRectangle(100,50,130,80,BLACK,DOT_PIXEL_2X2,DRAW_FILL_EMPTY);
    Paint_DrawString_EN(107, 57, "S", &Font20, BLACK, WHITE);
    Paint_DrawString_EN(83, 85, "Settings", &Font12, WHITE, WHITE);
    Paint_DrawRectangle(150,50,180,80,BLACK,DOT_PIXEL_2X2,DRAW_FILL_EMPTY);
    Paint_DrawString_EN(157, 57, "N", &Font20, BLACK, WHITE);
    Paint_DrawString_EN(132, 85, "Neighbours", &Font12, WHITE, WHITE);
  }
  //NEIGHBOURS SELECTED
  if (home_Cursor==2){
    Paint_Clear(WHITE);
    Paint_DrawRectangle(50,50,80,80,BLACK,DOT_PIXEL_2X2,DRAW_FILL_EMPTY);
    Paint_DrawString_EN(57, 57, "M", &Font20, BLACK, WHITE);
    Paint_DrawString_EN(37, 85, "Messages", &Font12, WHITE, WHITE);
    Paint_DrawRectangle(100,50,130,80,BLACK,DOT_PIXEL_2X2,DRAW_FILL_EMPTY);
    Paint_DrawString_EN(107, 57, "S", &Font20, BLACK, WHITE);
    Paint_DrawString_EN(83, 85, "Settings", &Font12, WHITE, WHITE);
    Paint_DrawRectangle(150,50,180,80,BLACK,DOT_PIXEL_2X2,DRAW_FILL_FULL);
    Paint_DrawString_EN(157, 57, "N", &Font20, WHITE, WHITE);
    Paint_DrawString_EN(132, 85, "Neighbours", &Font12, BLACK, WHITE);
  }
  if (new_Msg > 0){
    if (new_Msg == 1) {
      Paint_DrawString_EN(95, 110," new message", &Font12, BLACK, WHITE);
    }
    else {
      Paint_DrawString_EN(95, 110," new messages", &Font12, BLACK, WHITE);
    }
    Paint_DrawNum(90, 110, new_Msg, &Font12, WHITE, BLACK);
  }

  //Draw version number
  Paint_DrawString_EN(0, 0, "V/ink v1.0", &Font12, BLACK, WHITE);

  //Draw nearby nodes, replace with network code
  Paint_DrawString_EN(110, 0, "2 Nearby Nodes", &Font12, BLACK, WHITE);
}

void settings_Info(){
  Paint_SelectImage(image);
  if(settings_Cursor==0){
    //clear screen
    Paint_ClearWindows(170, 34, 300, 50, WHITE);
    //refresh display
    EPD_2in13_V4_Display_Partial(image);
    Paint_DrawString_EN(200, 26, "^", &Font12, BLACK, WHITE);
    Paint_DrawString_EN(200, 44, "v", &Font12, BLACK, WHITE);
    if (set_Info_Cursor == 0){
      Paint_DrawString_EN(170, 34, "    Never", &Font12, BLACK, WHITE);
    }
    else if (set_Info_Cursor == 1){
      Paint_DrawString_EN(170, 34, "5 Seconds", &Font12, BLACK, WHITE);
    }
    else if (set_Info_Cursor == 2){
      Paint_DrawString_EN(170, 34, "10 Seconds", &Font12, BLACK, WHITE);
    }
    else if (set_Info_Cursor == 3){
      Paint_DrawString_EN(170, 34, "30 Seconds", &Font12, BLACK, WHITE);
    }
    else if (set_Info_Cursor == 4){
      Paint_DrawString_EN(170, 34, "1 Minute", &Font12, BLACK, WHITE);
    }
    else if (set_Info_Cursor == 5){
      Paint_DrawString_EN(170, 34, "2 Minutes", &Font12, BLACK, WHITE);
    }
  }
}

void settings_Screen(){
  // Create a new display buffer
  Paint_NewImage(image, EPD_2in13_V4_WIDTH, EPD_2in13_V4_HEIGHT, 90, WHITE);
  // Paint the whole frame white
  Paint_Clear(WHITE);

  // Draw message selection screen
  Paint_DrawString_EN(0, 5, "Settings:", &Font16, BLACK, WHITE);

  if (settings_Cursor == 0){
  Paint_DrawRectangle(5,29,155,50,BLACK,DOT_PIXEL_2X2,DRAW_FILL_FULL);
  Paint_DrawString_EN(10, 34, "Sleep Timeout", &Font16, WHITE, WHITE);
  Paint_DrawRectangle(5,58,155,84,BLACK,DOT_PIXEL_2X2,DRAW_FILL_EMPTY);
  Paint_DrawString_EN(10, 63, "Mode", &Font16, BLACK, WHITE);
  }

  if (settings_Cursor == 1){
    Paint_DrawRectangle(5,29,155,50,BLACK,DOT_PIXEL_2X2,DRAW_FILL_EMPTY);
    Paint_DrawString_EN(10, 34, "Sleep Timeout", &Font16, BLACK, WHITE);
    Paint_DrawRectangle(5,58,155,84,BLACK,DOT_PIXEL_2X2,DRAW_FILL_FULL);
    Paint_DrawString_EN(10, 63, "Mode", &Font16, WHITE, WHITE);
    }
  //Paint_DrawString_EN(5, 82, "Mode", &Font16, BLACK, WHITE);

  // Display cursor
  //Paint_DrawString_EN(0, 34, ">", &Font16, BLACK, WHITE);
}

void handle_button_callback(uint gpio, uint32_t events) {
  if (screen != SCREEN_IDLE || (state != STATE_IDLE && state != STATE_RX))
    return;

  if (gpio == PIN_BUTTON_NEXT) {// add switch cases for each state
    switch (display) {
      case DISPLAY_HOME:
          printf("On Home Screen.\n"); //For testing purposes
          // Add selection drawings for home screen
          home_Cursor = (home_Cursor + 1) % 3;
          home_Screen();
          printf("new msgs: %d\n",new_Msg); //For testing purposes
          screen = SCREEN_DRAW_READY;
          break;
      case DISPLAY_RXMSG:
          printf("On Received Messages Screen.\n"); //For testing purposes
          // Reset cursor if moving to new page
          if (received_Cursor == 2 && (msg_Number- (received_Page-1)*3)> 3){
            received_Page++;
            received_Cursor = 0;
          }
          else {
            received_Cursor = (received_Cursor + 1) % (msg_Number- (received_Page-1)*3);
          }
          // Display cursor
          received_Msgs();
          screen = SCREEN_DRAW_READY;
          break;

      case DISPLAY_MSG:
          printf("On Message Screen.\n"); //For testing purposes
          message_Cursor = (message_Cursor + 1) % 3;
          for (int i = 0; i < 3; i++) {
            Paint_ClearWindows(0, 34 + i * 24, 20, 58 + i * 24, WHITE);
          }
          Paint_DrawString_EN(0, 34 + message_Cursor * 24, ">", &Font16, BLACK, WHITE);
          screen = SCREEN_DRAW_READY;
          break;

      case DISPLAY_SETTINGS:
          printf("On Settings Screen.\n"); //For testing purposes
          // Add selection drawings for settings screen
          settings_Cursor = (settings_Cursor + 1) % 2;
          settings_Screen();
          screen = SCREEN_DRAW_READY;
          break;
      case DISPLAY_SETTINGS_INFO:
          printf("On Settings Info Screen.\n"); //For testing purposes
          // Add selection drawings for settings screen
          set_Info_Cursor = (set_Info_Cursor + 1) % 6;
          settings_Info();
          screen = SCREEN_DRAW_READY;
          break;

      case DISPLAY_NEIGHBOURS:
          printf("On Neighbours Screen.\n"); //For testing purposes
          // Add selection drawings for neighbours screen
          break;
      }
  } else if (gpio == PIN_BUTTON_OK) {
    switch (display) {
      case DISPLAY_HOME:
          //printf("On Home Screen."); //For testing purposes
          // Add selection drawings for home screen
          if (home_Cursor == 0){ //Go to msg screen
            display = DISPLAY_RXMSG;
            received_Cursor = 0;
            received_Msgs();
            refresh_Counter = 15;
            screen = SCREEN_DRAW_READY;
          } 
          if (home_Cursor == 1){ //Go to settings screen
            display = DISPLAY_SETTINGS;
            settings_Screen();
            refresh_Counter = 15;
            screen = SCREEN_DRAW_READY;
          }
          if (home_Cursor == 2){ //Go to neighbours screen
            display = DISPLAY_NEIGHBOURS;
            neighbours_Screen();
            refresh_Counter = 15;
            screen = SCREEN_DRAW_READY;
          }
          break;
      case DISPLAY_RXMSG:
          // Add selection drawings for received messages screen
          break;

      case DISPLAY_MSG:
          //printf("Send Msg."); //For testing purposes
          state = STATE_TX_READY;
          break;

      case DISPLAY_SETTINGS:
          //printf("On Settings Screen."); //For testing purposes
          // Add selection drawings for settings screen
          temp_Cursor = set_Info_Cursor;
          display = DISPLAY_SETTINGS_INFO;
          settings_Info();
          //refresh_Counter = 15;
          screen = SCREEN_DRAW_READY;
          break;

      case DISPLAY_SETTINGS_INFO:
          //Set display timeout
          settings_Screen();
          display = DISPLAY_SETTINGS;
          refresh_Counter = 15;
          screen = SCREEN_DRAW_READY;
          if (set_Info_Cursor == 0){
            display_Timeout = 0;
          }
          if (set_Info_Cursor == 1){
            display_Timeout = 5000;
          }
          if (set_Info_Cursor == 2){
            display_Timeout = 10000;
          }
          if (set_Info_Cursor == 3){
            display_Timeout = 30000;
          }
          if (set_Info_Cursor == 4){
            display_Timeout = 60000;
          }
          if (set_Info_Cursor == 5){
            display_Timeout = 120000;
          }
          break;

      case DISPLAY_NEIGHBOURS:
          //printf("On Neighbours Screen."); //For testing purposes
          // Add selection drawings for neighbours screen
          break;
      }
  } else if (gpio == PIN_BUTTON_BACK) {
    switch (display) {
      case DISPLAY_HOME:
          printf("Already at home\n"); //For testing purposes
          // Add selection drawings for home screen
          sprintf(test,"TEST %d",msg_Number);
          strcpy(saved_Messages[msg_Number], test);
          new_Messages[msg_Number] = 1;
          msg_Number++;
          new_Msg++;
          screen = SCREEN_DRAW_READY;

          break;
      
      case DISPLAY_RXMSG:
          // Add selection drawings for received messages screen
          printf("Going home.\n"); //For testing purposes
          display = DISPLAY_HOME;
          received_Page = 1;
          home_Screen();
          refresh_Counter = 15;
          screen = SCREEN_DRAW_READY;
          break;

      case DISPLAY_MSG:
          //printf("Send Msg."); //For testing purposes
          display = DISPLAY_HOME;
          home_Screen();
          refresh_Counter = 15;
          screen = SCREEN_DRAW_READY;
          break;

      case DISPLAY_SETTINGS:
          //printf("On Settings Screen."); //For testing purposes
          // Add selection drawings for settings screen
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

      case DISPLAY_NEIGHBOURS:
          //printf("On Neighbours Screen."); //For testing purposes
          // Add selection drawings for neighbours screen
          display = DISPLAY_HOME;
          home_Screen();
          refresh_Counter = 15;
          screen = SCREEN_DRAW_READY;
          break;
      }
  }
}

void handle_irq_callback(uint gpio, uint32_t events) {
  if (gpio == PIN_DIO1) {
    handle_dio1_callback(gpio, events);
  } else if (gpio == PIN_BUTTON_NEXT || gpio == PIN_BUTTON_OK || gpio == PIN_BUTTON_BACK) {
    handle_button_callback(gpio, events);
  }
}

void setup_io() {
  // Initialize the uart for printing from the pico.
  stdio_init_all();

  // Wait until the usb is ready to transmit.
  while (!tud_cdc_connected()) {
    sleep_ms(100);
  }

  // Initialize the SPI interface of the pico (pins defined in the src/pico_config.h).
  spi_t spi = pico_spi_init(SPI_PORT, PIN_MISO, PIN_MOSI, PIN_SCLK, PIN_NSS);

  // Initialize other pins required to use the radio (pins defined in the src/pico_config.h).
  // `context` is used throughout the code to keep track of the configuration of our setup.
  context.spi = spi;
  context.nss = pico_gpio_init(PIN_NSS, GPIO_FUNC_SIO, GPIO_DIR_OUT, GPIO_PULL_NONE, 1);
  context.busy = pico_gpio_init(PIN_BUSY, GPIO_FUNC_SIO, GPIO_DIR_IN, GPIO_PULL_NONE, 0);
  context.reset = pico_gpio_init(PIN_RESET, GPIO_FUNC_SIO, GPIO_DIR_OUT, GPIO_PULL_NONE, 1);
  context.dio1 = pico_gpio_init(PIN_DIO1, GPIO_FUNC_SIO, GPIO_DIR_IN, GPIO_PULL_NONE, 0);

  // Enable the interrupts and set the callback function for DIO1 pin.
  pico_gpio_set_interrupt(context.dio1.pin, GPIO_IRQ_EDGE_RISE, &handle_irq_callback);

  // Initialize the buttons and set the callback function for each button.
  pico_gpio_init(PIN_BUTTON_NEXT, GPIO_FUNC_SIO, GPIO_DIR_IN, GPIO_PULL_UP, 1);
  pico_gpio_init(PIN_BUTTON_OK, GPIO_FUNC_SIO, GPIO_DIR_IN, GPIO_PULL_UP, 1);
  pico_gpio_init(PIN_BUTTON_BACK, GPIO_FUNC_SIO, GPIO_DIR_IN, GPIO_PULL_UP, 1);
  pico_gpio_set_interrupt(PIN_BUTTON_NEXT, GPIO_IRQ_EDGE_FALL, &handle_irq_callback);
  pico_gpio_set_interrupt(PIN_BUTTON_OK, GPIO_IRQ_EDGE_FALL, &handle_irq_callback);
  pico_gpio_set_interrupt(PIN_BUTTON_BACK, GPIO_IRQ_EDGE_FALL, &handle_irq_callback);

  // Light up the onboard LED to signify setup is done.
  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
  gpio_put(PICO_DEFAULT_LED_PIN, 1);
}

void setup_display() {
  DEV_Module_Init();
  EPD_2in13_V4_Init();
  EPD_2in13_V4_Clear();
  EPD_2in13_V4_Sleep();
}

void setup_sx126x() {
  // Try to read a register with a known reset value from the radio.
  // If this fails, either SPI connection is not setup correctly or the radio is not responding.
  uint8_t reg = 0;
  sx126x_read_register(&context, 0x0740, &reg, 1);
  if (reg != 0x14) {
    printf("sanity check failed: %d\n", reg);
    while (true) {
      tight_loop_contents();
    }
  }

  // The radio is using TCXO. The DIO3 pin needs to be setup as a voltage source for the TCXO.
  // Before this step, the radio will fail to start the XOSC. This is expected so, we clear the
  // errors.
  sx126x_clear_device_errors(&context);
  sx126x_set_dio3_as_tcxo_ctrl(&context, SX126X_TCXO_CTRL_1_7V, 5 << 6);

  // With the TCXO correctly configured now, re-calibrate all the clock on the chip.
  sx126x_cal_mask_t calibration_mask = 0x7F;
  sx126x_cal(&context, calibration_mask);

  // Make sure the calibration succeeded.
  sx126x_errors_mask_t errors = 0;
  sx126x_chip_status_t status = {.chip_mode = 0, .cmd_status = 0};
  sx126x_get_status(&context, &status);
  sx126x_get_device_errors(&context, &errors);
  if (!(status.chip_mode == SX126X_CHIP_MODE_STBY_RC &&
        status.cmd_status == SX126X_CMD_STATUS_RFU && errors == 0)) {
    printf("calibration failed\n");
    while (true) {
      tight_loop_contents();
    }
  }

  // Setup the radio for LORA at 915MHz.
  sx126x_set_standby(&context, SX126X_STANDBY_CFG_RC);
  sx126x_set_pkt_type(&context, SX126X_PKT_TYPE_LORA);
  sx126x_set_rf_freq(&context, 915000000);

  // Setup power amplifier settings for TX.
  sx126x_pa_cfg_params_t pa_cfg = {
      .pa_duty_cycle = 0x04,
      .hp_max = 0x07,
      .device_sel = 0x00,
      .pa_lut = 0x01,
  };
  sx126x_set_pa_cfg(&context, &pa_cfg);
  sx126x_set_tx_params(&context, 0x16, SX126X_RAMP_40_US);

  // Setup the modulation parameters for LORA.
  sx126x_mod_params_lora_t mod_params = {
      .sf = SX126X_LORA_SF7,
      .bw = SX126X_LORA_BW_250,
      .cr = SX126X_LORA_CR_4_5,
      .ldro = 0,
  };
  sx126x_set_lora_mod_params(&context, &mod_params);

  // Setup the packet parameters for LORA.
  sx126x_pkt_params_lora_t packet_params = {
      .preamble_len_in_symb = 0x10,
      .header_type = SX126X_LORA_PKT_EXPLICIT,
      .pld_len_in_bytes = 0x20,
      .crc_is_on = true,
      .invert_iq_is_on = false,
  };
  sx126x_set_lora_pkt_params(&context, &packet_params);

  // Setup the DIO1 pin to trigger for all interrupts.
  sx126x_set_dio_irq_params(&context, 0xFFFF, 0xFFFF, 0x0000, 0x0000);
}

// Transmit bytes over the radio.
void transmit_bytes(uint8_t *bytes, uint8_t length) {
  if (length > PAYLOAD_BUFFER_SIZE) {
    printf("payload too large\n");
    return;
  }

  // Set the radio to standby mode, in case we are in receive.
  sx126x_set_standby(&context, SX126X_STANDBY_CFG_RC);

  memcpy(payload_buf, bytes, length);

  // Write the payload to the radio's buffer.
  sx126x_write_buffer(&context, 0, payload_buf, length);

  // Setup the packet parameters for the transmission. Only length is needed to be updated.
  sx126x_pkt_params_lora_t packet_params = {
      .preamble_len_in_symb = 0x10,
      .header_type = SX126X_LORA_PKT_EXPLICIT,
      .pld_len_in_bytes = length,
      .crc_is_on = true,
      .invert_iq_is_on = false,
  };
  sx126x_set_lora_pkt_params(&context, &packet_params);

  // Start the transmission.
  sx126x_set_tx(&context, 0x0);
  sx126x_check(&context);
  state = STATE_TX;
}

// Transmit a string over the radio. Must be null terminated.
void transmit_string(char *string) { transmit_bytes((uint8_t *)string, strlen(string)); }

void receive_once() {
  sx126x_pkt_params_lora_t packet_params = {
      .preamble_len_in_symb = 0x10,
      .header_type = SX126X_LORA_PKT_EXPLICIT,
      .pld_len_in_bytes = 0x20,
      .crc_is_on = true,
      .invert_iq_is_on = false,
  };
  sx126x_set_lora_pkt_params(&context, &packet_params);
  sx126x_set_rx(&context, 0);
}

void receive_cont() {
  sx126x_pkt_params_lora_t packet_params = {
      .preamble_len_in_symb = 0x10,
      .header_type = SX126X_LORA_PKT_EXPLICIT,
      .pld_len_in_bytes = 0x20,
      .crc_is_on = true,
      .invert_iq_is_on = false,
  };
  sx126x_set_lora_pkt_params(&context, &packet_params);
  sx126x_set_rx_with_timeout_in_rtc_step(&context, SX126X_RX_CONTINUOUS);
}

// Function to be called after 5 seconds if the flag isn't set
int64_t alarm_callback(alarm_id_t id, void *user_data) {
  five_Seconds = true;
  printf("%d Seconds of inactivity, sleeping display.\n", display_Timeout/1000);
  Paint_SelectImage(image);
  //partial display refresh
  Paint_DrawString_EN(230, 0, "SLP", &Font12, WHITE, BLACK);
  EPD_2in13_V4_Display_Partial(image);
  sleep_ms(100);
  EPD_2in13_V4_Sleep();
  sleep_ms(100);
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

void wakeup_Screen(){
  // Create a new display buffer
  Paint_NewImage(wakeup, EPD_2in13_V4_WIDTH, EPD_2in13_V4_HEIGHT, 90, WHITE);
  // Paint the whole frame white
  Paint_Clear(WHITE);

  // Draw message selection screen
  Paint_DrawString_EN(50, 50, "VoidLink", &Font24, BLACK, WHITE);
  EPD_2in13_V4_Init_Fast();
  EPD_2in13_V4_Display_Fast(wakeup);
  sleep_ms(200);

}

void core1_entry() {
  wakeup_Screen();
  home_Screen();
  screen = SCREEN_DRAW_READY;

  while (true) {
    uint32_t flag = multicore_fifo_pop_blocking();

    if (flag == 0) {
      //Fast refresh on display wakup
      if (five_Seconds){
        EPD_2in13_V4_Init_Fast();
        printf("Waking display.\n");
        Paint_ClearWindows(250, 0, 350, 20, WHITE);
        EPD_2in13_V4_Display_Fast(image);
      }
      if (refresh_Counter == 15) {
        EPD_2in13_V4_Display_Base(image);
        refresh_Counter = 0;
      }
      else {
        EPD_2in13_V4_Display_Partial(image);
        refresh_Counter++;
      }
      //printf("Updating Image\n");
      sleep_ms(100);
      //Set alarm to sleep display after 7 seconds of inactivity
      set_flag_and_reset_alarm();
      multicore_fifo_push_blocking_inline(0);
    }
    
  }
}

int main() {
  sx126x_errors_mask_t errors = 0;
  sx126x_chip_status_t status = {.chip_mode = 0, .cmd_status = 0};

  setup_io();
  setup_display();
  setup_sx126x();
  memset(new_Messages, 0, sizeof(new_Messages));

  multicore_launch_core1(core1_entry);

  printf("setup done\n");

  while (true) {
    if (state == STATE_TX_READY) {
      transmit_string(messages[message_Cursor]);
    } else if (state == STATE_TX_DONE) {
      state = STATE_IDLE;
    } else if (state == STATE_IDLE) {
      receive_cont();
      state = STATE_RX;
    }

    if (screen == SCREEN_DRAW_READY) {
      multicore_fifo_push_blocking_inline(0);
      screen = SCREEN_DRAW;
    } else if (screen == SCREEN_DRAW) {
      if (multicore_fifo_rvalid()) {
        multicore_fifo_pop_blocking_inline();
        screen = SCREEN_IDLE;
      }
    }
  }
}
