#pragma once

#include <Arduino.h>
#include <Preferences.h>

Preferences brightness_prefs;

inline void save_brightness(uint16_t brightness) {
  brightness_prefs.begin("bcd_clock", false);
  brightness_prefs.putUInt("brightness", brightness);
  brightness_prefs.end();
}

inline uint16_t load_brightness() {
  brightness_prefs.begin("bcd_clock", true);
  uint16_t brightness = brightness_prefs.getUInt("brightness", 128);
  brightness_prefs.end();
  return brightness;
}
