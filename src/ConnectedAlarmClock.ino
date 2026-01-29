#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <TimeLib.h>
#include <Time.h>
#include <Wire.h>
#include "SSD1306.h"
#include "OLEDDisplayUi.h"
#include <Adafruit_NeoPixel.h>
#include "font.h"
#include "images.h"
#include "config.h"
#include "mqtt_handler.h"
#include "ota_handler.h"

/**
 * @defgroup pin_definitions Pin Definitions
 * @brief GPIO pin assignments for ESP32
 * @{
 */
#define OLED_SDA        21  ///< I2C SDA pin for OLED display
#define OLED_SCL        22  ///< I2C SCL pin for OLED display
#define BUZZER_PIN      4   ///< PWM pin for passive buzzer
#define BUTTON_PIN      33  ///< Input pin for alarm dismiss button
#define NEOPIXEL_PIN    5   ///< Data pin for NeoPixel LED
#define NEOPIXEL_COUNT  1   ///< Number of NeoPixel LEDs
/** @} */

/**
 * @defgroup timing_constants Timing Constants
 * @brief Interval definitions for various periodic tasks
 * @{
 */
const unsigned long NTP_UPDATE_INTERVAL     = 600000;  ///< NTP sync interval (10 minutes)
const unsigned long DISPLAY_UPDATE_INTERVAL = 1000;    ///< Display refresh interval (1 second)
const unsigned long MQTT_RECONNECT_INTERVAL = 5000;    ///< MQTT reconnect delay (5 seconds)
const unsigned long BUZZER_INTERVAL         = 300;     ///< Alarm beep interval in ms
const unsigned long COLOR_CHANGE_INTERVAL   = 1000;    ///< LED color change interval
const unsigned long DEBOUNCE_DELAY          = 50;      ///< Button debounce delay in ms
/** @} */

/**
 * @defgroup config_constants Configuration Constants
 * @brief Hardware and network configuration values
 * @{
 */
const int          TIMEZONE_OFFSET    = 1;             ///< Central European Time (CET) offset
const int          DEFAULT_UTC_OFFSET = 3600;          ///< Default UTC offset in seconds (1 hour)
const unsigned int BUZZER_FREQUENCY   = 2500;          ///< Buzzer frequency in Hz (KPM-G09C)
const char*        NTP_SERVER         = "europe.pool.ntp.org";  ///< NTP server address
const char*        HOST_NAME          = HOSTNAME;      ///< mDNS hostname
/** @} */

/**
 * @defgroup hardware_objects Hardware Objects
 * @brief Display and LED hardware instances
 * @{
 */
SSD1306            display(0x3C, OLED_SDA, OLED_SCL);  ///< OLED display (I2C address 0x3C)
Adafruit_NeoPixel  pixel(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);  ///< NeoPixel LED
/** @} */

/**
 * @defgroup network_objects Network Objects
 * @brief Network communication instances
 * @{
 */
WiFiUDP            ntpUDP;             ///< UDP client for NTP
WiFiClient         wifi;               ///< WiFi client for MQTT
PubSubClient       mqtt(wifi);         ///< MQTT client
WebServer          httpServer(80);     ///< HTTP server on port 80
NTPClient          timeClient(ntpUDP, NTP_SERVER, DEFAULT_UTC_OFFSET, NTP_UPDATE_INTERVAL);  ///< NTP client
/** @} */

/**
 * @defgroup timing_state Timing State Variables
 * @brief Timestamps for tracking periodic task execution
 * @{
 */
unsigned long lastUpdate               = -NTP_UPDATE_INTERVAL;  ///< Last NTP update timestamp
unsigned long lastDisplayUpdate        = 0;   ///< Last display refresh timestamp
unsigned long lastMqttReconnectAttempt = 0;   ///< Last MQTT reconnect attempt timestamp
unsigned long lastBuzzerToggle         = 0;   ///< Last buzzer state change timestamp
unsigned long lastButtonCheck          = 0;   ///< Last button check timestamp
unsigned long lastColorChange          = 0;   ///< Last LED color change timestamp
/** @} */

/**
 * @defgroup alarm_state Alarm State Variables
 * @brief Variables tracking alarm status and button state
 * @{
 */
