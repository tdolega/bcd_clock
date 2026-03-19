#pragma once

#include <IPAddress.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

// Put private credentials and per-device overrides in runtime_config_local.h.
#if __has_include("runtime_config_local.h")
#include "runtime_config_local.h"
#endif

// Wi-Fi
#ifndef CFG_WIFI_SSID
#define CFG_WIFI_SSID "CHANGE_ME_WIFI_SSID"
#endif

#ifndef CFG_WIFI_PASSWORD
#define CFG_WIFI_PASSWORD "CHANGE_ME_WIFI_PASSWORD"
#endif

#ifndef CFG_HOSTNAME
#define CFG_HOSTNAME "bcdClock"
#endif

// MQTT broker
#ifndef CFG_MQTT_HOST_A
#define CFG_MQTT_HOST_A 192
#endif

#ifndef CFG_MQTT_HOST_B
#define CFG_MQTT_HOST_B 168
#endif

#ifndef CFG_MQTT_HOST_C
#define CFG_MQTT_HOST_C 0
#endif

#ifndef CFG_MQTT_HOST_D
#define CFG_MQTT_HOST_D 50
#endif

#ifndef CFG_MQTT_PORT
#define CFG_MQTT_PORT 1883
#endif

#ifndef CFG_MQTT_USER
#define CFG_MQTT_USER "CHANGE_ME_MQTT_USER"
#endif

#ifndef CFG_MQTT_PASSWORD
#define CFG_MQTT_PASSWORD "CHANGE_ME_MQTT_PASSWORD"
#endif

// Time
#ifndef CFG_TIMEZONE_ENV
#define CFG_TIMEZONE_ENV "CET-1CEST,M3.5.0,M10.5.0/3"
#endif

#ifndef CFG_NTP_SERVER
#define CFG_NTP_SERVER "pool.ntp.org"
#endif

// MQTT topics
#ifndef CFG_MQTT_TOPIC_STATE_ONLINE
#define CFG_MQTT_TOPIC_STATE_ONLINE "bcdClock/state/online"
#endif

#ifndef CFG_MQTT_TOPIC_STATE_MODE
#define CFG_MQTT_TOPIC_STATE_MODE "bcdClock/state/mode"
#endif

#ifndef CFG_MQTT_TOPIC_STATE_BRIGHTNESS
#define CFG_MQTT_TOPIC_STATE_BRIGHTNESS "bcdClock/state/brightness"
#endif

#ifndef CFG_MQTT_TOPIC_STATE_TEMPERATURE
#define CFG_MQTT_TOPIC_STATE_TEMPERATURE "bcdClock/state/temperature"
#endif

#ifndef CFG_MQTT_TOPIC_STATE_JSON
#define CFG_MQTT_TOPIC_STATE_JSON "bcdClock/state/json"
#endif

#ifndef CFG_MQTT_TOPIC_CMD_MODE
#define CFG_MQTT_TOPIC_CMD_MODE "bcdClock/cmd/mode"
#endif

#ifndef CFG_MQTT_TOPIC_CMD_BRIGHTNESS
#define CFG_MQTT_TOPIC_CMD_BRIGHTNESS "bcdClock/cmd/brightness"
#endif

#ifndef CFG_MQTT_TOPIC_CMD_STATE
#define CFG_MQTT_TOPIC_CMD_STATE "bcdClock/cmd/state"
#endif

// Runtime tuning
#ifndef CFG_WIFI_RETRY_INTERVAL_MS
#define CFG_WIFI_RETRY_INTERVAL_MS 10000
#endif

#ifndef CFG_MQTT_RETRY_INTERVAL_MS
#define CFG_MQTT_RETRY_INTERVAL_MS 5000
#endif

#ifndef CFG_MQTT_RESTORE_WINDOW_MS
#define CFG_MQTT_RESTORE_WINDOW_MS 1500
#endif

#ifndef CFG_MQTT_TEMPERATURE_PUBLISH_INTERVAL_MS
#define CFG_MQTT_TEMPERATURE_PUBLISH_INTERVAL_MS 70000
#endif

