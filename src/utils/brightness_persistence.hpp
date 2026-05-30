#pragma once

#include <Arduino.h>
#include <Preferences.h>

Preferences brightness_prefs;

inline void save_brightness(uint16_t brightness) {
  brightness_prefs.begin("bcd_clock", false);
  uint16_t existing = brightness_prefs.getUShort("brightness", 0xFFFF);
  if (existing != brightness) {
    brightness_prefs.putUShort("brightness", brightness);
  }
  brightness_prefs.end();
}

inline uint16_t load_brightness() {
  brightness_prefs.begin("bcd_clock", true);
  uint16_t brightness = brightness_prefs.getUShort("brightness", 16);
  brightness_prefs.end();
  return brightness;
}
