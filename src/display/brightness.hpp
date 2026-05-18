#pragma once
#include "../config/globals.hpp"
#include <string>

// Forward declarations
void publish_brightness_state();
void publish_state_json();

/**
 * @brief Translates app.brightness level enum to readable string.
 */
inline const char* brightness_to_string(Brightness current_brightness) {
  switch(current_brightness) {
    case B_FULL:   return "full";
    case B_HIGH:   return "high";
    case B_MEDIUM: return "medium";
    case B_LOW:    return "low";
  }
  return "medium";
}

/**
 * @brief Modifies the LED app.brightness by issuing PWM channel command.
 */
inline void set_brightness_safe(Brightness new_brightness, bool publish_state = true) {
  if(app.brightness == new_brightness) {
    if(publish_state) {
      publish_brightness_state();
      publish_state_json();
    }
    return;
  }

  app.brightness = new_brightness;
  ledcWriteChannel(PWM_CHANNEL_LEDS_ON, app.brightness);

  if(publish_state) {
    publish_brightness_state();
    publish_state_json();
  }
}

/**
 * @brief Cycle to the next dimming level loop.
 */
inline void change_to_next_brightness_level() {
  switch(app.brightness) {
    case B_FULL:   set_brightness_safe(B_HIGH);   break;
    case B_HIGH:   set_brightness_safe(B_MEDIUM); break;
    case B_MEDIUM: set_brightness_safe(B_LOW);    break;
    case B_LOW:    set_brightness_safe(B_FULL);   break;
  }
}

/**
 * @brief Interprets text command and executes app.brightness adjustment.
 */
inline bool parse_and_set_brightness(const std::string& token, bool publish_state = true) {
  if(token == "full" || token == "4095") {
    set_brightness_safe(B_FULL, publish_state);
    return true;
  }
  if(token == "high" || token == "128") {
    set_brightness_safe(B_HIGH, publish_state);
    return true;
  }
  if(token == "medium" || token == "8") {
    set_brightness_safe(B_MEDIUM, publish_state);
    return true;
  }
  if(token == "low" || token == "2") {
    set_brightness_safe(B_LOW, publish_state);
    return true;
  }
  return false;
}
