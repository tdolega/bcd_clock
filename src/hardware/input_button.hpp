#pragma once
#include "../config/globals.hpp"
#include "../display/modes.hpp"
#include "../display/brightness.hpp"

/**
 * @brief Interrogates GPIO pin matrix for manual push-app.button interaction from user bridging logic.
 */
inline void handle_button(uint32_t now_ms) {
  app.button.loop();

  if(app.button.isPressed()) {
    app.button_pressed_ts = now_ms;
    app.button_long_press_handled = false;
  }

  if(app.button.isReleased()) {
    if(!app.button_long_press_handled) {
      change_to_next_mode();
    }
    app.button_pressed_ts = 0;
  }

  if(app.button_pressed_ts == 0) return;

  uint32_t pressed_for = now_ms - app.button_pressed_ts;
  if(pressed_for >= (uint32_t)LONG_PRESS_INITIAL_MS) {
    app.button_long_press_handled = true;
    change_to_next_brightness_level();
    app.button_pressed_ts += LONG_PRESS_REPEAT_MS;
  }
}
