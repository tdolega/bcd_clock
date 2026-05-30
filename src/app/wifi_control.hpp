#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include "../config/globals.hpp"
#include "../config/wifi_secrets_local.h"
#include "../utils/helpers.hpp"

inline void setup_wifi_initial() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.setHostname(DEVICE_NAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  uint32_t start_ms = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start_ms < 5000)) {
    delay(100);
  }
  
  app.wifi_connected = (WiFi.status() == WL_CONNECTED);
}

inline void ensure_wifi_connected() {
  if (WiFi.status() == WL_CONNECTED) {
    app.wifi_connected = true;
    return;
  }

  app.wifi_connected = false;
}
