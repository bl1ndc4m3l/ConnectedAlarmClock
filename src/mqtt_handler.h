/**
 * =============================================================================
 * MQTT Handler - Header
 * =============================================================================
 * Handles MQTT connection, subscription, and message processing
 */

#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <PubSubClient.h>
#include <WiFiClient.h>
#include <Adafruit_NeoPixel.h>

// External variables needed by MQTT handler
extern String temp;
extern bool alarmActive;
extern unsigned long lastBuzzerToggle;
extern bool buzzerState;
extern Adafruit_NeoPixel pixel;

// Note: BUZZER_PIN is defined in the main .ino file as a #define

// MQTT Topics
#define MQTT_TOPIC_TEMPERATURE   "homeassistant/bresser61/Temperature0"
#define MQTT_TOPIC_ALARM_REQUEST "alarmclock/alarmrequest"
#define MQTT_TOPIC_ALARM_CONFIRM "alarmclock/alarmconfirm"

// MQTT client
extern WiFiClient wifi;
extern PubSubClient mqtt;

// Function declarations
void setupMQTT();
void connectMQTT();
void messageReceived(char* topic, byte* payload, unsigned int length);

#endif // MQTT_HANDLER_H
