#pragma once

#include <Arduino.h>
#include <time.h>
#include "../config/globals.hpp"
#include "../hardware/led_control.hpp"

inline void invalidate_display_cache() {
  app.last_displayed_seconds = -1;
  app.last_clock_status_signature = -1024;
  app.last_displayed_temperature_signature = -1024;
  for(int col=0; col<6; col++) {
    for(int row=0; row<4; row++) {
      app.leds_channel_cache[col][row] = -128;
    }
  }
}

inline bool is_time_synchronized() {
  time_t now;
  time(&now);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  return timeinfo.tm_year > (2020 - 1900);
}

inline void display_clock() {
  time_t now;
  time(&now);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);

  if (!is_time_synchronized()) {
    set_all(LOW);
    apply_leds();
    return;
  }

  if (timeinfo.tm_sec == app.last_displayed_seconds) {
    return;
  }
  
  app.last_displayed_seconds = timeinfo.tm_sec;

  reset_leds();
  set_number(0, 1, timeinfo.tm_hour);
  set_number(2, 3, timeinfo.tm_min);
  set_number(4, 5, timeinfo.tm_sec);
  
  static bool clock_started = false;
  if (!clock_started) {
    Serial.println("Clock synchronized and running! Serial debug will now be quiet.");
    clock_started = true;
  }
}

inline void display_temperature() {
  int value = app.temperature_valid ? app.temperature_celsius_abs_int : ERROR_DISPLAY_VALUE;
  if (value < 0 || value > 99) value = ERROR_DISPLAY_VALUE;

  bool negative = app.temperature_valid ? (app.temperature_celsius < 0.0f) : false;
  int signature = negative ? -value : value;

  if (app.last_displayed_temperature_signature == signature) {
    return;
  }
  app.last_displayed_temperature_signature = signature;

  reset_leds();
  set_number(0, 1, value);
  app.leds_state[5][3] = negative ? HIGH : LOW;
  apply_leds();
}
