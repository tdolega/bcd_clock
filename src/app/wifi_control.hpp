#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include "../config/globals.hpp"
#include "../config/wifi_secrets_local.h"
#include "../utils/helpers.hpp"

inline void setup_wifi_blocking() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.setHostname(DEVICE_NAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  app.wifi_connected = true;
  app.wifi_last_attempt_ts = millis();
}

inline void ensure_wifi_connected(uint32_t now_ms) {
  if (WiFi.status() == WL_CONNECTED) {
    app.wifi_connected = true;
    return;
  }

  app.wifi_connected = false;
  if (!elapsed_ms(now_ms, app.wifi_last_attempt_ts, WIFI_RETRY_INTERVAL_MS)) {
    return;
  }

  app.wifi_last_attempt_ts = now_ms;
  WiFi.disconnect(false, true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}
