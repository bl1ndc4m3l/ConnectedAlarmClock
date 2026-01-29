# ESP32 Connected Alarm Clock

A smart alarm clock for children built with an ESP32 (TTGO board), featuring OLED display, MQTT integration with Home Assistant, and multi-sensory alarm notifications.

Migrated from existing project for ESP8266

https://github.com/lazyzero/ESP8266-Children-Alarm-Clock

## Features

### Display
- **128x64 OLED Display** (SSD1306) showing:
  - Current time in large font (HH:MM format)
  - Outside temperature from MQTT sensor
  - Bell icon when alarm is active

### Alarm System
- **Trigger**: MQTT message from Home Assistant (based on calendar events)
- **Notifications**:
  - 🔔 **Visual**: Bell icon on OLED display
  - 🔊 **Audio**: Buzzer beeping (500ms interval)
  - 💡 **Light**: NeoPixel LED with random changing colors
- **Dismiss**: Physical button press
- **Confirmation**: Publishes to MQTT when alarm is dismissed locally

### Connectivity
- **WiFi**: Connects to local network
- **MQTT**: Bi-directional communication with Home Assistant
- **NTP**: Automatic time synchronization with European NTP servers
- **mDNS**: Accessible via hostname on local network
- **DST**: Automatic daylight saving time adjustment for Central Europe
- **OTA**: Over-The-Air firmware updates via Arduino IDE or PlatformIO

## Hardware

### Components
| Component | Description |
|-----------|-------------|
| ESP32 | TTGO ESP32 development board |
| OLED Display | 128x64 SSD1306 I2C display |
| Buzzer | KPM-G09C passive buzzer (requires PWM frequency) |
| Push Button | Momentary switch to dismiss alarm |
| NeoPixel | WS2812B RGB LED for visual alarm |

### Pin Configuration
| Function | GPIO Pin |
|----------|----------|
| OLED SDA | 21 |
| OLED SCL | 22 |
| Buzzer | 4 |
| Button | 33 (INPUT_PULLUP) |
| NeoPixel | 5 |

### Wiring Diagram
```
ESP32 TTGO Board
┌─────────────────────┐
│                     │
│  GPIO 21 ──────────►│ OLED SDA
│  GPIO 22 ──────────►│ OLED SCL
│                     │
│  GPIO 4  ──────────►│ Buzzer (+)
│  GND     ──────────►│ Buzzer (-)
│                     │
│  GPIO 33 ──────────►│ Button (one side)
│  GND     ──────────►│ Button (other side)
│                     │
│  GPIO 5  ──────────►│ NeoPixel DIN
│  3.3V    ──────────►│ NeoPixel VCC
│  GND     ──────────►│ NeoPixel GND
│                     │
└─────────────────────┘
```

## Software

### Dependencies (PlatformIO)
```ini
lib_deps = 
    knolleary/PubSubClient @ ^2.8
    arduino-libraries/NTPClient @ ^3.2.1
    thingpulse/ESP8266 and ESP32 OLED driver for SSD1306 displays @ ^4.6.1
    adafruit/Adafruit NeoPixel @ ^1.12.0
```

### Configuration
Edit `src/config.h` with your settings:
```cpp
#define WIFI_SSID "YourWiFiSSID"
#define WIFI_PASS "YourWiFiPassword"

#define HOSTNAME "AlarmClock"
#define MQTT_HOST "192.168.x.x"      // Your MQTT broker IP
#define MQTT_CLIENT_ID "AlarmClock"
#define MQTT_USER "mqtt_user"
#define MQTT_PASS "mqtt_password"
```

### Building & Uploading
```bash
# Using PlatformIO CLI (USB)
pio run -t upload

# Monitor serial output
pio device monitor
```

### OTA (Over-The-Air) Updates
Once the device is running and connected to WiFi, you can upload new firmware wirelessly:

**Using PlatformIO:**
```bash
# Upload via OTA (replace IP with your device's IP)
pio run -t upload --upload-port AlarmClock.local
# Or using IP address
pio run -t upload --upload-port 192.168.x.x
```

**Using Arduino IDE:**
1. Go to Tools → Port
2. Select "AlarmClock at 192.168.x.x" (network port)
3. Upload as usual

**During OTA Update:**
- Display shows progress bar with percentage
- Serial monitor shows progress
- Device automatically reboots after successful update

## MQTT Topics