bool alarmActive     = false;   ///< True when alarm is sounding
bool buzzerState     = false;   ///< Current buzzer on/off state
int  lastButtonState = HIGH;    ///< Previous button reading for edge detection
/** @} */

/**
 * @defgroup display_data Display Data
 * @brief Strings and values shown on the OLED display
 * @{
 */
String timeS   = "";    ///< Formatted time string (HH:MM)
String temp    = "99";  ///< Temperature string from MQTT
String weather = "";    ///< Weather description (unused)
/** @} */

/**
 * @defgroup other_state Other State Variables
 * @brief Miscellaneous state variables
 * @{
 */
int defaultOffset = DEFAULT_UTC_OFFSET;  ///< Mutable UTC offset for DST adjustment
time_t currentTime;                       ///< Current Unix timestamp (unused)
/** @} */


/**
 * @defgroup prototypes Function Prototypes
 * @brief Forward declarations
 * @{
 */
int offsetDstEurope();
void connect();
/** @} */

/**
 * @defgroup main_functions Main Functions
 * @brief Arduino setup and loop entry points
 * @{
 */

/**
 * @brief Initialize hardware, network connections, and services
 * 
 * Configures GPIO pins, display, NeoPixel LED, WiFi, mDNS, OTA updates,
 * MQTT client, and NTP time synchronization.
 */
void setup() {
  Serial.begin(115200);
  Serial.println("ConnectedAlarmClock");

  // Initialize buzzer and button
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Initialize NeoPixel
  pixel.begin();
  pixel.clear();
  pixel.show();
  randomSeed(analogRead(0));  // Seed random number generator

  // put your setup code here, to run once:
  display.init();
  display.setContrast(30);
  display.clear();
  display.flipScreenVertically();
  
  // Show WiFi icon (left side) - connecting to WiFi
  display.drawXbm(16, 20, WiFi_Small_width, WiFi_Small_height, WiFi_Small_bits);
  display.display();

  connect();
  
  // Show MQTT icon (right side) - connecting to MQTT
  display.drawXbm(80, 20, MQTT_Icon_width, MQTT_Icon_height, MQTT_Icon_bits);
  display.display();

  MDNS.begin(HOST_NAME);
  httpServer.begin();
  MDNS.addService("http", "tcp", 80);

  // Setup OTA and MQTT
  setupOTA();
  setupMQTT();

  Serial.println("start NTP");
  timeClient.begin();

  // Force initial NTP update with retries
  Serial.print("Getting initial time from NTP...");
  int ntpRetries = 0;
  while (!timeClient.forceUpdate() && ntpRetries < 5) {
    Serial.print(".");
    delay(1000);
    ntpRetries++;
  }
  if (ntpRetries < 5) {
    Serial.println(" success!");
    timeClient.setTimeOffset(offsetDstEurope());
    lastUpdate = millis();  // Reset update timer after successful sync
  } else {
    Serial.println(" failed! Will retry later.");
  }

  delay(1500);
}



/**
 * @brief Main application loop
 * 
 * Handles OTA updates, MQTT communication, button input for alarm dismissal,
 * buzzer/LED alarm feedback, web server requests, NTP time updates, and
 * display refresh.
 */
