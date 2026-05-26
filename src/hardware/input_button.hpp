#pragma once

#include <Arduino.h>
#include "../config/globals.hpp"

extern ezButton button;

inline void setup_button() {
  button.setDebounceTime(BUTTON_DEBOUNCE_MS);
}

inline void handle_button() {
  button.loop();

  if (button.isPressed()) {
    app.button_click_count++;
    app.button_last_click_ts = millis();
    Serial.printf("Button pressed. Toggling mode to %s\n", app.mode == M_CLOCK ? "thermometer" : "clock");
    app.mode = (app.mode == M_CLOCK) ? M_THERMOMETER : M_CLOCK;
    app.last_displayed_seconds = -1;
    app.last_displayed_temperature_signature = -1024;
  }
}
