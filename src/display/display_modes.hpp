#pragma once
#include "../config/globals.hpp"
#include "../hardware/led_control.hpp"
#include <time.h>

/**
 * @brief Checks if time has been synced and populates out struct.
 */
inline bool is_time_synchronized(struct tm* out_timeinfo) {
  time_t epoch = time(NULL);
  if(epoch < NTP_VALID_EPOCH_THRESHOLD) return false;
  localtime_r(&epoch, out_timeinfo);
  return true;
}

/**
 * @brief Retrieves synchronization time safely and parses it to BCD formatted visualization.
 */
inline void display_clock() {
  struct tm timeinfo;
  bool time_ok = is_time_synchronized(&timeinfo);

  if(!time_ok) {
    int status_signature = (app.wifi_connected ? 1 : 0) | (app.mqtt_connected ? 2 : 0);
    if(app.last_clock_status_signature == status_signature) return;

    set_all(LOW);
    app.leds_state[0][0] = HIGH; // no valid time
    app.leds_state[0][1] = app.wifi_connected ? HIGH : LOW;
    app.leds_state[1][0] = app.mqtt_connected ? HIGH : LOW;
    apply_leds();

    app.last_clock_status_signature = status_signature;
    app.last_displayed_seconds = -1;
    return;
  }

  if(app.last_displayed_seconds == timeinfo.tm_sec) return;

  app.last_displayed_seconds = timeinfo.tm_sec;
  app.last_clock_status_signature = -1024;

  set_number(0, timeinfo.tm_hour);
  set_number(1, timeinfo.tm_min);
  set_number(2, timeinfo.tm_sec);
  apply_leds();
}

/**
 * @brief Binds fetched temperature and visualizes it onto board columns considering negative values.
 */
inline void display_temperature() {
  int value = app.temperature_valid ? app.temperature_celsius_abs_int : ERROR_DISPLAY_VALUE;
  if(value < 0 || value > 99) value = ERROR_DISPLAY_VALUE;

  bool negative = app.temperature_valid ? (app.temperature_celsius < 0.0f) : false;
  int signature = negative ? -value : value;

  if(app.last_displayed_temperature_signature == signature) return;
  app.last_displayed_temperature_signature = signature;

  set_all(LOW);
  app.leds_state[0][1] = app.leds_state[1][1] = negative ? HIGH : LOW;
  set_number(1, value);
  apply_leds();
}

/**
 * @brief Extrapolates ICMP metrics representing them visually with a numeric rating logic.
 */
inline void display_pingtest() {
  int signature = app.ping_latency_ms;
  if(app.last_displayed_ping_signature == signature) return;
  app.last_displayed_ping_signature = signature;

  set_all(LOW);

  if(app.ping_latency_ms >= 0) {
    int value = app.ping_latency_ms;
    if(value > 9999) value = 9999;

    set_digit(2, (value / 1000) % 10);
    set_digit(3, (value / 100) % 10);
    set_digit(4, (value / 10) % 10);
    set_digit(5, value % 10);

    int quality_leds = 1;
    if(value >= 100) {
      quality_leds = 4;
    } else if(value >= 50) {
      quality_leds = 3;
    } else if(value >= 25) {
      quality_leds = 2;
    }

    for(int row = 0; row < quality_leds; row++) {
      app.leds_state[1][row] = HIGH;
    }
  }

  apply_leds();
}
