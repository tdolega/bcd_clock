#include "src/config/globals.hpp"
#include "src/hardware/led_control.hpp"
#include "src/hardware/input_button.hpp"
#include "src/hardware/sensor_temp.hpp"
#include "src/display/display_modes.hpp"
#include "src/app/matter_sync.hpp"
#include "src/app/wifi_control.hpp"
#include "src/app/time_sync.hpp"
#include "src/app/diagnostics.hpp"

#include <Matter.h>

AppState app;
ezButton button(GPIO_PIN_BUTTON);
MatterDimmableLight matterLight;
MatterTemperatureSensor matterTemperatureSensor;

void setup() {
  Serial.end(); 

  for(int col = 0; col < LEDS_COLS; col++) {
    for(int row = 0; row < LEDS_ROWS; row++) {
      app.leds_channel_cache[col][row] = LED_CACHE_INVALID;
      int gpio = GPIO_DIGITS[col][row];
      if (gpio != NO_PIN) {
        pinMode(gpio, OUTPUT);
        digitalWrite(gpio, LOW);
      }
    }
  }

  set_all(LOW);
  apply_leds();
  setup_button();
  setup_temperature_sensor();
  matterTemperatureSensor.begin(-25.0);
  setup_wifi_initial();
  configure_time_sync();
  matterLight.begin();
  
  matterLight.onChange([](bool state, uint8_t brightness) -> bool {
    if (!state) {
      app.brightness = 0;
    } else {
      app.brightness = brightness_to_pwm(brightness);
    }
    app.matter_last_brightness_reported = brightness;
    return true;
  });
  
  Matter.begin();

  uint32_t now_ms = millis();
  app.temperature_last_cycle_ts = 0;
  app.temperature_last_read_ts = now_ms;
  app.matter_last_temperature_report_ts = now_ms;
  app.matter_last_brightness_report_ts = now_ms;
}

void loop() {
  handle_button();
  uint32_t now_ms = millis();

  ensure_wifi_connected(now_ms);

  start_temperature_conversion(now_ms);
  finish_temperature_conversion(now_ms);
  update_matter_temperature(app.temperature_celsius, app.temperature_valid, now_ms);

  if (!app.wifi_connected || is_ntp_stale(now_ms) || !has_recent_temperature(now_ms)) {
    display_diagnostics(now_ms);
  } else {
    switch(app.mode) {
      case M_CLOCK:        display_clock();        break;
      case M_THERMOMETER:  display_temperature();  break;
    }
  }

  apply_leds();
  
  delay(MAIN_LOOP_DELAY_MS);
}
