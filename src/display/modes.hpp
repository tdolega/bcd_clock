#pragma once
#include "../config/globals.hpp"
#include "../hardware/led_control.hpp"

// Forward declarations to break cyclic dependency with mqtt_client
void publish_mode_state();
void publish_state_json();

/**
 * @brief Invalidates the caching constraints for drawing to force re-render.
 */
inline void invalidate_display_cache() {
  last_displayed_seconds = -1;
  last_displayed_temperature_signature = -1024;
  last_clock_status_signature = -1024;
  last_displayed_ping_signature = -1024;
}

/**
 * @brief Converts the current mode into a string.
 */
inline const char* mode_to_string(Modes current_mode) {
  switch(current_mode) {
    case M_CLOCK:       return "clock";
    case M_THERMOMETER: return "thermometer";
    case M_PINGTEST:    return "pingtest";
    case M_BLANK:       return "blank";
  }
  return "clock";
}

/**
 * @brief Sets a new operation mode and updates LEDs immediately.
 */
inline void set_mode_safe(Modes new_mode, bool publish_state = true) {
  if(mode == new_mode) {
    if(publish_state) {
      publish_mode_state();
      publish_state_json();
    }
    return;
  }

  mode = new_mode;
  invalidate_display_cache();

  set_all(LOW);
  apply_leds();

  if(publish_state) {
    publish_mode_state();
    publish_state_json();
  }
}

/**
 * @brief Toggles cyclically onto the next mode.
 */
inline void change_to_next_mode() {
  switch(mode) {
    case M_CLOCK:       set_mode_safe(M_THERMOMETER); break;
    case M_THERMOMETER: set_mode_safe(M_PINGTEST);    break;
    case M_PINGTEST:    set_mode_safe(M_BLANK);       break;
    case M_BLANK:       set_mode_safe(M_CLOCK);       break;
  }
}

/**
 * @brief Parses string token and applies match to mode property.
 */
inline bool parse_and_set_mode(const char* token, bool publish_state = true) {
  if(strcmp(token, "clock") == 0 || strcmp(token, "m_clock") == 0) {
    set_mode_safe(M_CLOCK, publish_state);
    return true;
  }
  if(strcmp(token, "thermometer") == 0 || strcmp(token, "m_thermometer") == 0 || strcmp(token, "temp") == 0) {
    set_mode_safe(M_THERMOMETER, publish_state);
    return true;
  }
  if(strcmp(token, "pingtest") == 0 || strcmp(token, "m_pingtest") == 0 || strcmp(token, "ping") == 0) {
    set_mode_safe(M_PINGTEST, publish_state);
    return true;
  }
  if(strcmp(token, "blank") == 0 || strcmp(token, "m_blank") == 0 || strcmp(token, "off") == 0) {
    set_mode_safe(M_BLANK, publish_state);
    return true;
  }
  return false;
}
