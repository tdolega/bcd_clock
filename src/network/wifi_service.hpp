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

    app.wifi_connected = true;
    app.mqtt_connect_due_ts = millis();
    app.ping_next_due_ts = millis();
}

/**
 * @brief Reacts to lost link properties, forcing tear down of reliant protocols like MQTT.
 */
inline void on_wifi_disconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    (void)event;
    (void)info;

    app.wifi_connected = false;
    app.mqtt_connected = false;
    app.mqtt_restore_active = false;
    app.ping_latency_ms = PING_NO_RESULT;
    stop_ping_session();
}
