#pragma once

#include <Arduino.h>
#include <ezButton.h>

#define kHz * 1000

inline const uint32_t PWM_FREQUENCY = 10 kHz;
inline const uint8_t PWM_RESOLUTION = 12; // 1-20 [bits]
inline const int MAIN_LOOP_DELAY_MS = 2;
inline const int BUTTON_DEBOUNCE_MS = 50;

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
inline const int8_t PWM_CHANNEL_LEDS_ON = 1;

typedef enum Modes {
  M_COMMISSIONING,
  M_NTP_SYNC,
  M_CLOCK
} Modes;

struct AppState {
  int leds_state[6][4];
  int8_t leds_channel_cache[6][4];

  uint16_t brightness = 512; // 0-4095
  Modes mode = M_COMMISSIONING;

  int last_displayed_seconds = -1;
  int last_clock_status_signature = -1024;
  
  uint32_t commissioning_animation_state = 0;
  uint32_t ntp_sync_animation_state = 0;
  int button_click_count = 0;
  uint32_t button_last_click_ts = 0;
};

extern AppState app;
