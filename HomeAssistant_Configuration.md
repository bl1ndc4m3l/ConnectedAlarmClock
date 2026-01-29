# Home Assistant Configuration for ESP32 Alarm Clock

This document describes how to configure Home Assistant to trigger the ESP32 alarm clock based on calendar events.

## MQTT Topics

| Topic | Direction | Purpose |
|-------|-----------|---------|
| `alarmclock/alarmrequest` | HA → ESP32 | Trigger alarm (`on`/`off`) |
| `alarmclock/alarmconfirm` | ESP32 → HA | Confirmation when alarm is dismissed locally |

## Option 1: Using automations.yaml

Add the following to your `automations.yaml` file:

```yaml
- id: 'alarm_clock_trigger'
  alias: 'Children Alarm Clock Trigger'
  description: 'Triggers the alarm clock when a calendar event named "wakeup" starts'
  trigger:
    - platform: calendar
      event: start
      entity_id: calendar.children_alarm  # Change to your calendar entity
  condition:
    - condition: template
      value_template: "{{ trigger.calendar_event.summary | lower == 'wakeup' }}"
  action:
    - service: mqtt.publish
      data:
        topic: "alarmclock/alarmrequest"
        payload: "on"
        qos: 1
        retain: false
  mode: single

- id: 'alarm_clock_auto_off'
  alias: 'Children Alarm Clock Auto Off'
  description: 'Automatically turn off alarm after 10 minutes if not dismissed'
  trigger:
    - platform: calendar
      event: start
      entity_id: calendar.children_alarm
  condition:
    - condition: template
      value_template: "{{ trigger.calendar_event.summary | lower == 'wakeup' }}"
  action:
    - delay:
        minutes: 10
    - service: mqtt.publish
      data:
        topic: "alarmclock/alarmrequest"
        payload: "off"
        qos: 1
  mode: restart

- id: 'alarm_clock_dismissed_notification'
  alias: 'Alarm Clock Dismissed Notification'
  description: 'Notify when child dismisses the alarm'
  trigger:
    - platform: mqtt
      topic: "alarmclock/alarmconfirm"
      payload: "dismissed"
  action:
    - service: notify.notify
      data:
        title: "Alarm Clock"
        message: "The alarm was dismissed at {{ now().strftime('%H:%M') }}"
  mode: single
```

### Alternative Condition: Partial Match

If you want to match events that *contain* "wakeup" :

```yaml
condition:
  - condition: template
    value_template: "{{ 'wakeup' in trigger.calendar_event.summary | lower }}"
```

### Alternative Condition: Weekday Only

To trigger only on weekdays:

```yaml
condition:
  - condition: template
    value_template: "{{ trigger.calendar_event.summary | lower == 'wakeup' }}"
  - condition: time
    weekday:
      - mon
      - tue
      - wed
      - thu
      - fri
```

## Option 2: Using the Home Assistant UI

### Step 1: Create a Calendar

1. Go to **Settings** → **Devices & Services** → **Calendars**
2. Add a new calendar (e.g., "Children Alarm")
3. Create events named **"aufstehen"** at your desired wake-up times

### Step 2: Create the Automation via UI

1. Go to **Settings** → **Automations & Scenes** → **Create Automation**
2. Click **Create new automation**

#### Trigger Configuration:
- **Trigger type**: Calendar
- **Calendar entity**: Select your calendar (e.g., `calendar.children_alarm`)
- **Event**: Start

#### Condition Configuration:
- **Condition type**: Template
- **Value template**: `{{ trigger.calendar_event.summary | lower == 'wakeup' }}`

#### Action Configuration:
- **Action type**: Call service
- **Service**: `mqtt.publish`
- **Service data**:
  ```yaml
  topic: alarmclock/alarmrequest
  payload: "on"
  qos: 1
  ```

3. Save the automation

## MQTT Entities (Optional)

Add to `configuration.yaml` to track alarm state in Home Assistant:

```yaml
mqtt:
  sensor:
    - name: "Alarm Clock Status"
      state_topic: "alarmclock/alarmconfirm"
      icon: mdi:alarm

  switch:
    - name: "Alarm Clock"
      state_topic: "alarmclock/alarmrequest"
      command_topic: "alarmclock/alarmrequest"
      payload_on: "on"
      payload_off: "off"
      state_on: "on"
      state_off: "off"
      icon: mdi:alarm
```

## Dashboard Card (Optional)

Add to your Lovelace dashboard:

```yaml
type: entities
title: Children Alarm Clock
entities:
  - entity: switch.alarm_clock
    name: Trigger Alarm
  - entity: sensor.alarm_clock_status
    name: Last Status
  - entity: calendar.children_alarm
    name: Alarm Schedule
```

## Testing

### Via Developer Tools

1. Go to **Developer Tools** → **Services**
2. Select service: `mqtt.publish`
3. Enter service data:
   ```yaml
   topic: alarmclock/alarmrequest
   payload: "on"
   ```
4. Click **Call Service**

The ESP32 buzzer should start beeping immediately.

### Via MQTT Explorer

Publish to topic `alarmclock/alarmrequest` with payload `on` or `off`.

## Troubleshooting

- **Alarm doesn't trigger**: Check that the calendar event name matches exactly (case-insensitive)
- **MQTT not working**: Verify MQTT broker connection in Home Assistant integrations
- **ESP32 not receiving**: Check serial monitor for MQTT connection status and subscription confirmations
