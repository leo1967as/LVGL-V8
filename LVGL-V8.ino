/*Using LVGL with Arduino requires some extra steps:
  Be sure to read the docs here: https://docs.lvgl.io/master/get-started/platforms/arduino.html  */

#include "WiFi.h"
#include <lvgl.h>
#include <TFT_eSPI.h>
#include "MonitorPage.h"
#include <set>
#include <vector>
#include <EEPROM.h>
#include <ESP.h>


#define configCHECK_FOR_STACK_OVERFLOW  2
#define configASSERT( x ) if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for( ;; ); }
#define configUSE_HEAP_OVERFLOW_CHECK 1

bool wifiConnected = false;
// WiFiCredentials taskCredentials; // Global instance of WiFiCredentials
/* Screen Resolution */
static const uint16_t screenWidth = 480;
static const uint16_t screenHeight = 320;

/* Display buffer and TFT instance */
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * screenHeight / 10];
static lv_obj_t *password_ta;

#define WHITE 0xFFFF

TFT_eSPI tft = TFT_eSPI(screenWidth, screenHeight); /* TFT instance */

#if LV_USE_LOG != 0
/* Serial debugging */
void my_print(const char *buf) {
  Serial.printf(buf);
  Serial.flush();
}
#endif

#if LV_USE_LOG != 0
void print_memory_info(void) {
  lv_mem_monitor_t mon;
  lv_mem_monitor(&mon);
  Serial.printf("Total: %d, Used: %d, Frag: %d\n", mon.total_size, mon.used_size, mon.frag_rate);
}
#endif
// Screen and Buttons
static lv_obj_t *Screen1, *Screen2;
static lv_obj_t *button1, *buttonchangescreen, *buttonchangescreen2;
static lv_obj_t *settings;
static lv_obj_t *settinglabel, *settinglabel2, *settinglabel3;
static lv_obj_t *alcohol;
static lv_obj_t *lines;

// Styles and Labels
static lv_style_t border_style;

// Setting Widgets
static lv_obj_t *settingBtn, *settingCloseBtn;
static lv_obj_t *settingWiFiSwitch, *wfList;
lv_obj_t *network_list = NULL;
lv_obj_t *list = NULL;

// Message Box Widgets
static lv_obj_t *mboxConnect, *mboxTitle, *mboxPassword;
static lv_obj_t *mboxConnectBtn, *mboxCloseBtn;

// Keyboard and Input
static lv_obj_t *keyboard;
static lv_obj_t *active_text_area;
static lv_group_t *group;
static lv_obj_t *input;

// Popup Box and Timer
static lv_timer_t *timer;
static lv_obj_t *popupWait, *msgpopupWait;

// Task
TaskHandle_t ntScanTaskHandler, ntConnectTaskHandler , scanNetworksTaskHandle;
String selectedSSID, password;
////////////////
void btn_event_handler(lv_event_t *btn) {
  lv_event_code_t code = lv_event_get_code(btn);
  if (code == LV_EVENT_CLICKED) {
    Serial.println("Button clicked!");
    LV_LOG_USER("Clicked button");
  }
}

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)&color_p->full, w * h, true);
  tft.endWrite();

  lv_disp_flush_ready(disp_drv);
}

/*Read the touchpad*/
void my_touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
  uint16_t touchX, touchY;

  bool touched = tft.getTouch(&touchX, &touchY, 600);

  if (!touched) {
    data->state = LV_INDEV_STATE_REL;
  } else {
    data->state = LV_INDEV_STATE_PR;

    /*Set the coordinates*/
    data->point.x = touchX;
    data->point.y = touchY;

    Serial.print("Data x ");
    Serial.println(touchX);

    Serial.print("Data y ");
    Serial.println(touchY);
  }
}

void btn_event_change_screen(lv_event_t *btn) {
  lv_event_code_t event_code = lv_event_get_code(btn);

  if (event_code == LV_EVENT_CLICKED) {
    // Check the current parent of Screen1 and Screen2
    lv_obj_t *parent_of_screen1 = lv_obj_get_parent(Screen1);
    lv_obj_t *parent_of_screen2 = lv_obj_get_parent(Screen2);

    // Toggle the screens
    if (parent_of_screen1 == NULL) {
      // Attach Screen2 to Screen1 and load Screen2
      lv_obj_set_parent(Screen2, Screen1);
      lv_scr_load(Screen2);
      Serial.println("Switched to Screen2");
    } else if (parent_of_screen2 == NULL) {
      // Attach Screen1 to Screen2 and load Screen1
      lv_obj_set_parent(Screen1, Screen2);
      lv_scr_load(Screen1);
      Serial.println("Switched to Screen1");
    }
  }
}

