#include "src/config/globals.hpp"
#include "src/config/wifi_secrets_local.h"
#include "src/hardware/led_control.hpp"
#include "src/hardware/input_button.hpp"
#include "src/display/display_modes.hpp"

#include <Matter.h>
#include <WiFi.h>
#include <esp_heap_caps.h>

AppState app;
ezButton button(GPIO_PIN_BUTTON);
MatterDimmableLight matterLight;

inline void log_heap(const char* tag) {
  Serial.printf(
    "HEAP[%s] free=%u min=%u largest=%u\n",
    tag,
    (unsigned)ESP.getFreeHeap(),
    (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT),
    (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)
  );
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

  Serial.println("BOOT[4] setup: connecting WiFi");
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

  Serial.println("BOOT[5] setup: configuring NTP timezone");
  Serial.flush();
  configTime(0, 0, "pool.ntp.org");
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();
  log_heap("after-ntp");

  Serial.println("BOOT[6] setup: creating Matter endpoint");
  Serial.flush();
  matterLight.begin();
  Serial.println("BOOT[7] setup: Matter endpoint begin() returned");
  Serial.flush();

  Serial.println("BOOT[8] setup: registering Matter onChange callback");
  Serial.flush();
  matterLight.onChange([](bool state, uint8_t brightness) -> bool {
    Serial.printf("Matter Callback: state=%d, brightness=%d\n", state, brightness);
    if (!state) {
      app.brightness = 0;
    } else {
      if (brightness == 0) app.brightness = 0;
      else app.brightness = map(brightness, 1, 254, 1, 4095);
    }
    ledcWriteChannel(PWM_CHANNEL_LEDS_ON, app.brightness);
    return true;
  });

  Serial.println("BOOT[9] setup: starting Matter engine");
  Serial.flush();
  log_heap("before-matter-begin");
  Matter.begin();

  Serial.println("BOOT[10] setup: Matter stack initialized");
  Serial.flush();
  log_heap("after-matter-begin");

  Serial.println("BOOT[11] pairing: manual code and QR URL");
  Serial.printf("Matter manual pairing code: %s\n", Matter.getManualPairingCode().c_str());
  Serial.printf("Matter QR code URL: %s\n", Matter.getOnboardingQRCodeUrl().c_str());
  Serial.flush();
}

void loop() {
  handle_button();

  static uint32_t last_heap_log_ms = 0;
  uint32_t now_ms = millis();
  if (now_ms - last_heap_log_ms > 30000) {
    log_heap("loop");
    last_heap_log_ms = now_ms;
  }

  if (!Matter.isDeviceCommissioned()) {
    app.mode = M_COMMISSIONING;
  } else if (!is_time_synchronized()) {
    app.mode = M_NTP_SYNC;
  } else {
    app.mode = M_CLOCK;
  }

  switch(app.mode) {
    case M_COMMISSIONING: display_commissioning(); break;
    case M_NTP_SYNC:      display_ntp_sync();      break;
    case M_CLOCK:         display_clock();         break;
  }

  apply_leds();
  
  delay(MAIN_LOOP_DELAY_MS);
}
