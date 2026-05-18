#pragma once
#include "../config/globals.hpp"
#include "../display/modes.hpp"
#include "../display/brightness.hpp"

/**
 * @brief Interrogates GPIO pin matrix for manual push-button interaction from user bridging logic.
 */
inline void handle_button(uint32_t now_ms) {
  button.loop();

  if(button.isPressed()) {
    button_pressed_ts = now_ms;
    button_long_press_handled = false;
  }

  if(button.isReleased()) {
    if(!button_long_press_handled) {
      change_to_next_mode();
    }
    button_pressed_ts = 0;
  }

  if(button_pressed_ts == 0) return;

  uint32_t pressed_for = now_ms - button_pressed_ts;
  if(pressed_for >= (uint32_t)LONG_PRESS_INITIAL_MS) {
    button_long_press_handled = true;
    change_to_next_brightness_level();
    button_pressed_ts += LONG_PRESS_REPEAT_MS;
  }
}