void btn_event_change_screen2(lv_event_t *btn2) {
  lv_event_code_t event_code = lv_event_get_code(btn2);

  if (event_code == LV_EVENT_CLICKED) {
    // Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED) {
      lv_obj_set_style_text_color(settinglabel, lv_color_hex(0x00FF00), LV_PART_MAIN);  // Green WIFI
      lv_label_set_text(settinglabel2, " WIFI CONNECTED ");
      lv_obj_set_style_text_color(settinglabel2, lv_color_hex(0x00FF00), LV_PART_MAIN);  // Green Connected
      Serial.println("WiFi GREEN");
    } else {
      lv_obj_set_style_text_color(settinglabel, lv_color_hex(0xFF0000), LV_PART_MAIN);  // Red WIFI
      lv_label_set_text(settinglabel2, " WIFI NOT CONNECT ");
      lv_obj_set_style_text_color(settinglabel2, lv_color_hex(0xFF0000), LV_PART_MAIN);  // Red Not Connected
      Serial.println("WiFi RED");
    }

    // Check the current parent of Screen2
    lv_obj_t *parent_of_screen2 = lv_obj_get_parent(Screen2);

    // Switch screens if Screen2 is not attached to Screen1
    if (parent_of_screen2 == NULL) {
      // Attach Screen2 to Screen1 and load Screen1
      lv_obj_set_parent(Screen2, Screen1);
      lv_scr_load(Screen1);
      Serial.println("Switched from Screen2 to Screen1");
    }
  }
}




/// WIFI THING
bool scanning_enabled = false;

void switch_event_handler(lv_event_t *event) {
  lv_event_code_t code = lv_event_get_code(event);
  lv_obj_t *switch_obj = lv_event_get_target(event);

  if (code == LV_EVENT_VALUE_CHANGED) {
    bool switch_state = lv_obj_has_state(switch_obj, LV_STATE_CHECKED);

    if (switch_state && !scanning_enabled) {
      // Turn on scanning
      scanning_enabled = true;
      lv_label_set_text(network_list, "Scanning for networks...");
      Serial.println("Scanning for networks...");
      xTaskCreatePinnedToCore(scanNetworksTask, "scanNetworksTask", 8192, NULL, 1, &scanNetworksTaskHandle, 1);
    } else if (!switch_state && scanning_enabled) {
      // Turn off scanning
      scanning_enabled = false;
      lv_label_set_text(network_list, "Turn on the switch to scan for networks");

      if (list) {
        lv_obj_del(list); // Remove the network list
        Serial.println("Network List Removed");
        list = NULL;
        // WiFi.scanDelete();
      }
    }
  }
}

void connectToWiFi(const char *selectedSSID, const char *password)
{
  Serial.print(" Using : ");
  Serial.println(String(selectedSSID));

  Serial.print(" Using : ");
  Serial.println(String(password));

  if (WiFi.begin(selectedSSID, password) == WL_CONNECTED)
  {
  }
  else
  {
    Serial.println("Waiting for WiFi connection...");
    unsigned long startTime = millis();
    const unsigned long connectionTimeout = 10000; // 10 seconds timeout
    bool connected = false;

    while (!connected && (millis() - startTime < connectionTimeout))
    {
      if (WiFi.status() == WL_CONNECTED)
      {
        Serial.println("WiFi connected");
        lv_obj_set_style_text_color(settinglabel, lv_color_hex(0x00FF00), LV_PART_MAIN); // Green WIFI
        lv_label_set_text(settinglabel2, " WIFI CONNECTED ");
        lv_obj_set_style_text_color(settinglabel2, lv_color_hex(0x00FF00), LV_PART_MAIN); // Green Connectded
        Serial.println("WiFi GREEN");
        wifiConnected = true; // Set the Wi-Fi connection status to true
        connected = true;
        saveCredentialEEPROM(selectedSSID, password);
      }
      else
      {
        Serial.println("tried");
        delay(500); // Wait for 1 second before checking again
      }
    }

    if (!connected)
    {
      Serial.println("WiFi connection failed");
      // TODO: Add additional error handling if needed
    }
  }
}

