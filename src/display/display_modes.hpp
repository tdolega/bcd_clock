#pragma once

#include <Arduino.h>
#include "../config/globals.hpp"
#include "../hardware/led_control.hpp"

inline void invalidate_display_cache() {
  app.last_displayed_seconds = -1;
  app.last_clock_status_signature = -1024;
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

inline void display_commissioning() {
  reset_leds();
  uint32_t now = millis();
  
  // A snake-like dot looping around the border
  int step = (now / 150) % 16;
  
  int col = 0, row = 0;
  
  if (step < 6) { col = step; row = 0; }
  else if (step < 9) { col = 5; row = step - 5; }
  else if (step < 14) { col = 14 - step; row = 3; }
  else { col = 0; row = 17 - step; }
  
  if (GPIO_DIGITS[col][row] != NO_PIN) {
    app.leds_state[col][row] = HIGH;
  }
  
  // Every time animation cycle resets, print one debug ping to not clutter
  static uint32_t last_anim_cycle = 0;
  if (step == 0 && (now - last_anim_cycle > 1000)) {
    Serial.println("Waiting for Matter Commissioning...");
    last_anim_cycle = now;
  }
}

inline void display_ntp_sync() {
  reset_leds();
  uint32_t now = millis();

  // Blinking horizontal line in the middle
  bool on = (now / 500) % 2 == 0;
  if (on) {
    for(int i = 0; i < 6; ++i) {
      if (GPIO_DIGITS[i][1] != NO_PIN) {
        app.leds_state[i][1] = HIGH;
      }
    }
  }
  
  static uint32_t last_ntp_print = 0;
  if (now - last_ntp_print > 1000) {
    Serial.println("Waiting for NTP Sync...");
    last_ntp_print = now;
  }
}

inline void display_clock() {
  time_t now;
  time(&now);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);

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
