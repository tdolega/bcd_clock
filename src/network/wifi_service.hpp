#pragma once
#include <WiFi.h>
#include "../config/globals.hpp"
#include "../network/ping_service.hpp" // needed for stop_ping_session

/**
 * @brief System event handler catching successful IP lease provisionings.
 */
inline void on_wifi_connected(WiFiEvent_t event, WiFiEventInfo_t info) {
  (void)event;
  (void)info;

  wifi_connected = true;
  mqtt_connect_due_ts = millis();
  ping_next_due_ts = millis();
}

/**
 * @brief Reacts to lost link properties, forcing tear down of reliant protocols like MQTT.
 */
inline void on_wifi_disconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  (void)event;
  (void)info;

  wifi_connected = false;
  mqtt_connected = false;
  mqtt_restore_active = false;
  ping_latency_ms = PING_NO_RESULT;
  stop_ping_session();
}
