#pragma once
#include "../config/globals.hpp"
#include "../hardware/led_control.hpp"

// Forward declarations
void publish_mode_state();
void publish_state_json();

/**
 * @brief Invalidates the caching constraints for drawing to force re-render.
 */
inline void invalidate_display_cache() {
  app.last_displayed_seconds = -1;
  app.last_displayed_temperature_signature = -1024;
  app.last_clock_status_signature = -1024;
  app.last_displayed_ping_signature = -1024;
}

/**
 * @brief Converts the current app.mode into a string.
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
 * @brief Sets a new operation app.mode and updates LEDs immediately.
 */
inline void set_mode_safe(Modes new_mode, bool publish_state = true) {
  if(app.mode == new_mode) {
    if(publish_state) {
      publish_mode_state();
      publish_state_json();
    }
    return;
  }

  app.mode = new_mode;
  invalidate_display_cache();

  set_all(LOW);
  apply_leds();

  if(publish_state) {
    publish_mode_state();
    publish_state_json();
  }
}

/**
 * @brief Toggles cyclically onto the next app.mode.
 */
inline void change_to_next_mode() {
  switch(app.mode) {
    case M_CLOCK:       set_mode_safe(M_THERMOMETER); break;
    case M_THERMOMETER: set_mode_safe(M_PINGTEST);    break;
    case M_PINGTEST:    set_mode_safe(M_BLANK);       break;
    case M_BLANK:       set_mode_safe(M_CLOCK);       break;
  }
}

/**
 * @brief Parses string token and applies match to app.mode property.
 */
inline bool parse_and_set_mode(const std::string& token, bool publish_state = true) {
  if(token == "clock" || token == "m_clock") {
    set_mode_safe(M_CLOCK, publish_state);
    return true;
  }
  if(token == "thermometer" || token == "m_thermometer" || token == "temp") {
    set_mode_safe(M_THERMOMETER, publish_state);
    return true;
  }
  if(token == "pingtest" || token == "m_pingtest" || token == "ping") {
    set_mode_safe(M_PINGTEST, publish_state);
    return true;
  }
  if(token == "blank" || token == "m_blank" || token == "off") {
    set_mode_safe(M_BLANK, publish_state);
    return true;
  }
  return false;
}
