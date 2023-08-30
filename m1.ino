 



void clearRam() {
  //       // Allocate memory to clear RAM
  //     size_t ramSize = ESP.getFreeHeap();
  //     byte *tempMemory = (byte *)malloc(ramSize);
  //     if (tempMemory) {
  //         free(tempMemory); // Free the allocated memory
  // }
}


  // void connectToWiFiTask(void *param) {
//   char lastSSID[32];
//   char lastPassword[64];

//     char storedSSID[32]; // Declare storedSSID here
//   char storedPassword[64]; // Declare storedPassword here

//   loadCredentialsFromEEPROM(storedSSID, storedPassword);

//   Serial.println("Trying to connect to last saved WiFi...");

//   if (WiFi.begin(storedSSID, storedPassword) == WL_CONNECTED) {
//     Serial.println("WiFi connected");
//     // Update your LVGL labels or any other UI elements here
//     lv_obj_set_style_text_color(settinglabel, lv_color_hex(0x00FF00), LV_PART_MAIN);
//     lv_label_set_text(settinglabel2, " WIFI CONNECTED ");
//     lv_obj_set_style_text_color(settinglabel2, lv_color_hex(0x00FF00), LV_PART_MAIN);
//     wifiConnected = true;
//   } else {
//     Serial.println("WiFi connection failed");
//     // Handle connection failure if needed
//   }

//   vTaskDelete(NULL); // Delete the task
// }

// void connectToLastWiFi() {
//   xTaskCreate(
//     connectToWiFiTask, // Function to run
//     "WiFiTask",        // Name of the task
//     8192,             // Stack size
//     NULL,              // Task parameters
//     0,                 // Priority
//     NULL               // Task handle
//   );
// }


