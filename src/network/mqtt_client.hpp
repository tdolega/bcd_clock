#pragma once
#include "../config/globals.hpp"
#include "../display/modes.hpp"
#include "../display/brightness.hpp"
#include "../utils/helpers.hpp"

/**
 * @brief Publishes the currently active app.mode string property to MQTT.
 */
inline void publish_mode_state() {
  if(!app.mqtt_connected) return;
  app.mqtt_client.publish(MQTT_TOPIC_STATE_MODE, 1, true, mode_to_string(app.mode));
}

/**
 * @brief Publishes the app.brightness enumeration property to MQTT string representation.
 */
inline void publish_brightness_state() {
  if(!app.mqtt_connected) return;
  app.mqtt_client.publish(MQTT_TOPIC_STATE_BRIGHTNESS, 1, true, brightness_to_string(app.brightness));
}

/**
 * @brief Dispatch sensor string representation reading for temperature value.
 */
inline void publish_temperature_state() {
  if(!app.mqtt_connected) return;
  char payload[32];
  if(app.temperature_valid) {
    snprintf(payload, sizeof(payload), "%.2f", app.temperature_celsius);
  } else {
    snprintf(payload, sizeof(payload), "nan");
  }
  app.mqtt_client.publish(MQTT_TOPIC_STATE_TEMPERATURE, 1, true, payload);
}

/**
 * @brief Composes collective status as JSON payload and broadcasts it over MQTT.
 */
inline void publish_state_json() {
  if(!app.mqtt_connected) return;
  char payload[160];
  if(app.temperature_valid) {
    snprintf(
      payload,
      sizeof(payload),
      "{\"mode\":\"%s\",\"brightness\":\"%s\",\"temperature_c\":%.2f}",
      mode_to_string(app.mode),
      brightness_to_string(app.brightness),
      app.temperature_celsius
    );
  } else {
    snprintf(
      payload,
      sizeof(payload),
      "{\"mode\":\"%s\",\"brightness\":\"%s\",\"temperature_c\":null}",
      mode_to_string(app.mode),
      brightness_to_string(app.brightness)
    );
  }
  app.mqtt_client.publish(MQTT_TOPIC_STATE_JSON, 1, true, payload);
}

/**
 * @brief Orchestrates successive publishment vectors across all discrete statuses.
 */
inline void publish_full_state() {
  publish_mode_state();
  publish_brightness_state();
  publish_temperature_state();
  publish_state_json();
}

/**
 * @brief Asserts and manages periodic routine validations for maintaining server connection.
 */
inline void ensure_mqtt_connected(uint32_t now_ms) {
  if(!app.wifi_connected) return;
  if(app.mqtt_connected) return;
  if(!due_ms(now_ms, app.mqtt_connect_due_ts)) return;

  app.mqtt_client.connect();
  app.mqtt_connect_due_ts = now_ms + MQTT_RETRY_INTERVAL_MS;
}

/**
 * @brief Closes historical retained messages evaluation bracket window.
 */
inline void finish_restore_window_if_needed(uint32_t now_ms) {
  if(!app.mqtt_restore_active) return;
  if(!elapsed_ms(now_ms, app.mqtt_restore_started_ts, MQTT_RESTORE_WINDOW_MS)) return;

  app.mqtt_restore_active = false;
  app.mqtt_client.unsubscribe(MQTT_TOPIC_STATE_MODE);
  app.mqtt_client.unsubscribe(MQTT_TOPIC_STATE_BRIGHTNESS);

  publish_full_state();
}

/**
 * @brief Hook executed by backend context during active networking initialization.
 */
inline void on_mqtt_connected(bool session_present) {
  (void)session_present;

  app.mqtt_connected = true;
  app.mqtt_client.publish(MQTT_TOPIC_STATE_ONLINE, 1, true, "1");

  app.mqtt_client.subscribe(MQTT_TOPIC_CMD_MODE, 1);
  app.mqtt_client.subscribe(MQTT_TOPIC_CMD_BRIGHTNESS, 1);
  app.mqtt_client.subscribe(MQTT_TOPIC_CMD_STATE, 1);

  app.mqtt_client.subscribe(MQTT_TOPIC_STATE_MODE, 1);
  app.mqtt_client.subscribe(MQTT_TOPIC_STATE_BRIGHTNESS, 1);

  app.mqtt_restore_active = true;
  app.mqtt_restore_started_ts = millis();
}

/**
 * @brief Diagnostic fallback routing for server rejection handling callbacks.
 */
inline void on_mqtt_disconnected(AsyncMqttClientDisconnectReason reason) {
  (void)reason;
  app.mqtt_connected = false;
  app.mqtt_restore_active = false;
  if(app.wifi_connected) {
    app.mqtt_connect_due_ts = millis() + MQTT_RETRY_INTERVAL_MS;
  }
}

/**
 * @brief Aggregator interface to intercept topic content.
 */
inline void process_mqtt_message(const std::string& topic, const std::string& payload, bool retained) {
  if(topic == MQTT_TOPIC_CMD_MODE) {
    if(payload == "next") {
      change_to_next_mode();
      return;
    }
    parse_and_set_mode(payload, true);
    return;
  }
  if(topic == MQTT_TOPIC_CMD_BRIGHTNESS) {
    if(payload == "next") {
      change_to_next_brightness_level();
      return;
    }
    parse_and_set_brightness(payload, true);
    return;
  }
  if(topic == MQTT_TOPIC_CMD_STATE) {
    publish_full_state();
    return;
  }
  if(!app.mqtt_restore_active || !retained) return;

  if(topic == MQTT_TOPIC_STATE_MODE) {
    parse_and_set_mode(payload, false);
    return;
  }
  if(topic == MQTT_TOPIC_STATE_BRIGHTNESS) {
    parse_and_set_brightness(payload, false);
    return;
  }
}

/**
 * @brief Callback wrapper to interpret streaming fragmented frames safely with std::string.
 */
inline void on_mqtt_message(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  if(index == 0) {
    app.mqtt_rx_topic = topic;
    app.mqtt_rx_payload.clear();
  }

  app.mqtt_rx_payload.append(payload, len);

  if(index + len < total) return;
  trim_and_lowercase(app.mqtt_rx_payload);
  process_mqtt_message(app.mqtt_rx_topic, app.mqtt_rx_payload, properties.retain);
}
