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

inline void setup_temperature_sensor() {
  app.thermometer.begin();
  app.thermometer.setResolution(THERMOMETER_RESOLUTION);
  app.thermometer.setWaitForConversion(false);
}

inline void start_temperature_conversion(uint32_t now_ms) {
  if (app.temperature_conversion_in_progress) return;
  
  // Wymuszenie pomiaru tylko na starcie (0). Zapobiega atakowi DOS na szynę, jeśli czujnik padnie później.
  if (app.temperature_last_cycle_ts != 0 && !elapsed_ms(now_ms, app.temperature_last_cycle_ts, THERMOMETER_READING_INTERVAL_MS)) return;

  app.thermometer.requestTemperatures();
  app.temperature_conversion_in_progress = true;
  app.temperature_conversion_started_ts = now_ms;
}

inline void finish_temperature_conversion(uint32_t now_ms) {
  if (!app.temperature_conversion_in_progress) return;
  if (!elapsed_ms(now_ms, app.temperature_conversion_started_ts, THERMOMETER_CONVERSION_WAIT_MS)) return;

  app.temperature_conversion_in_progress = false;
  app.temperature_last_cycle_ts = now_ms;

  float reading = app.thermometer.getTempCByIndex(THERMOMETER_ONE_WIRE_IDX);
  if (!is_valid_temperature(reading)) {
    app.temperature_valid = false;
    return;
  }

  app.temperature_valid = true;
  app.temperature_celsius = reading;
  app.temperature_celsius_abs_int = abs((int)roundf(reading));
  app.temperature_last_read_ts = now_ms;
}
