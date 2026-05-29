#pragma once

#include <Arduino.h>
#include <math.h>
#include <Matter.h>
#include "../config/globals.hpp"
#include "../utils/helpers.hpp"

extern MatterDimmableLight matterLight;
extern MatterTemperatureSensor matterTemperatureSensor;

inline uint16_t brightness_to_pwm(uint8_t brightness) {
  if (brightness == 0) return 0;

  float normalized = brightness / 254.0f;
  float gamma = powf(normalized, 2.6f);
  uint16_t pwm = (uint16_t)lroundf(gamma * 4095.0f);
  if (pwm == 0) pwm = 1;
  return pwm;
}

inline uint8_t pwm_to_matter_brightness(uint16_t pwm) {
  if (pwm == 0) return 0;
  float normalized = (float)pwm / 4095.0f;
  float linear = powf(normalized, 1.0f / 2.6f);
  int value = (int)lroundf(linear * 254.0f);
  if (value < 1) value = 1;
  if (value > 254) value = 254;
  return (uint8_t)value;
}

inline void update_matter_brightness(uint16_t pwm, uint32_t now_ms) {
  uint8_t level = pwm_to_matter_brightness(pwm);
  bool state = pwm > 0;
  if (app.matter_last_brightness_reported == level && matterLight.getOnOff() == state) {
    return;
  }
  if (!elapsed_ms(now_ms, app.matter_last_brightness_report_ts, MATTER_BRIGHTNESS_REPORT_MIN_MS)) {
    return;
  }

  if (matterLight.getOnOff() != state) {
    matterLight.setOnOff(state);
  }
  if (matterLight.getBrightness() != level) {
    matterLight.setBrightness(level);
  }
  app.matter_last_brightness_reported = level;
  app.matter_last_brightness_report_ts = now_ms;
}

inline void update_matter_temperature(float temperature, bool valid, uint32_t now_ms) {
  if (!valid) return;

  float delta = fabsf(temperature - app.matter_last_temperature_reported);
  bool min_elapsed = elapsed_ms(now_ms, app.matter_last_temperature_report_ts, MATTER_TEMP_REPORT_MIN_MS);
  bool max_elapsed = elapsed_ms(now_ms, app.matter_last_temperature_report_ts, MATTER_TEMP_REPORT_MAX_MS);
  if (!min_elapsed && !max_elapsed) {
    return;
  }
  if (delta < MATTER_TEMP_REPORT_MIN_DELTA && !max_elapsed) {
    return;
  }

  matterTemperatureSensor.setTemperature(temperature);
  app.matter_last_temperature_reported = temperature;
  app.matter_last_temperature_report_ts = now_ms;
}
