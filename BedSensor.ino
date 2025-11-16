#ifndef ZIGBEE_MODE_ZCZR
#error "Zigbee coordinator/router device mode is not selected in Tools->Zigbee mode"
#endif

#include "Zigbee.h"

/* Zigbee OTA configuration */
#define OTA_UPGRADE_RUNNING_FILE_VERSION    0x01010100  // Increment this value when the running image is updated
#define OTA_UPGRADE_DOWNLOADED_FILE_VERSION 0x01010101  // Increment this value when the downloaded image is updated
#define OTA_UPGRADE_HW_VERSION              0x0001      // The hardware version, this can be used to differentiate between different hardware versions

uint8_t analogPin1 = A2;
uint8_t analogPin2 = A3;

uint8_t button = BOOT_PIN;

uint8_t analogPrecision = 10;

ZigbeeAnalog zbAnalogDeviceBed1 = ZigbeeAnalog(10);
ZigbeeAnalog zbAnalogDeviceBed2 = ZigbeeAnalog(11);

void setup() {
  Serial.begin(115200);
  Serial.println("Starting...");

  // Init button switch
  pinMode(button, INPUT_PULLUP);

  // Set analog resolution
  analogReadResolution(analogPrecision);

  // Optional: set Zigbee device name and model
  zbAnalogDeviceBed1.setManufacturerAndModel("Espressif", "Custom-made bed occupancy sensor");

  // Add OTA client
  zbAnalogDeviceBed1.addOTAClient(OTA_UPGRADE_RUNNING_FILE_VERSION, OTA_UPGRADE_DOWNLOADED_FILE_VERSION, OTA_UPGRADE_HW_VERSION);

  // Set up analog input
  zbAnalogDeviceBed1.addAnalogInput();
  zbAnalogDeviceBed1.setAnalogInputApplication(ESP_ZB_ZCL_AI_PERCENTAGE_OTHER);
  zbAnalogDeviceBed1.setAnalogInputDescription("Percentage");
  zbAnalogDeviceBed1.setAnalogInputResolution(0.01);

  // Set up analog input
  zbAnalogDeviceBed2.addAnalogInput();
  zbAnalogDeviceBed2.setAnalogInputApplication(ESP_ZB_ZCL_AI_PERCENTAGE_OTHER);
  zbAnalogDeviceBed2.setAnalogInputDescription("Percentage");
  zbAnalogDeviceBed2.setAnalogInputResolution(0.01);

  // Add endpoints to Zigbee Core
  Zigbee.addEndpoint(&zbAnalogDeviceBed1);
  Zigbee.addEndpoint(&zbAnalogDeviceBed2);

  Serial.println("Starting Zigbee...");
  // When all EPs are registered, start Zigbee in Router Device mode
  if (!Zigbee.begin(ZIGBEE_ROUTER)) {
    Serial.println("Zigbee failed to start!");
    Serial.println("Rebooting...");
    ESP.restart();
  } else {
    Serial.println("Zigbee started successfully!");
  }
  Serial.println("Connecting to network");
  while (!Zigbee.connected()) {
    Serial.print(".");
    delay(100);
  }
  Serial.println("Connected");

  // Optional: Add reporting for analog input
  zbAnalogDeviceBed1.setAnalogInputReporting(0, 30, 5);  // report every 30 seconds if value changes by 5
  zbAnalogDeviceBed2.setAnalogInputReporting(0, 30, 5);  // report every 30 seconds if value changes by 5

  // Start Zigbee OTA client query, first request is within a minute and the next requests are sent every hour automatically
  zbAnalogDeviceBed1.requestOTAUpdate();
}

float as_percent(uint8_t pin) {
  float analogValue = (float) analogRead(pin);
  float analogPercent = (analogValue * 100.0) / pow(2.0, analogPrecision);
  return analogPercent;
}

void loop() {
  static uint32_t timeCounter = 0;

  // Read ADC value and update the analog value every 2s
  if (!(timeCounter++ % 20)) {  // delaying for 100ms x 20 = 2s
    float analog1Percent = as_percent(analogPin1);
    float analog2Percent = as_percent(analogPin2);

    Serial.printf("Updating analog input to %.1f, %.1f\r\n", analog1Percent, analog2Percent);

    zbAnalogDeviceBed1.setAnalogInput(analog1Percent);
    zbAnalogDeviceBed2.setAnalogInput(analog2Percent);
  }

  // Checking button for factory reset and reporting
  if (digitalRead(button) == LOW) {  // Push button pressed
    // Key debounce handling
    delay(100);
    int startTime = millis();
    while (digitalRead(button) == LOW) {
      delay(50);
      if ((millis() - startTime) > 3000) {
        // If key pressed for more than 3secs, factory reset Zigbee and reboot
        Serial.println("Resetting Zigbee to factory and rebooting in 1s.");
        delay(1000);
        Zigbee.factoryReset();
      }
    }
  }
  delay(100);
}