| Topic | Direction | Payload | Description |
|-------|-----------|---------|-------------|
| `alarmclock/alarmrequest` | HA → ESP32 | `on`, `1`, `true` | Activate alarm |
| `alarmclock/alarmrequest` | HA → ESP32 | `off`, `0`, `false` | Deactivate alarm |
| `alarmclock/alarmconfirm` | ESP32 → HA | `dismissed` | Alarm was dismissed locally |
| `homeassistant/bresser61/Temperature0` | HA → ESP32 | Temperature value | Updates displayed temperature |

## Home Assistant Integration

### Automation Example
Trigger alarm when a calendar event named "aufstehen" starts:

```yaml
- id: 'alarm_clock_trigger'
  alias: 'Children Alarm Clock Trigger'
  trigger:
    - platform: calendar
      event: start
      entity_id: calendar.children_alarm
  condition:
    - condition: template
      value_template: "{{ trigger.calendar_event.summary | lower == 'aufstehen' }}"
  action:
    - service: mqtt.publish
      data:
        topic: "alarmclock/alarmrequest"
        payload: "on"
        qos: 1
  mode: single
```

### Optional MQTT Entities
```yaml
mqtt:
  sensor:
    - name: "Alarm Clock Status"
      state_topic: "alarmclock/alarmconfirm"
      icon: mdi:alarm

  switch:
    - name: "Alarm Clock"
      command_topic: "alarmclock/alarmrequest"
      payload_on: "on"
      payload_off: "off"
      icon: mdi:alarm
```

For complete Home Assistant configuration, see [HomeAssistant_Configuration.md](HomeAssistant_Configuration.md).

## Operation

### Normal Mode
1. Display shows current time and temperature
2. Time updates from NTP server every 10 minutes
3. Automatic DST adjustment for Central European Time

### Alarm Mode
When an alarm is triggered via MQTT:
1. 🔔 Bell icon appears on display (top-left corner)
2. 🔊 Buzzer beeps at 2700 Hz (on/off every 500ms)
3. 💡 NeoPixel LED shows random colors, changing every second

### Dismissing Alarm
- **Local**: Press the button on GPIO 33
  - Stops buzzer and LED
  - Publishes "dismissed" to `alarmclock/alarmconfirm`
- **Remote**: Send `off` to `alarmclock/alarmrequest` via MQTT

### Auto-Off
Configure Home Assistant to automatically turn off the alarm after a timeout (e.g., 10 minutes) if not dismissed manually.

## File Structure
```
ConnectedAlarmClock/
├── platformio.ini              # PlatformIO configuration
├── ChildClockAlarm.md          # This documentation
├── HomeAssistant_Configuration.md  # HA setup guide
├── README.md                   # Project readme
├── lib/                        # Local libraries
│   ├── ESP8266_Oled_Driver_for_SSD1306_display/
│   ├── MQTT/
│   ├── NTPClient/
│   └── Time/
└── src/
    ├── ConnectedAlarmClock.ino # Main application code
    ├── _config.h               # Configuration template (rename to config.h)
    ├── config.h                # WiFi/MQTT configuration (gitignored)
    ├── mqtt_handler.h          # MQTT handler interface
    ├── mqtt_handler.cpp        # MQTT connection and message handling
    ├── ota_handler.h           # OTA update interface
    ├── ota_handler.cpp         # OTA update implementation
    ├── font.h                  # Custom display fonts
    └── images.h                # Icons (WiFi logo, bell, etc.)
```

## Code Organization

The project is structured with separation of concerns:

- **ConnectedAlarmClock.ino**: Main application logic, display updates, alarm handling, time synchronization
- **mqtt_handler.cpp/.h**: MQTT broker connection, topic subscription, message processing
- **ota_handler.cpp/.h**: Over-the-air firmware update functionality with display feedback
- **config.h**: Network and MQTT credentials (create from `_config.h` template)
```

## Troubleshooting

### MQTT Not Connecting
- Verify broker IP address in `config.h`
- Check MQTT credentials
- Ensure ESP32 is on the same network as the broker

### Alarm Not Triggering
- Check MQTT topic matches exactly (`alarmclock/alarmrequest`)
- Verify subscription in serial monitor after boot
- Test with MQTT Explorer by publishing directly to the topic

### Time Not Updating
- Check WiFi connection
- Verify NTP server is reachable (`europe.pool.ntp.org`)
- Check serial monitor for NTP update messages

### Display Issues
- Verify I2C address (default: `0x3C`)
- Check SDA/SCL wiring to GPIO 21/22

## License

This project is open source. See individual library licenses in the `lib/` folder.
