#include "src/config/globals.hpp"
#include "src/config/wifi_secrets_local.h"
#include "src/hardware/led_control.hpp"
#include "src/hardware/input_button.hpp"
#include "src/hardware/sensor_temp.hpp"
#include "src/display/display_modes.hpp"

#include <Matter.h>
#include <math.h>
#include <WiFi.h>
#include <esp_heap_caps.h>

AppState app;
ezButton button(GPIO_PIN_BUTTON);
MatterDimmableLight matterLight;
MatterTemperatureSensor matterTemperatureSensor;

inline void log_heap(const char* tag) {
  Serial.printf(
    "HEAP[%s] free=%u min=%u largest=%u\n",
    tag,
    (unsigned)ESP.getFreeHeap(),
    (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT),
    (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)
  );
}

inline uint16_t brightness_to_pwm(uint8_t brightness) {
  if (brightness == 0) return 0;

  float normalized = brightness / 254.0f;
  float gamma = powf(normalized, 2.6f);
  uint16_t pwm = (uint16_t)lroundf(gamma * 4095.0f);
  if (pwm == 0) pwm = 1;
  return pwm;
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("Initialising BCD Clock Matter Edition...");
  Serial.flush();
  log_heap("boot");

  Serial.println("BOOT[0] pairing: manual code and QR URL");
  Serial.printf("Matter manual pairing code: %s\n", Matter.getManualPairingCode().c_str());
  Serial.printf("Matter QR code URL: %s\n", Matter.getOnboardingQRCodeUrl().c_str());
  Serial.flush();

  Serial.println("BOOT[1] setup: begin LED init");
  Serial.flush();
  for(int col = 0; col < LEDS_COLS; col++) {
    for(int row = 0; row < LEDS_ROWS; row++) {
      app.leds_channel_cache[col][row] = -2;
      int gpio = GPIO_DIGITS[col][row];
      if (gpio != NO_PIN) {
        pinMode(gpio, OUTPUT);
        digitalWrite(gpio, LOW);
      }
    }
  }

  Serial.println("BOOT[2] setup: applying initial LED state");
  Serial.flush();
  set_all(LOW);
  apply_leds();

  Serial.println("BOOT[3] setup: button configured");
  Serial.flush();
  setup_button();

  Serial.println("BOOT[4] setup: preparing thermometer");
  Serial.flush();
  setup_temperature_sensor();
  matterTemperatureSensor.begin(-25.0);

  Serial.println("BOOT[5] setup: connecting WiFi");
  Serial.flush();
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("Connecting to WiFi SSID: %s\n", WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("BOOT[6] setup: configuring NTP timezone");
  Serial.flush();
  configTime(0, 0, "pool.ntp.org");
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();
  log_heap("after-ntp");

  Serial.println("BOOT[7] setup: creating Matter endpoint");
  Serial.flush();
  matterLight.begin();
  Serial.println("BOOT[8] setup: Matter endpoint begin() returned");
  Serial.flush();

  Serial.println("BOOT[9] setup: registering Matter onChange callback");
  Serial.flush();
  matterLight.onChange([](bool state, uint8_t brightness) -> bool {
    Serial.printf("Matter Callback: state=%d, brightness=%d\n", state, brightness);
    if (!state) {
      app.brightness = 0;
    } else {
      app.brightness = brightness_to_pwm(brightness);
    }
    ledcWriteChannel(PWM_CHANNEL_LEDS_ON, app.brightness);
    return true;
  });

  Serial.println("BOOT[10] setup: starting Matter engine");
  Serial.flush();
  log_heap("before-matter-begin");
  Matter.begin();

  Serial.println("BOOT[11] setup: Matter stack initialized");
  Serial.flush();
  log_heap("after-matter-begin");

  Serial.println("BOOT[12] pairing: manual code and QR URL");
  Serial.printf("Matter manual pairing code: %s\n", Matter.getManualPairingCode().c_str());
  Serial.printf("Matter QR code URL: %s\n", Matter.getOnboardingQRCodeUrl().c_str());
  Serial.flush();

  uint32_t now_ms = millis();
  app.temperature_last_cycle_ts = now_ms - THERMOMETER_READING_INTERVAL_MS;
}

void loop() {
  handle_button();

  static uint32_t last_heap_log_ms = 0;
  uint32_t now_ms = millis();
  if (now_ms - last_heap_log_ms > 30000) {
    log_heap("loop");
    last_heap_log_ms = now_ms;
  }

  start_temperature_conversion(now_ms);
  finish_temperature_conversion(now_ms);

  switch(app.mode) {
    case M_CLOCK:        display_clock();        break;
    case M_THERMOMETER:  display_temperature();  break;
  }

  apply_leds();
  
  delay(MAIN_LOOP_DELAY_MS);
}
