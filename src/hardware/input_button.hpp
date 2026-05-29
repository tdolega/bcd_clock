#pragma once

#include <Arduino.h>
#include <Matter.h>
#include "../config/globals.hpp"
#include "../app/matter_sync.hpp"

extern ezButton button;

inline uint16_t next_brightness_level(uint16_t current) {
  constexpr uint16_t presets[] = {2, 8, 128, 4095};
  constexpr int preset_count = sizeof(presets) / sizeof(presets[0]);

  int nearest_index = 0;
  uint32_t nearest_diff = 0xFFFFFFFFu;
  for (int i = 0; i < preset_count; i++) {
    uint32_t diff = (current > presets[i]) ? (current - presets[i]) : (presets[i] - current);
    if (diff < nearest_diff) {
      nearest_diff = diff;
      nearest_index = i;
    }
  }

  int next_index = (nearest_index + 1) % preset_count;
  return presets[next_index];
}

inline void setup_button() {
  button.setDebounceTime(BUTTON_DEBOUNCE_MS);
}
inline void handle_button() {
  button.loop();

  uint32_t now_ms = millis();

  if (app.button_click_count > 0 && (now_ms - app.button_last_click_ts > RESET_CLICK_TIMEOUT_MS)) {
    app.button_click_count = 0;
  }

  if (button.isPressed()) {
    app.button_pressed_ts = now_ms;
    app.button_long_press_handled = false;
  }

  if (button.isReleased()) {
    if (!app.button_long_press_handled) {
      app.mode = (app.mode == M_CLOCK) ? M_THERMOMETER : M_CLOCK;
      app.last_displayed_seconds = -1;
      app.last_displayed_temperature_signature = -1024;

      app.button_click_count++;
      app.button_last_click_ts = now_ms;
      if (app.button_click_count >= RESET_CLICK_TARGET) {
        app.button_click_count = 0;
        Matter.decommission();
        delay(500);
        ESP.restart();
      }
    }
    app.button_pressed_ts = 0;
  }

  if (app.button_pressed_ts == 0) return;

  uint32_t pressed_for = now_ms - app.button_pressed_ts;
  if (pressed_for >= (uint32_t)LONG_PRESS_INITIAL_MS) {
    app.button_long_press_handled = true;
    app.button_click_count = 0;
    app.brightness = next_brightness_level(app.brightness);
    ledcWriteChannel(PWM_CHANNEL_LEDS_ON, app.brightness);
    update_matter_brightness(app.brightness, now_ms);
    app.button_pressed_ts += LONG_PRESS_REPEAT_MS;
  }
}