void loop() {
  // Handle OTA updates
  ArduinoOTA.handle();

  // MQTT loop should be called first to maintain connection
  mqtt.loop();

  // Only attempt reconnection if disconnected AND enough time has passed
  if (!mqtt.connected()) {
    if (millis() - lastMqttReconnectAttempt >= MQTT_RECONNECT_INTERVAL) {
      lastMqttReconnectAttempt = millis();
      connectMQTT();
    }
  }

  // Check button to dismiss alarm (with debounce)
  if (millis() - lastButtonCheck >= DEBOUNCE_DELAY) {
    int buttonState = digitalRead(BUTTON_PIN);
    if (buttonState == LOW && lastButtonState == HIGH && alarmActive) {
      // Button pressed - dismiss alarm
      alarmActive = false;
      buzzerState = false;
      noTone(BUZZER_PIN);
      pixel.clear();
      pixel.show();
      Serial.println("Alarm dismissed locally");
      // Publish confirmation
      mqtt.publish(MQTT_TOPIC_ALARM_CONFIRM, "dismissed");
    }
    lastButtonState = buttonState;
    lastButtonCheck = millis();
  }

  // Handle buzzer beeping when alarm is active
  if (alarmActive) {
    if (millis() - lastBuzzerToggle >= BUZZER_INTERVAL) {
      buzzerState = !buzzerState;
      if (buzzerState) {
        tone(BUZZER_PIN, BUZZER_FREQUENCY);
      } else {
        noTone(BUZZER_PIN);
      }
      lastBuzzerToggle = millis();
    }
    // Change NeoPixel to random color periodically
    if (millis() - lastColorChange >= COLOR_CHANGE_INTERVAL) {
      uint8_t r = random(50, 256);
      uint8_t g = random(50, 256);
      uint8_t b = random(50, 256);
      pixel.setPixelColor(0, pixel.Color(r, g, b));
      pixel.show();
      lastColorChange = millis();
    }
  }

  // Handle web server
  httpServer.handleClient();

  // Update NTP time periodically
  if (millis() - lastUpdate >= NTP_UPDATE_INTERVAL) {
    if (WiFi.status() != WL_CONNECTED) connect();

    Serial.print("Update time....");
    bool updateSuccess = timeClient.forceUpdate();  // Use forceUpdate to ensure we actually sync
    if (!updateSuccess) {
      Serial.println(" NTP update failed, will retry.");
      lastUpdate = millis() - NTP_UPDATE_INTERVAL + 60000;  // Retry in 1 minute
      return;
    }
    if (timeClient.getHours() == 2 && timeClient.getMinutes() == 0) timeClient.forceUpdate();

    timeClient.setTimeOffset(offsetDstEurope());

    Serial.print(offsetDstEurope());
    Serial.print(" ");

    Serial.print(timeClient.getHours());
    Serial.print(":");
    Serial.println(timeClient.getMinutes());
    lastUpdate = millis();
  }

  // Update display every second (non-blocking)
  if (millis() - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
    timeS = timeClient.getHours();
    timeS += ":";
    if (timeClient.getMinutes() < 10) timeS += "0";
    timeS += timeClient.getMinutes();
    display.clear();
    //display.drawXbm( 35, 0,50, 50, sleet_bits);
    display.setFont(Monospaced_bold_22);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawString(128 - display.getStringWidth(temp + "°C"), 2, temp + "°C");
    display.setFont(Monospaced_bold_42);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 24, timeS);
    
    // Show bell icon when alarm is active
    if (alarmActive) {
      display.drawXbm(2, 2, bell_icon_width, bell_icon_height, bell_icon_bits);
    }
    
    display.display();
    lastDisplayUpdate = millis();
  }
}

/**
 * @brief Establish WiFi connection
 * 
 * Connects to the configured WiFi network (WIFI_SSID/WIFI_PASS from config.h).
 * Blocks until connection is established.
 */
void connect() {
  Serial.print("checking wifi...");
  Serial.print("SSID:");
  Serial.print(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nconnected to wifi!");
}

/**
 * @brief Calculate UTC offset with European DST rules
 * 
 * Determines whether Daylight Saving Time is active based on European rules
 * (last Sunday of March to last Sunday of October).
 * 
 * @return int UTC offset in seconds (3600 for CET, 7200 for CEST)
 */
int offsetDstEurope() {
  // getEpochTime() already returns Unix epoch (seconds since 1970)
  // No need to subtract SEVENZYYEARS again!
  unsigned long epochTime = timeClient.getEpochTime();
  
  // Only calculate DST if we have a valid time (epoch > 0 means we got NTP response)
  if (epochTime < 86400) {
    // Time not yet synchronized, return default offset
    return defaultOffset;
  }
  
  setTime(epochTime);  // Set TimeLib time from epoch

  int beginDSTDate = (31 - (5 * year() / 4 + 4) % 7);  // last sunday of march
  int beginDSTMonth = 3;
  int endDSTDate = (31 - (5 * year() / 4 + 1) % 7);    // last sunday of october
  int endDSTMonth = 10;

  if (((month() > beginDSTMonth) && (month() < endDSTMonth))
    || ((month() == beginDSTMonth) && (day() >= beginDSTDate))
    || ((month() == endDSTMonth) && (day() <= endDSTDate)))
    return 7200;  // DST europe = utc +2 hour
  else 
    return 3600;  // nonDST europe = utc +1 hour
}
/** @} */ // End of main_functions group