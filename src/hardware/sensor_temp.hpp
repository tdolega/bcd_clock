#pragma once

#include <Arduino.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include "../config/globals.hpp"
#include "../utils/helpers.hpp"

inline bool is_valid_temperature(float value) {
  if (value == DEVICE_DISCONNECTED_C) return false;
  if (value < -55.0f || value > 125.0f) return false;
  return true;
}

inline void store_temperature_reading(float reading, uint32_t now_ms) {
  app.temperature_valid = true;
  app.temperature_celsius = reading;
  app.temperature_celsius_abs_int = abs((int)roundf(reading));
  app.temperature_last_read_ts = now_ms;
}

inline void update_temperature_init_heartbeat(bool state) {
  int heartbeat_gpio = GPIO_DIGITS[0][0];
  pinMode(heartbeat_gpio, OUTPUT);
  digitalWrite(heartbeat_gpio, state ? HIGH : LOW);
}

inline float setup_temperature_sensor() {
  app.thermometer.begin();
  app.thermometer.setResolution(THERMOMETER_RESOLUTION);
  app.thermometer.setWaitForConversion(true);
  bool heartbeat_state = false;

  while (true) {
    app.thermometer.requestTemperatures();

    float reading = app.thermometer.getTempCByIndex(THERMOMETER_ONE_WIRE_IDX);
    if (is_valid_temperature(reading)) {
      update_temperature_init_heartbeat(false);
      uint32_t now_ms = millis();
      store_temperature_reading(reading, now_ms);
      app.temperature_conversion_in_progress = false;
      app.temperature_conversion_started_ts = 0;
      app.temperature_last_cycle_ts = now_ms;
      app.thermometer.setWaitForConversion(false);
      return reading;
    }

    heartbeat_state = !heartbeat_state;
    update_temperature_init_heartbeat(heartbeat_state);
    delay(250);
  }
}

inline void start_temperature_conversion(uint32_t now_ms) {
  if (app.temperature_conversion_in_progress) return;
  
  if (app.temperature_last_cycle_ts != 0 && !elapsed_ms(now_ms, app.temperature_last_cycle_ts, THERMOMETER_READING_INTERVAL_MS)) return;

  app.thermometer.requestTemperatures();
  app.temperature_conversion_in_progress = true;
  app.temperature_conversion_started_ts = now_ms;
}

inline bool finish_temperature_conversion(uint32_t now_ms) {
  if (!app.temperature_conversion_in_progress) return false;
  if (!elapsed_ms(now_ms, app.temperature_conversion_started_ts, THERMOMETER_CONVERSION_WAIT_MS)) return false;

  app.temperature_conversion_in_progress = false;
  app.temperature_last_cycle_ts = now_ms;

  float reading = app.thermometer.getTempCByIndex(THERMOMETER_ONE_WIRE_IDX);
  if (!is_valid_temperature(reading)) {
    app.temperature_valid = false;
    return false;
  }

  store_temperature_reading(reading, now_ms);
  return true;
}
