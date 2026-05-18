#pragma once
#include "../config/globals.hpp"

/**
 * @brief Sets all LEDs to the given state.
 */
inline void set_all(int state) {
  for(int col = 0; col < LEDS_COLS; col++) {
    for(int row = 0; row < LEDS_ROWS; row++) {
      app.leds_state[col][row] = state;
    }
  }
}

/**
 * @brief Sets a specific column group to represent a single digit in BCD.
 */
inline void set_digit(int col, int digit) {
  if(col < 0 || col >= LEDS_COLS) return;
  if(digit < 0 || digit > 9) return;

  for(int row = 0; row < LEDS_ROWS; row++) {
    app.leds_state[col][row] = DIGIT_TO_BCD[digit][row];
  }
}

/**
 * @brief Sets two columns to display a two-digit number.
 */
inline void set_number(int group, int number) {
  if(number < 0) number = 0;
  if(number > 99) number = 99;

  set_digit(group * 2 + 0, number / 10);
  set_digit(group * 2 + 1, number % 10);
}

/**
 * @brief Applies the requested states to actual GPIOs and PWM channels.
 */
inline void apply_leds() {
  for(int col = 0; col < LEDS_COLS; col++) {
    for(int row = 0; row < LEDS_ROWS; row++) {
      int gpio = GPIO_DIGITS[col][row];
      if(gpio == NO_PIN) continue;

      bool turn_on = app.leds_state[col][row];
      int8_t desired_channel = turn_on ? PWM_CHANNEL_LEDS_ON : -1;

      if(app.leds_channel_cache[col][row] == desired_channel) continue;

      ledcDetach(gpio);
      if(turn_on) {
        ledcAttachChannel(gpio, PWM_FREQUENCY, PWM_RESOLUTION, PWM_CHANNEL_LEDS_ON);
      } else {
        pinMode(gpio, OUTPUT);
        digitalWrite(gpio, LOW);
      }

      app.leds_channel_cache[col][row] = desired_channel;
    }
  }

  ledcWriteChannel(PWM_CHANNEL_LEDS_ON, app.brightness);
}
