/**
 * =============================================================================
 * OTA Handler - Implementation
 * =============================================================================
 * Handles Over-The-Air firmware updates with display feedback
 */

#include "ota_handler.h"
#include "config.h"
#include <Arduino.h>

/**
 * Initialize OTA updates with callbacks
 */
void setupOTA() {
  ArduinoOTA.setHostname(HOSTNAME);
  
  ArduinoOTA.onStart([]() {
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    Serial.println("OTA Start updating " + type);
    display.clear();
    display.setFont(ArialMT_Plain_16);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 20, "OTA Update...");
    display.display();
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA End");
    display.clear();
    display.drawString(64, 20, "Update Done!");
    display.drawString(64, 40, "Rebooting...");
    display.display();
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    int percent = (progress / (total / 100));
    Serial.printf("OTA Progress: %u%%\r", percent);
    display.clear();
    display.setFont(ArialMT_Plain_16);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 10, "OTA Update");
    display.drawProgressBar(10, 32, 108, 16, percent);
    display.drawString(64, 52, String(percent) + "%");
    display.display();
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]: ", error);
    String errorMsg;
    if      (error == OTA_AUTH_ERROR)    errorMsg = "Auth Failed";
    else if (error == OTA_BEGIN_ERROR)   errorMsg = "Begin Failed";
    else if (error == OTA_CONNECT_ERROR) errorMsg = "Connect Failed";
    else if (error == OTA_RECEIVE_ERROR) errorMsg = "Receive Failed";
    else if (error == OTA_END_ERROR)     errorMsg = "End Failed";
    Serial.println(errorMsg);
    display.clear();
    display.drawString(64, 20, "OTA Error:");
    display.drawString(64, 40, errorMsg);
    display.display();
  });
  
  ArduinoOTA.begin();
  Serial.println("OTA Ready");
}
