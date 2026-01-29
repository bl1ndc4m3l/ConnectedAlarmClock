/**
 * =============================================================================
 * ConnectedAlarmClock - Configuration Template
 * =============================================================================
 * 
 * IMPORTANT: This is a template file!
 * 
 * To use this project:
 * 1. Copy or rename this file from "_config.h" to "config.h"
 * 2. Fill in your actual values below
 * 3. The file "config.h" is ignored by git to keep your credentials private
 * 
 * =============================================================================
 * CONFIGURATION OPTIONS:
 * =============================================================================
 * 
 * WIFI_SSID      - Your WiFi network name (SSID)
 * WIFI_PASS      - Your WiFi password
 * 
 * HOSTNAME       - mDNS hostname for the device (used for OTA updates)
 *                  Device will be accessible as <HOSTNAME>.local
 * 
 * MQTT_HOST      - IP address or hostname of your MQTT broker
 * MQTT_CLIENT_ID - Unique identifier for this MQTT client
 * MQTT_USER      - MQTT broker username (leave empty "" if no auth required)
 * MQTT_PASS      - MQTT broker password (leave empty "" if no auth required)
 * 
 * =============================================================================
 */

#define WIFI_SSID "YourSSID"
#define WIFI_PASS "YourPassword"

#define HOSTNAME "AlarmClock"
#define MQTT_HOST "192.xxx.yyy.zzz"
#define MQTT_CLIENT_ID "AlarmClock"
#define MQTT_USER "YourMQTTUser"
#define MQTT_PASS "YourMQTTPassword"