#pragma once
#include "../config/globals.hpp"
#include "../utils/helpers.hpp"
#include "../network/mqtt_client.hpp"

inline bool is_valid_temperature(float value) {
  if(value == DEVICE_DISCONNECTED_C) return false;
  if(value < -55.0f || value > 125.0f) return false;
  return true;
}

/**
 * @brief Drives OneWire sequence steps based on waiting times efficiently for app.thermometer values.
 */
inline void update_temperature_cycle(uint32_t now_ms) {
  if(!app.temperature_conversion_in_progress) {
    if(!elapsed_ms(now_ms, app.temperature_last_cycle_ts, THERMOMETER_READING_INTERVAL_MS)) return;

    app.thermometer.requestTemperatures();
    app.temperature_conversion_in_progress = true;
    app.temperature_conversion_started_ts = now_ms;
    return;
  }

  if(!elapsed_ms(now_ms, app.temperature_conversion_started_ts, THERMOMETER_CONVERSION_WAIT_MS)) return;

  app.temperature_conversion_in_progress = false;
  app.temperature_last_cycle_ts = now_ms;

  float reading = app.thermometer.getTempCByIndex(THERMOMETER_ONE_WIRE_IDX);
  if(!is_valid_temperature(reading)) {
    app.temperature_valid = false;
    return;
  }

  app.temperature_valid = true;
  app.temperature_celsius = reading;
  app.temperature_celsius_abs_int = abs((int)round(reading));
}

/**
 * @brief Dictates when to forcibly update MQTT broker with newer recorded values automatically.
 */
inline void publish_periodic_temperature(uint32_t now_ms) {
  if(!app.mqtt_connected) return;
  if(!due_ms(now_ms, app.temperature_next_publish_ts)) return;

  publish_temperature_state();
  publish_state_json();
  app.temperature_next_publish_ts = now_ms + MQTT_TEMPERATURE_PUBLISH_INTERVAL_MS;
}