#ifndef CFG_THERMOMETER_READING_INTERVAL_MS
#define CFG_THERMOMETER_READING_INTERVAL_MS 5000
#endif

#ifndef CFG_THERMOMETER_CONVERSION_WAIT_MS
#define CFG_THERMOMETER_CONVERSION_WAIT_MS 800
#endif

#ifndef CFG_NTP_VALID_EPOCH_THRESHOLD
#define CFG_NTP_VALID_EPOCH_THRESHOLD 1700000000
#endif

#ifndef CFG_MQTT_RX_TOPIC_BUFFER_SIZE
#define CFG_MQTT_RX_TOPIC_BUFFER_SIZE 96
#endif

#ifndef CFG_MQTT_RX_PAYLOAD_BUFFER_SIZE
#define CFG_MQTT_RX_PAYLOAD_BUFFER_SIZE 256
#endif

// Constants used by the firmware
static const char* const WIFI_SSID = CFG_WIFI_SSID;
static const char* const WIFI_PASSWORD = CFG_WIFI_PASSWORD;
static const char* const HOSTNAME = CFG_HOSTNAME;

static const IPAddress MQTT_HOST(CFG_MQTT_HOST_A, CFG_MQTT_HOST_B, CFG_MQTT_HOST_C, CFG_MQTT_HOST_D);
static const int MQTT_PORT = CFG_MQTT_PORT;
static const char* const MQTT_USER = CFG_MQTT_USER;
static const char* const MQTT_PASSWORD = CFG_MQTT_PASSWORD;

static const char* const TIMEZONE_ENV = CFG_TIMEZONE_ENV;
static const char* const NTP_SERVER = CFG_NTP_SERVER;

static const char* const MQTT_TOPIC_STATE_ONLINE = CFG_MQTT_TOPIC_STATE_ONLINE;
static const char* const MQTT_TOPIC_STATE_MODE = CFG_MQTT_TOPIC_STATE_MODE;
static const char* const MQTT_TOPIC_STATE_BRIGHTNESS = CFG_MQTT_TOPIC_STATE_BRIGHTNESS;
static const char* const MQTT_TOPIC_STATE_TEMPERATURE = CFG_MQTT_TOPIC_STATE_TEMPERATURE;
static const char* const MQTT_TOPIC_STATE_JSON = CFG_MQTT_TOPIC_STATE_JSON;

static const char* const MQTT_TOPIC_CMD_MODE = CFG_MQTT_TOPIC_CMD_MODE;
static const char* const MQTT_TOPIC_CMD_BRIGHTNESS = CFG_MQTT_TOPIC_CMD_BRIGHTNESS;
static const char* const MQTT_TOPIC_CMD_STATE = CFG_MQTT_TOPIC_CMD_STATE;

static const uint32_t WIFI_RETRY_INTERVAL_MS = CFG_WIFI_RETRY_INTERVAL_MS;
static const uint32_t MQTT_RETRY_INTERVAL_MS = CFG_MQTT_RETRY_INTERVAL_MS;
static const uint32_t MQTT_RESTORE_WINDOW_MS = CFG_MQTT_RESTORE_WINDOW_MS;
static const uint32_t MQTT_TEMPERATURE_PUBLISH_INTERVAL_MS = CFG_MQTT_TEMPERATURE_PUBLISH_INTERVAL_MS;
static const uint32_t THERMOMETER_READING_INTERVAL_MS = CFG_THERMOMETER_READING_INTERVAL_MS;
static const uint32_t THERMOMETER_CONVERSION_WAIT_MS = CFG_THERMOMETER_CONVERSION_WAIT_MS;
static const time_t NTP_VALID_EPOCH_THRESHOLD = (time_t)CFG_NTP_VALID_EPOCH_THRESHOLD;

static const size_t MQTT_RX_TOPIC_BUFFER_SIZE = CFG_MQTT_RX_TOPIC_BUFFER_SIZE;
static const size_t MQTT_RX_PAYLOAD_BUFFER_SIZE = CFG_MQTT_RX_PAYLOAD_BUFFER_SIZE;
