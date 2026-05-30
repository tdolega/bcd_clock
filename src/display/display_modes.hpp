#pragma once

#include <Arduino.h>
#include <time.h>
#include "../config/globals.hpp"
#include "../hardware/led_control.hpp"
#include "../app/time_sync.hpp"

inline void display_clock() {
  time_t now;
  time(&now);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);

  if (!is_time_synchronized()) {
    set_all(LOW);
    return;
  }

  reset_leds();
  set_number(0, 1, timeinfo.tm_hour);
  set_number(2, 3, timeinfo.tm_min);
  set_number(4, 5, timeinfo.tm_sec);
}

inline void display_temperature() {
  int value = app.temperature_valid ? app.temperature_celsius_abs_int : ERROR_DISPLAY_VALUE;
  if (value < 0 || value > 99) value = ERROR_DISPLAY_VALUE;

  bool negative = app.temperature_valid ? (app.temperature_celsius <= -0.5f) : false;

  reset_leds();
  app.leds_state[0][1] = negative ? HIGH : LOW;
  app.leds_state[1][1] = negative ? HIGH : LOW;
  set_number(2, 3, value);
}