void scanNetworksTask(void *parameter) {
  // Scanning for networks
  Serial.println("SCAN");
  
  // Perform WiFi network scan
  int numNetworks = WiFi.scanNetworks();
  Serial.print("WiFi scan result: ");
  Serial.println(numNetworks);
  
  // Delay to allow the scan result to stabilize
  vTaskDelay(pdMS_TO_TICKS(10));
  
  if (numNetworks == 0) {
    // No networks found
    lv_label_set_text(network_list, "No networks found");
  } else {
    // Clear existing list if it exists
    if (list) {
      lv_obj_del(list);
      WiFi.disconnect();
    }

    // Create a new list to display network names
    list = lv_list_create(lv_scr_act());
    lv_obj_set_size(list, LV_HOR_RES, LV_VER_RES - 85);
    lv_obj_align(list, LV_ALIGN_CENTER, 0, 0);

    // Define a buffer to hold the network name
    char networkNameBuffer[34];  // 33 characters + null terminator

    // Iterate through scanned networks and add them to the list
    for (int i = 0; i < numNetworks; ++i) {
      memset(networkNameBuffer, 0, sizeof(networkNameBuffer));  // Clear buffer

      // Copy network name into buffer
      strncpy(networkNameBuffer, WiFi.SSID(i).c_str(), sizeof(networkNameBuffer) - 1);

      createNetworkItem(networkNameBuffer, i);

      // Delay to allow time for GUI to update
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }
  Serial.print("TASK DELETED ");  
  // Delete the task once done
  vTaskDelete(NULL);
}

void createNetworkItem(const char *networkName, int index) {
  lv_obj_t *network_item = lv_list_add_btn(list, LV_SYMBOL_WIFI, networkName);
  lv_obj_set_user_data(network_item, (void *)index);
  
  // Add event handler for short click on network item
  lv_obj_add_event_cb(
    network_item, [](lv_event_t *btn) {
      if (lv_event_get_code(btn) == LV_EVENT_SHORT_CLICKED) {
        lv_obj_t *btn_obj = lv_event_get_target(btn);
        int selected_index = (int)lv_obj_get_user_data(btn_obj);
        const char *selected_network = WiFi.SSID(selected_index).c_str();
        
        // Handle the selected network (e.g., connect to it)
        Serial.print("Selected network: ");
        Serial.println(selected_network);
        buildPWMsgBox(selected_network);  
      }
    },
    LV_EVENT_SHORT_CLICKED, NULL);
}


void setupWiFiSettings() {
  // Create and position the Wi-Fi switch
  lv_obj_t *switch_obj = lv_switch_create(Screen2);
  lv_obj_align(switch_obj, LV_ALIGN_TOP_RIGHT, -20, 10);
  lv_obj_add_event_cb(switch_obj, switch_event_handler, LV_EVENT_VALUE_CHANGED, NULL);

  // Create and configure the network list label
  network_list = lv_label_create(Screen2);
  lv_label_set_text(network_list, "Turn on the switch to scan for networks");
  lv_obj_align(network_list, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color(network_list, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
}



void networkConnectorMain() {
  // xTaskCreatePinnedToCore([](void *param) {
    const char *selectedSSID = lv_label_get_text(mboxTitle);

    // Remove the prefix "Selected WiFi SSID: " if it exists
    const char *prefix = "WiFi SSID: ";
    if (strncmp(selectedSSID, prefix, strlen(prefix)) == 0) {
      selectedSSID += strlen(prefix);
    }

    const char *password = lv_textarea_get_text(mboxPassword);

    connectToWiFi(selectedSSID, password);
//    vTaskDelete(NULL);  // Task finished, delete itself
  // },
  //             "connectToWiFi", 8192, NULL, 1, NULL,1);
}

/// //////////////////////////////////////////////////////////////////////////

void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  // Initialize Wi-Fi in station mode, disconnect, and disable auto-reconnect
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.setAutoReconnect(false);
  
  // Short delay for stabilization
  delay(100);

  // Initialize the graphical user interface
  guiTask();

  // Attempt to connect to a previously saved network
  tryPreviousNetwork();
}


void loop() {
  lv_timer_handler(); /* let the GUI do its work */
  delay(5);
}

void guiTask() {
  Serial.println("I am LVGL_Arduino");
  Serial.println("Setting up LVGL and GUI");
  initlvgl();

  Screen1 = lv_obj_create(NULL);

  lv_scr_load(Screen1);
  lv_obj_set_style_bg_color(Screen1, lv_color_hex(000000), LV_STATE_DEFAULT);
  Screen2 = lv_label_create(NULL);
  lv_obj_set_style_bg_color(Screen2, lv_color_hex(000000), LV_STATE_DEFAULT);

  /////Initi Keyboard
  makeKeyboard();

  ////Setup GUI Grid, Element ,Settings Icon
  setupElementName();

  setupWiFiStatusUI();

  setupWiFiSettingsButton();

  setupBackButton();

  setupWiFiManagerUI();

  setupWiFiSettings();

  Serial.println("Setup GUI done");
}

//////
//**Build Popup Wifi Settings
//////
static void buildPWMsgBox(const char *selectedSSID) {
  // Create the main message box
  mboxConnect = lv_obj_create(lv_scr_act());
  lv_obj_add_style(mboxConnect, &border_style, 0);
  lv_obj_set_size(mboxConnect, tft.width() * 2 / 3, tft.height() / 2);
  lv_obj_center(mboxConnect);

  // Create and set the title label
  mboxTitle = lv_label_create(mboxConnect);
  char titleText[50];  // Adjust the buffer size as needed
  snprintf(titleText, sizeof(titleText), "WiFi SSID: %s", selectedSSID);
  lv_label_set_text(mboxTitle, titleText);
  lv_obj_align(mboxTitle, LV_ALIGN_TOP_LEFT, 0, 0);

  // Create the password input textarea
  mboxPassword = lv_textarea_create(mboxConnect);
  lv_obj_set_size(mboxPassword, tft.width() / 2, 40);
  lv_obj_align_to(mboxPassword, mboxTitle, LV_ALIGN_TOP_LEFT, 0, 30);
  lv_textarea_set_placeholder_text(mboxPassword, "Password?");
  lv_obj_add_event_cb(mboxPassword, text_input_event_cb, LV_EVENT_ALL, keyboard);

  // Create the "Connect" button
  mboxConnectBtn = lv_btn_create(mboxConnect);
  lv_obj_set_size(mboxConnectBtn, 100, 40);
  lv_obj_align_to(mboxConnectBtn, mboxPassword, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
  lv_obj_t *btnLabel = lv_label_create(mboxConnectBtn);
  lv_label_set_text(btnLabel, "Connect");
  lv_obj_center(btnLabel);

  // Attach event handler to "Connect" button
  lv_obj_add_event_cb(
    mboxConnectBtn, [](lv_event_t *btn) {
      if (lv_event_get_code(btn) == LV_EVENT_CLICKED) {
        // Get Password and SSID from selected
        const char *password = lv_textarea_get_text(mboxPassword);
        const char *selectedSSID = lv_label_get_text(mboxTitle);

        // Remove the prefix "WiFi SSID: " if it exists
        const char *prefix = "WiFi SSID: ";
        if (strncmp(selectedSSID, prefix, strlen(prefix)) == 0) {
          selectedSSID += strlen(prefix);
        }

        Serial.println(selectedSSID);
        Serial.println(password);

        // networkConnectorMain();
        connectToWiFi(selectedSSID, password);
        Serial.print("Try To Del Connect");
        delay(1000);
        lv_obj_del(mboxConnect);  // Close the message box
      }
    },
    LV_EVENT_CLICKED, NULL);

  // Create the "Cancel" button
  lv_obj_t *mboxCloseBtn = lv_btn_create(mboxConnect);
  lv_obj_set_size(mboxCloseBtn, 100, 40);
  lv_obj_align_to(mboxCloseBtn, mboxPassword, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 10);
  lv_obj_t *closeLabel = lv_label_create(mboxCloseBtn);
  lv_label_set_text(closeLabel, "Cancel");
  lv_obj_center(closeLabel);

  // Attach event handler to "Cancel" button
  lv_obj_add_event_cb(
    mboxCloseBtn, [](lv_event_t *btn) {
      if (lv_event_get_code(btn) == LV_EVENT_CLICKED) {
        lv_obj_del(mboxConnect);  // Close the message box
      }
    },
    LV_EVENT_CLICKED, NULL);
}

//////
//**Build Keyboard And Keyboard Event Handler
//////
// Function to create and set up the keyboard
void makeKeyboard() {
  // Create the keyboard object
  keyboard = lv_keyboard_create(lv_scr_act());
  lv_obj_align(keyboard, LV_ALIGN_CENTER, 0, 0);  // Align at the center of the screen
  lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);  // Initially hide the keyboard
}

// Focus event callback for text input
static void text_input_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *ta = lv_event_get_target(e);

  if (code == LV_EVENT_FOCUSED) {
    // Create and set up the keyboard when the text area is focused
    keyboard = lv_keyboard_create(lv_scr_act());
    lv_obj_set_size(keyboard, LV_HOR_RES, LV_VER_RES / 2.20);  // Adjust the size as needed
    lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_keyboard_set_textarea(keyboard, ta);
    lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    Serial.print("OPEN KeyBoard");
    active_text_area = ta;
  }

  if (code == LV_EVENT_DEFOCUSED) {
    // Remove keyboard association when the text area loses focus
    lv_keyboard_set_textarea(keyboard, NULL);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    Serial.print("CLOSE KeyBoard");
    active_text_area = NULL;
  }
}


// void setupWiFiSettings()
// {
//   Serial.println("Setting up WIFI");
//   lv_obj_t *network_list = lv_label_create(Screen2);
//   lv_label_set_text(network_list, "Scanning for networks...");
//   lv_obj_align(network_list, LV_ALIGN_CENTER, 0, 0);
//   lv_obj_set_style_text_color(network_list, lv_color_hex(0xFFFFFF), LV_PART_MAIN);

//   int numNetworks = WiFi.scanNetworks();
//   if (numNetworks == 0) {
//     lv_label_set_text(network_list, "No networks found");
//   } else {
//     lv_obj_del(network_list);  // Remove the "Scanning for networks..." label

//     lv_obj_t *list = lv_list_create(Screen2);
//     lv_obj_set_size(list, LV_HOR_RES, LV_VER_RES - 85);
//     lv_obj_align(list, LV_ALIGN_CENTER, 0, 0);

//     for (int i = 0; i < numNetworks; ++i) {
//       const char *networkName = WiFi.SSID(i).c_str();
//       lv_obj_t *network_item = lv_list_add_btn(list, LV_SYMBOL_WIFI, networkName);

//       // Store the selected index as user data
//       lv_obj_set_user_data(network_item, (void *)i);

//       lv_obj_add_event_cb(
//         network_item, [](lv_event_t *btn) {
//           if (lv_event_get_code(btn) == LV_EVENT_SHORT_CLICKED) {
//             lv_obj_t *btn_obj = lv_event_get_target(btn);
//             int selected_index = (int)lv_obj_get_user_data(btn_obj);
//             const char *selected_network = WiFi.SSID(selected_index).c_str();
//             // Handle the selected network (e.g., connect to it)

//             Serial.print("Selected network: ");
//             Serial.println(selected_network);
//             buildPWMsgBox(selected_network);
//           }
//         },
//         LV_EVENT_SHORT_CLICKED, NULL);
//     }
//   }
//     Serial.println("Setup WIFI done");
// }

void deleteTask(void *task) {
  vTaskDelete(task);
}


//////
//**EEPROM PART
//////
struct WiFiCredentials {
  char selectedSSID[32];
  char password[64];
};

const int EEPROM_SIZE = 512;  // Adjust the EEPROM size as neede
const int SSID_EEPROM_ADDR = 0;
const int PASSWORD_EEPROM_ADDR = SSID_EEPROM_ADDR + sizeof(WiFiCredentials);
void clearEEPROM(int startAddr, int size) {
  for (int i = startAddr; i < startAddr + size; i++) {
    EEPROM.write(i, 0);
  }
}


void saveCredentialEEPROM(const char *selectedSSID, const char *password) {
    EEPROM.begin(EEPROM_SIZE);
  WiFiCredentials credentials;
  strncpy(credentials.selectedSSID, selectedSSID, sizeof(credentials.selectedSSID) - 1);
  credentials.selectedSSID[sizeof(credentials.selectedSSID) - 1] = '\0'; // Ensure null-termination
  strncpy(credentials.password, password, sizeof(credentials.password) - 1);
  credentials.password[sizeof(credentials.password) - 1] = '\0'; // Ensure null-termination



  EEPROM.put(SSID_EEPROM_ADDR, credentials);
  EEPROM.commit();
  EEPROM.end();

  Serial.print("Stored SSID: ");
  Serial.println(credentials.selectedSSID);

  Serial.print("Stored Password: ");
  Serial.println(credentials.password);

  Serial.println();
}



void loadCredentialsFromEEPROM(struct WiFiCredentials *credentials) {
  Serial.println("Start LOAD WIFI");
  EEPROM.begin(512);

  EEPROM.get(SSID_EEPROM_ADDR, *credentials);

  String storedSSID = credentials->selectedSSID;
  String storedPassword = credentials->password;

  Serial.print("Loaded SSID: ");
  Serial.println(storedSSID);

  Serial.print("Loaded Password: ");
  Serial.println(storedPassword);

  if (storedSSID.length() > 0) {
    int numNetworks = WiFi.scanNetworks();
    bool foundMatch = false;

    for (int i = 0; i < numNetworks; i++) {
      if (WiFi.SSID(i) == storedSSID) {
        foundMatch = true;
        break;
      }
    }

    if (foundMatch) {
      Serial.println("Found Stored SSID: " + storedSSID);
      Serial.println("Found Stored Password: " + storedPassword);

      if (storedPassword.length() > 0) {
        Serial.println("Connecting to WiFi...");
        connectToWiFi(credentials->selectedSSID, credentials->password);
        
      } else {
        Serial.println("Password is empty. Not connecting.");
      }
    } else {
      Serial.println("Stored SSID does not match any current networks. Not loading credentials.");
    }
  } else {
    Serial.println("Stored SSID is empty. Not loading credentials.");
  }

  EEPROM.end();
}


// void wifiTaskWrapper(void *param) {

//   struct WiFiCredentials *credentials = (struct WiFiCredentials *)param;
//    EEPROM.get(SSID_EEPROM_ADDR, *credentials);
//   connectToWiFi(credentials->selectedSSID, credentials->password);
//   vTaskDelete(NULL); // Delete the task
// }

void tryPreviousNetwork() {
  if (!EEPROM.begin(512)) {
    delay(1000);
    ESP.restart();
  }

  struct WiFiCredentials credentials;
  loadCredentialsFromEEPROM(&credentials);
}





// void networkConnector(void *param) {
//   xTaskCreate(
//     wifiTaskWrapper,   // Function to run
//     "beginWIFITask",   // Task name
//     10000,              // Stack size
//     param,             // Task parameters
//     2,                 // Priority
//     NULL           // Run on core 0
//   );
// }




//////
//**NETWORK Tasks
//////

void setupWiFiStatusUI() {
  settings = lv_obj_create(lv_scr_act());
  lv_obj_set_size(settings, 145, 30);
  lv_obj_set_style_text_color(settings, lv_color_hex(0xFFFFFF), LV_PART_MAIN);  // Black
  lv_obj_align(settings, LV_ALIGN_TOP_RIGHT, 0, 0);

  settinglabel = lv_label_create(Screen1);
  lv_label_set_text(settinglabel, LV_SYMBOL_WIFI);
  lv_obj_set_style_text_color(settinglabel, lv_color_hex(0xFF0000), LV_PART_MAIN);  // Red
  lv_obj_align(settinglabel, LV_ALIGN_TOP_RIGHT, -10, 7);

  settinglabel2 = lv_label_create(Screen1);
  lv_label_set_text(settinglabel2, " WIFI NOT CONNECT ");
  lv_obj_set_style_text_font(settinglabel2, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(settinglabel2, lv_color_hex(0xFF0000), LV_PART_MAIN);  // Red
  lv_obj_align(settinglabel2, LV_ALIGN_TOP_RIGHT, -32, 10);
}

void setupWiFiSettingsButton() {
  buttonchangescreen = lv_btn_create(Screen1);
  lv_obj_set_style_radius(buttonchangescreen, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_size(buttonchangescreen, 70, 70);
  lv_obj_set_style_bg_color(buttonchangescreen, lv_color_make(0, 255, 0), 0);
  lv_obj_align(buttonchangescreen, LV_ALIGN_BOTTOM_RIGHT, -25, -20);

  settinglabel3 = lv_label_create(buttonchangescreen);
  lv_label_set_text(settinglabel3, LV_SYMBOL_WIFI);
  lv_obj_set_style_text_font(settinglabel3, &lv_font_montserrat_30, LV_PART_MAIN);
  lv_obj_set_style_text_color(settinglabel3, lv_color_hex(000000), LV_PART_MAIN);  // Red
  lv_obj_align(settinglabel3, LV_ALIGN_CENTER, 0, 0);
    ////////// Event Handler For Change Screen
  lv_obj_add_event_cb(buttonchangescreen, btn_event_change_screen, LV_EVENT_ALL, NULL);

}

void setupBackButton() {
  lv_obj_t *buttonchangescreen2 = lv_btn_create(Screen2);
  lv_obj_set_size(buttonchangescreen2, 100, 50);
  lv_obj_align(buttonchangescreen2, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
  lv_obj_set_style_bg_color(buttonchangescreen2, lv_color_hex(0xFFFFFF), LV_PART_MAIN);

  lv_obj_t *wifi_label = lv_label_create(buttonchangescreen2);
  lv_label_set_text(wifi_label, "Back");
  lv_obj_align(wifi_label, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color(wifi_label, lv_color_hex(0x00000), LV_PART_MAIN);  // Red
    lv_obj_add_event_cb(buttonchangescreen2, btn_event_change_screen2, LV_EVENT_ALL, NULL);
}

void setupWiFiManagerUI() {
  lv_obj_t *WifimanagerLabel = lv_label_create(Screen2);
  lv_label_set_text(WifimanagerLabel, " ");
  lv_obj_align(WifimanagerLabel, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color(WifimanagerLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN);

  lv_obj_t *bar = lv_bar_create(Screen2);
  lv_obj_set_size(bar, LV_HOR_RES, 50);
  lv_obj_set_style_radius(bar, 0, LV_PART_MAIN);
  lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_bg_color(bar, lv_color_hex(0xFFFFFF), LV_PART_MAIN);

  lv_obj_t *bar_label = lv_label_create(bar);
  lv_label_set_text(bar_label, "WiFi Setting");
  lv_obj_align(bar_label, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color(bar_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
}

void setupElementName() {

  /// Element
  alcohol = lv_label_create(Screen1);
  lv_label_set_text(alcohol, "Alcohol");
  lv_obj_set_style_text_font(alcohol, &lv_font_montserrat_12, 0);
  lv_obj_set_width(alcohol, 150);
  lv_obj_align(alcohol, LV_ALIGN_TOP_LEFT, 10, 5);
  lv_obj_set_style_text_color(alcohol, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  //    v_obj_t *line = lv_label_create(Screen1);
  //  /*
  lines = lv_chart_create(Screen1);
  lv_obj_set_size(lines, 170, 1.5);
  lv_obj_center(lines);
  lv_chart_set_type(lines, LV_CHART_TYPE_LINE);
  lv_obj_align(lines, LV_ALIGN_TOP_LEFT, 10, 77.5);
  //  */

  lv_obj_t *benzene = lv_label_create(Screen1);
  lv_label_set_text(benzene, "Benzene");
  lv_obj_set_style_text_font(benzene, &lv_font_montserrat_12, 0);
  lv_obj_set_width(benzene, 150);
  lv_obj_align(benzene, LV_ALIGN_TOP_LEFT, 10, 80);
  lv_obj_set_style_text_color(benzene, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  //  /*
  lv_obj_t *line2 = lv_chart_create(Screen1);
  lv_obj_set_size(line2, 170, 1.5);
  lv_obj_center(line2);
  lv_chart_set_type(line2, LV_CHART_TYPE_LINE);
  lv_obj_align(line2, LV_ALIGN_TOP_LEFT, 10, 157.5);
  //  */

  lv_obj_t *hexane = lv_label_create(Screen1);
  lv_label_set_text(hexane, "hexane");
  lv_obj_set_style_text_font(hexane, &lv_font_montserrat_12, 0);
  lv_obj_set_width(hexane, 150);
  lv_obj_align(hexane, LV_ALIGN_TOP_LEFT, 10, 160);
  lv_obj_set_style_text_color(hexane, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  //  /*
  lv_obj_t *line3 = lv_chart_create(Screen1);
  lv_obj_set_size(line3, 170, 1.5);
  lv_obj_center(line3);
  lv_chart_set_type(line3, LV_CHART_TYPE_LINE);
  lv_obj_align(line3, LV_ALIGN_TOP_LEFT, 10, 237.5);
  //  */

  lv_obj_t *methane = lv_label_create(Screen1);
  lv_label_set_text(methane, "methane");
  lv_obj_set_style_text_font(methane, &lv_font_montserrat_12, 0);
  lv_obj_set_width(methane, 150);
  lv_obj_align(methane, LV_ALIGN_TOP_LEFT, 10, 240);
  lv_obj_set_style_text_color(methane, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  //  /*
  lv_obj_t *line10 = lv_chart_create(Screen1);
  lv_obj_set_size(line10, 2, 320);
  lv_obj_center(line10);
  lv_chart_set_type(line10, LV_CHART_TYPE_LINE);
  lv_obj_align(line10, LV_ALIGN_TOP_MID, -70, 0);
  //*/

  lv_obj_t *CO2 = lv_label_create(Screen1);
  lv_label_set_text(CO2, "CO2");
  lv_obj_set_style_text_font(CO2, &lv_font_montserrat_12, 0);
  lv_obj_set_width(CO2, 150);
  lv_obj_align(CO2, LV_ALIGN_TOP_MID, 10, 5);
  lv_obj_set_style_text_color(CO2, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  //  /*
  lv_obj_t *line4 = lv_chart_create(Screen1);
  lv_obj_set_size(line4, 165, 1.5);
  lv_obj_center(line4);
  lv_chart_set_type(line4, LV_CHART_TYPE_LINE);
  lv_obj_align(line4, LV_ALIGN_TOP_MID, 0, 77.5);
  //  */

  lv_obj_t *Toulone = lv_label_create(Screen1);
  lv_label_set_text(Toulone, "Toulone");
  // lv_obj_set_width(alcohol, 5);
  lv_obj_set_style_text_font(Toulone, &lv_font_montserrat_12, 0);
  lv_obj_set_width(Toulone, 150);
  lv_obj_align(Toulone, LV_ALIGN_TOP_MID, 10, 80);
  lv_obj_set_style_text_color(Toulone, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  //  /*
  lv_obj_t *line5 = lv_chart_create(Screen1);
  lv_obj_set_size(line5, 165, 1.5);
  lv_obj_center(line5);
  lv_chart_set_type(line5, LV_CHART_TYPE_LINE);
  lv_obj_align(line5, LV_ALIGN_TOP_MID, 0, 157.5);
  //  */

  lv_obj_t *acetone = lv_label_create(Screen1);
  lv_label_set_text(acetone, "Acetone");
  lv_obj_set_style_text_font(acetone, &lv_font_montserrat_12, 0);
  lv_obj_set_width(acetone, 150);
  lv_obj_align(acetone, LV_ALIGN_TOP_MID, 10, 160);
  lv_obj_set_style_text_color(acetone, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  //  /*
  lv_obj_t *line6 = lv_chart_create(Screen1);
  lv_obj_set_size(line6, 165, 1.5);
  lv_obj_center(line6);
  lv_chart_set_type(line6, LV_CHART_TYPE_LINE);
  lv_obj_align(line6, LV_ALIGN_TOP_MID, 0, 237.5);
  //  */

  lv_obj_t *H2 = lv_label_create(Screen1);
  lv_label_set_text(H2, "H2");
  lv_obj_set_style_text_font(H2, &lv_font_montserrat_12, 0);
  lv_obj_set_width(H2, 150);
  lv_obj_align(H2, LV_ALIGN_TOP_MID, 10, 240);
  lv_obj_set_style_text_color(H2, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  //  /*
  lv_obj_t *line11 = lv_chart_create(Screen1);
  lv_obj_set_size(line11, 2, 320);
  lv_obj_center(line11);
  lv_chart_set_type(line11, LV_CHART_TYPE_LINE);
  lv_obj_align(line11, LV_ALIGN_TOP_RIGHT, -155, 0);

  lv_obj_t *temp = lv_label_create(Screen1);
  lv_label_set_text(temp, "TEMP");
  lv_obj_set_style_text_font(temp, &lv_font_montserrat_12, 0);
  lv_obj_set_width(temp, 150);
  lv_obj_align(temp, LV_ALIGN_TOP_RIGHT, 0, 35);
  lv_obj_set_style_text_color(temp, lv_color_hex(0xFFFFFF), LV_PART_MAIN);

  lv_obj_t *hum = lv_label_create(Screen1);
  lv_label_set_text(hum, "HUM");
  lv_obj_set_style_text_font(hum, &lv_font_montserrat_12, 0);
  lv_obj_set_width(hum, 150);
  lv_obj_align(hum, LV_ALIGN_TOP_RIGHT, 0, 115);
  lv_obj_set_style_text_color(hum, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
}

void initlvgl() {
  lv_init();


 #if LV_USE_LOG != 0
  lv_log_register_print_cb(my_print); /* register print function for debugging */
 #endif

  tft.begin();        /* TFT init */
  tft.setRotation(3); /* Landscape orientation, flipped */

  /*Set the touchscreen calibration data,
    the actual data for your display can be acquired using
    the Generic -> Touch_calibrate example from the TFT_eSPI library*/
  uint16_t calData[5] = { 275, 3620, 264, 3532, 1 };
  tft.setTouch(calData);

  lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * screenHeight / 32);

  /*Initialize the display*/
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  /*Change the following line to your display resolution*/
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  /*Initialize the (dummy) input device driver*/
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);
}