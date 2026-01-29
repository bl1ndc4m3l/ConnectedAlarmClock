/**
 * =============================================================================
 * MQTT Handler - Implementation
 * =============================================================================
 * Handles MQTT connection, subscription, and message processing
 */

#include "mqtt_handler.h"
#include "config.h"
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// Pin definitions (must match main file)
#define BUZZER_PIN 4

/**
 * Initialize MQTT client
 */
void setupMQTT() {
  mqtt.setServer(MQTT_HOST, 1883);
  mqtt.setCallback(messageReceived);
  Serial.println("Start MQTT");
  connectMQTT();
}

/**
 * Connect to MQTT broker and subscribe to topics
 * Non-blocking: attempts connection once and returns
 */
void connectMQTT() {
  Serial.print("checking mqtt...");
  
  // Single connection attempt (non-blocking)
  if (mqtt.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS)) {
    // Subscribe to topics
    boolean success1 = mqtt.subscribe(MQTT_TOPIC_TEMPERATURE);
    boolean success2 = mqtt.subscribe(MQTT_TOPIC_ALARM_REQUEST);
    Serial.print("\nconnected to mqtt! Subscriptions: ");
    Serial.print(success1 ? "Temp:OK " : "Temp:FAIL ");
    Serial.println(success2 ? "Alarm:OK" : "Alarm:FAIL");
  } else {
    Serial.print("\nMQTT connection failed, rc=");
    Serial.println(mqtt.state());
  }
}

/**
 * Handle incoming MQTT messages
 */
void messageReceived(char* topic, byte* payload, unsigned int length) {
  String payloadStr = "";
  for (unsigned int i = 0; i < length; i++) {
    payloadStr += (char)payload[i];
  }
  Serial.println(payloadStr);
  
  String topicStr = String(topic);
  
  // Handle temperature updates
  if (topicStr == MQTT_TOPIC_TEMPERATURE) {
    temp = payloadStr;
    Serial.print("Temperature updated to: ");
    Serial.println(temp);
  } 
  // Handle alarm requests
  else if (topicStr == MQTT_TOPIC_ALARM_REQUEST) {
    if (payloadStr == "on" || payloadStr == "1" || payloadStr == "true") {
      alarmActive = true;
      lastBuzzerToggle = millis();
      Serial.println("Alarm activated!");
    } else if (payloadStr == "off" || payloadStr == "0" || payloadStr == "false") {
      alarmActive = false;
      buzzerState = false;
      noTone(BUZZER_PIN);
      pixel.clear();
      pixel.show();
      Serial.println("Alarm deactivated via MQTT");
    }
  }
}
