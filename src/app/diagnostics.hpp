#pragma once

#include <Arduino.h>
#include "../config/globals.hpp"
#include "../utils/helpers.hpp"
#include "../hardware/led_control.hpp"
#include "time_sync.hpp"

inline bool has_recent_temperature(uint32_t now_ms) {
  return elapsed_ms(now_ms, app.temperature_last_read_ts, THERMOMETER_READING_INTERVAL_MS * 3) == false;
}

inline void display_diagnostics(uint32_t now_ms) {
  int codes[3];
  int code_count = 0;

  if (!app.wifi_connected) {
    codes[code_count++] = DIAG_CODE_WIFI;
  }
  if (is_ntp_stale(now_ms)) {
    codes[code_count++] = DIAG_CODE_NTP;
  }
  if (!has_recent_temperature(now_ms)) {
    codes[code_count++] = DIAG_CODE_TEMP;
  }

  if (code_count == 0) {
    return;
  }

  int slot = (now_ms / DIAGNOSTIC_SLOT_MS) % code_count;
  int code = codes[slot];

  reset_leds();
  if (code == DIAG_CODE_WIFI) {
    app.leds_state[0][0] = HIGH;
    app.leds_state[1][0] = HIGH;
  } else if (code == DIAG_CODE_NTP) {
    app.leds_state[0][1] = HIGH;
    app.leds_state[1][1] = HIGH;
  } else if (code == DIAG_CODE_TEMP) {
    app.leds_state[0][2] = HIGH;
    app.leds_state[1][2] = HIGH;
  }
  apply_leds();
}
