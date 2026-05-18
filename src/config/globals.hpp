#pragma once

#include <Arduino.h>
#include <ezButton.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <AsyncMqttClient.h>
#include <ping/ping_sock.h>
#include <string>
#include "runtime_config.h"

#define kHz * 1000

inline const uint32_t PWM_FREQUENCY = 10 kHz;
inline const uint8_t PWM_RESOLUTION = 12; // 1-20 [bits]
inline const int THERMOMETER_RESOLUTION = 12; // 9-12 [bits]
inline const int ERROR_DISPLAY_VALUE = 77;
inline const int MAIN_LOOP_DELAY_MS = 2;
inline const int BUTTON_DEBOUNCE_MS = 50;
inline const int LONG_PRESS_INITIAL_MS = 800;
inline const int LONG_PRESS_REPEAT_MS = 400;

inline const int LEDS_COLS = 6;
inline const int LEDS_ROWS = 4;
inline const int NO_PIN = -1;

inline const int GPIO_DIGITS[6][4] = {
  {21, 17, NO_PIN, NO_PIN},
  { 4, 14, 16, 27},
  {19,  2, 13, NO_PIN},
  {18, 22, 26, 15},
  {23, 12,  3, NO_PIN},
  { 5,  1, 25, 33}
};

inline const int DIGIT_TO_BCD[10][4] = {
  { LOW,  LOW,  LOW,  LOW},
  {HIGH,  LOW,  LOW,  LOW},
  { LOW, HIGH,  LOW,  LOW},
  {HIGH, HIGH,  LOW,  LOW},
  { LOW,  LOW, HIGH,  LOW},
  {HIGH,  LOW, HIGH,  LOW},
  { LOW, HIGH, HIGH,  LOW},
  {HIGH, HIGH, HIGH,  LOW},
  { LOW,  LOW,  LOW, HIGH},
  {HIGH,  LOW,  LOW, HIGH},
};

inline const int GPIO_PIN_THERMOMETER = 32;
inline const int GPIO_PIN_BUTTON = 35;

inline const int8_t PWM_CHANNEL_LEDS_ON = 1;

inline const int THERMOMETER_ONE_WIRE_IDX = 0;

inline const uint32_t PING_INTERVAL_MS = 10000;
inline const uint32_t PING_TIMEOUT_MS = 1000;
inline const char* const PING_TARGET_IP = "8.8.8.8";
inline const int PING_NO_RESULT = -1;

typedef enum Brightness {
  B_FULL = (1 << 12) - 1,
  B_HIGH = 128,
  B_MEDIUM = 8,
  B_LOW = 2,
} Brightness;

typedef enum Modes {
  M_CLOCK,
  M_THERMOMETER,
  M_PINGTEST,
  M_BLANK,
} Modes;

struct AppState {
  int leds_state[6][4];
  int8_t leds_channel_cache[6][4];

  Brightness brightness = B_MEDIUM;
  Modes mode = M_CLOCK;

  int last_displayed_seconds = -1;
  int last_displayed_temperature_signature = -1024;
  int last_clock_status_signature = -1024;
  int last_displayed_ping_signature = -1024;

  uint32_t button_pressed_ts = 0;
  bool button_long_press_handled = false;

  float temperature_celsius = 0.0f;
  int temperature_celsius_abs_int = ERROR_DISPLAY_VALUE;
  bool temperature_valid = false;

  int ping_latency_ms = PING_NO_RESULT;
  uint32_t ping_next_due_ts = 0;
  bool ping_session_active = false;
  esp_ping_handle_t ping_session_handle = NULL;
  volatile bool ping_reply_received = false;
  volatile uint32_t ping_reply_time_ms = 0;
  volatile bool ping_session_finished = false;

  bool temperature_conversion_in_progress = false;
  uint32_t temperature_conversion_started_ts = 0;
  uint32_t temperature_last_cycle_ts = 0;
  uint32_t temperature_next_publish_ts = 0;

  bool wifi_connected = false;
  bool mqtt_connected = false;
  uint32_t mqtt_connect_due_ts = 0;

  bool mqtt_restore_active = false;
  uint32_t mqtt_restore_started_ts = 0;

  std::string mqtt_rx_topic;
  std::string mqtt_rx_payload;

  // Components explicitly instanced inside the state
  OneWire one_wire_thermometer{GPIO_PIN_THERMOMETER};
  DallasTemperature thermometer{&one_wire_thermometer};
  ezButton button{GPIO_PIN_BUTTON};
  AsyncMqttClient mqtt_client;
};

inline AppState app;
