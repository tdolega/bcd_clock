#pragma once

#include <Arduino.h>
#include <Matter.h>
#include "../config/globals.hpp"

extern ezButton button;

inline void setup_button() {
  button.setDebounceTime(BUTTON_DEBOUNCE_MS);
}

inline void handle_button() {
  button.loop();

  uint32_t now = millis();

  // Reset click count after 3 seconds of inactivity
  if (app.button_click_count > 0 && (now - app.button_last_click_ts > 3000)) {
    Serial.printf("Button click timeout, resetting count from %d to 0\n", app.button_click_count);
    app.button_click_count = 0;
  }

  if (button.isPressed()) {
    app.button_click_count++;
    app.button_last_click_ts = now;
    Serial.printf("Button pressed. Count: %d/16\n", app.button_click_count);

    if (app.button_click_count >= 16) {
      Serial.println("16 consecutive clicks detected. Initiating Matter Factory Reset (decommission)!");
      for (int i = 0; i < 5; i++) {
        set_all(HIGH);
        apply_leds();
        delay(100);
        set_all(LOW);
        apply_leds();
        delay(100);
      }
      
      // Delay to let user release button
      delay(1000);
      Matter.decommission();
      delay(500);
      ESP.restart();
    }
  }
}
