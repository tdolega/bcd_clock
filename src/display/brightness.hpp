#pragma once
#include "../config/globals.hpp"

// Forward declarations for MQTT
void publish_brightness_state();
void publish_state_json();

/**
 * @brief Translates brightness level enum to readable string.
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
 * @brief Modifies the LED brightness by issuing PWM channel command.
 */
inline void set_brightness_safe(Brightness new_brightness, bool publish_state = true) {
  if(brightness == new_brightness) {
    if(publish_state) {
      publish_brightness_state();
      publish_state_json();
    }
    return;
  }

  brightness = new_brightness;
  ledcWriteChannel(PWM_CHANNEL_LEDS_ON, brightness);

  if(publish_state) {
    publish_brightness_state();
    publish_state_json();
  }
}

/**
 * @brief Cycle to the next dimming level loop.
 */
inline void change_to_next_brightness_level() {
  switch(brightness) {
    case B_FULL:   set_brightness_safe(B_HIGH);   break;
    case B_HIGH:   set_brightness_safe(B_MEDIUM); break;
    case B_MEDIUM: set_brightness_safe(B_LOW);    break;
    case B_LOW:    set_brightness_safe(B_FULL);   break;
  }
}

/**
 * @brief Interprets text command and executes brightness adjustment.
 */
inline bool parse_and_set_brightness(const char* token, bool publish_state = true) {
  if(strcmp(token, "full") == 0 || strcmp(token, "4095") == 0) {
    set_brightness_safe(B_FULL, publish_state);
    return true;
  }
  if(strcmp(token, "high") == 0 || strcmp(token, "128") == 0) {
    set_brightness_safe(B_HIGH, publish_state);
    return true;
  }
  if(strcmp(token, "medium") == 0 || strcmp(token, "8") == 0) {
    set_brightness_safe(B_MEDIUM, publish_state);
    return true;
  }
  if(strcmp(token, "low") == 0 || strcmp(token, "2") == 0) {
    set_brightness_safe(B_LOW, publish_state);
    return true;
  }
  return false;
}
