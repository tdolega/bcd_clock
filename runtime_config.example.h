#pragma once

// Copy this file to runtime_config_local.h and fill your private values.

#define CFG_WIFI_SSID "YOUR_WIFI_SSID"
#define CFG_WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define CFG_HOSTNAME "bcdClock"

#define CFG_MQTT_HOST_A 192
#define CFG_MQTT_HOST_B 168
#define CFG_MQTT_HOST_C 0
#define CFG_MQTT_HOST_D 50
#define CFG_MQTT_PORT 1883
#define CFG_MQTT_USER "YOUR_MQTT_USER"
#define CFG_MQTT_PASSWORD "YOUR_MQTT_PASSWORD"

#define CFG_TIMEZONE_ENV "CET-1CEST,M3.5.0,M10.5.0/3"
#define CFG_NTP_SERVER "pool.ntp.org"

#define CFG_MQTT_TOPIC_STATE_ONLINE "bcdClock/state/online"
#define CFG_MQTT_TOPIC_STATE_MODE "bcdClock/state/mode"
#define CFG_MQTT_TOPIC_STATE_BRIGHTNESS "bcdClock/state/brightness"
#define CFG_MQTT_TOPIC_STATE_TEMPERATURE "bcdClock/state/temperature"
#define CFG_MQTT_TOPIC_STATE_JSON "bcdClock/state/json"

#define CFG_MQTT_TOPIC_CMD_MODE "bcdClock/cmd/mode"
#define CFG_MQTT_TOPIC_CMD_BRIGHTNESS "bcdClock/cmd/brightness"
#define CFG_MQTT_TOPIC_CMD_STATE "bcdClock/cmd/state"

#define CFG_WIFI_RETRY_INTERVAL_MS 10000
#define CFG_MQTT_RETRY_INTERVAL_MS 5000
#define CFG_MQTT_RESTORE_WINDOW_MS 1500
#define CFG_MQTT_TEMPERATURE_PUBLISH_INTERVAL_MS 70000
#define CFG_THERMOMETER_READING_INTERVAL_MS 5000
#define CFG_THERMOMETER_CONVERSION_WAIT_MS 800
#define CFG_NTP_VALID_EPOCH_THRESHOLD 1700000000
#define CFG_MQTT_RX_TOPIC_BUFFER_SIZE 96
#define CFG_MQTT_RX_PAYLOAD_BUFFER_SIZE 256
