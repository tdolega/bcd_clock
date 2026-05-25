#pragma once

#include <Arduino.h>
#include "../config/globals.hpp"

inline void reset_leds() {
  for(int col=0; col<6; col++) {
    for(int row=0; row<4; row++) {
      app.leds_state[col][row] = LOW;
    }
  }
}

inline void set_all(int state) {
  for(int col=0; col<6; col++) {
    for(int row=0; row<4; row++) {
      app.leds_state[col][row] = state;
    }
  }
}

inline void set_digit(int col, int number) {
  if (number < 0 || number > 9) return;
  for(int row=0; row<4; row++) {
    app.leds_state[col][row] = DIGIT_TO_BCD[number][row];
  }
}

inline void set_number(int colHigh, int colLow, int number) {
  int tens = number / 10;
  int units = number % 10;
  set_digit(colHigh, tens);
  set_digit(colLow, units);
}

inline void apply_leds() {
  for(int col=0; col<6; col++) {
    for(int row=0; row<4; row++) {
      int gpio = GPIO_DIGITS[col][row];
      if (gpio == NO_PIN) continue;

      bool turn_on = app.leds_state[col][row];
      int8_t desired_channel = turn_on ? PWM_CHANNEL_LEDS_ON : -1;

      if (app.leds_channel_cache[col][row] != desired_channel) {
        if(turn_on) {
          ledcAttachChannel(gpio, PWM_FREQUENCY, PWM_RESOLUTION, PWM_CHANNEL_LEDS_ON);
        } else {
          ledcDetach(gpio);
          pinMode(gpio, OUTPUT);
          digitalWrite(gpio, LOW);
        }
        app.leds_channel_cache[col][row] = desired_channel;
      }
    }
  }

  ledcWriteChannel(PWM_CHANNEL_LEDS_ON, app.brightness);
}
