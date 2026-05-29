#pragma once

#include <Arduino.h>
#include <ezButton.h>
#include <DallasTemperature.h>
#include <OneWire.h>

#define kHz * 1000

inline constexpr const char DEVICE_NAME[] = "bcd_clock";

inline const uint32_t PWM_FREQUENCY = 10 kHz;
inline const uint8_t PWM_RESOLUTION = 12; // 1-20 [bits]
inline const int THERMOMETER_RESOLUTION = 12; // 9-12 [bits]
inline const int ERROR_DISPLAY_VALUE = 77;
inline const int MAIN_LOOP_DELAY_MS = 2;
inline const int BUTTON_DEBOUNCE_MS = 50;
inline const int LONG_PRESS_INITIAL_MS = 800;
inline const int LONG_PRESS_REPEAT_MS = 400;
inline const uint32_t RESET_CLICK_TIMEOUT_MS = 3000;
inline const int RESET_CLICK_TARGET = 15;
inline const uint32_t WIFI_RETRY_INTERVAL_MS = 10000;
inline const uint32_t NTP_SYNC_INTERVAL_MS = 6 * 60 * 60 * 1000;
inline const uint32_t MATTER_BRIGHTNESS_REPORT_MIN_MS = 300;
inline const uint32_t MATTER_TEMP_REPORT_MIN_MS = 5000;
inline const uint32_t MATTER_TEMP_REPORT_MAX_MS = 60000;
inline const float MATTER_TEMP_REPORT_MIN_DELTA = 0.1f;
inline const uint32_t DIAGNOSTIC_SLOT_MS = 1000;
inline const int DIAG_CODE_WIFI = 1;
inline const int DIAG_CODE_NTP = 2;
inline const int DIAG_CODE_TEMP = 3;

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

inline const int GPIO_PIN_BUTTON = 35;
inline const int GPIO_PIN_THERMOMETER = 32;
inline const int8_t PWM_CHANNEL_LEDS_ON = 1;
inline const int THERMOMETER_ONE_WIRE_IDX = 0;
inline const uint32_t THERMOMETER_READING_INTERVAL_MS = 5000;
inline const uint32_t THERMOMETER_CONVERSION_WAIT_MS = 800;

typedef enum Modes {
  M_CLOCK,
  M_THERMOMETER
} Modes;

struct AppState {
  int leds_state[6][4];
  int8_t leds_channel_cache[6][4];

  uint16_t brightness = 8; // 0-4095
  Modes mode = M_CLOCK;

  int last_displayed_seconds = -1;
  int last_displayed_temperature_signature = -1024;

  uint32_t button_pressed_ts = 0;
  bool button_long_press_handled = false;
  int button_click_count = 0;
  uint32_t button_last_click_ts = 0;

  bool wifi_connected = false;
  uint32_t wifi_last_attempt_ts = 0;
  uint32_t ntp_last_sync_ts = 0;
  uint8_t matter_last_brightness_reported = 0;
  uint32_t matter_last_brightness_report_ts = 0;
  float matter_last_temperature_reported = -1000.0f;
  uint32_t matter_last_temperature_report_ts = 0;
  uint32_t temperature_last_read_ts = 0;
  
  float temperature_celsius = 0.0f;
  int temperature_celsius_abs_int = ERROR_DISPLAY_VALUE;
  bool temperature_valid = false;
  bool temperature_conversion_in_progress = false;
  uint32_t temperature_conversion_started_ts = 0;
  uint32_t temperature_last_cycle_ts = 0;

  OneWire one_wire_thermometer{GPIO_PIN_THERMOMETER};
  DallasTemperature thermometer{&one_wire_thermometer};
};

extern AppState app;
