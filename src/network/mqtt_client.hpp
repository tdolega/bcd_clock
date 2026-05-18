#pragma once
#include "../config/globals.hpp"
#include "../display/modes.hpp"
#include "../display/brightness.hpp"
#include "../utils/helpers.hpp"

/**
 * @brief Publishes the currently active mode string property to MQTT.
 */
inline void publish_mode_state() {
  if(!mqtt_connected) return;
  mqtt_client.publish(MQTT_TOPIC_STATE_MODE, 1, true, mode_to_string(mode));
}

/**
 * @brief Publishes the brightness enumeration property to MQTT string representation.
 */
inline void publish_brightness_state() {
  if(!mqtt_connected) return;
  mqtt_client.publish(MQTT_TOPIC_STATE_BRIGHTNESS, 1, true, brightness_to_string(brightness));
}

/**
 * @brief Dispatch sensor string representation reading for temperature value.
 */
inline void publish_temperature_state() {
  if(!mqtt_connected) return;
  char payload[32];
  if(temperature_valid) {
    snprintf(payload, sizeof(payload), "%.2f", temperature_celsius);
  } else {
    snprintf(payload, sizeof(payload), "nan");
  }
  mqtt_client.publish(MQTT_TOPIC_STATE_TEMPERATURE, 1, true, payload);
}

/**
 * @brief Composes collective status as JSON payload and broadcasts it over MQTT.
 */
inline void publish_state_json() {
  if(!mqtt_connected) return;
  char payload[160];
  if(temperature_valid) {
    snprintf(
      payload,
      sizeof(payload),
      "{\"mode\":\"%s\",\"brightness\":\"%s\",\"temperature_c\":%.2f}",
      mode_to_string(mode),
      brightness_to_string(brightness),
      temperature_celsius
    );
  } else {
    snprintf(
      payload,
      sizeof(payload),
      "{\"mode\":\"%s\",\"brightness\":\"%s\",\"temperature_c\":null}",
      mode_to_string(mode),
      brightness_to_string(brightness)
    );
  }
  mqtt_client.publish(MQTT_TOPIC_STATE_JSON, 1, true, payload);
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
  if(!wifi_connected) return;
  if(mqtt_connected) return;
  if(!due_ms(now_ms, mqtt_connect_due_ts)) return;

  mqtt_client.connect();
  mqtt_connect_due_ts = now_ms + MQTT_RETRY_INTERVAL_MS;
}

/**
 * @brief Closes historical retained messages evaluation bracket window.
 */
inline void finish_restore_window_if_needed(uint32_t now_ms) {
  if(!mqtt_restore_active) return;
  if(!elapsed_ms(now_ms, mqtt_restore_started_ts, MQTT_RESTORE_WINDOW_MS)) return;

  mqtt_restore_active = false;
  mqtt_client.unsubscribe(MQTT_TOPIC_STATE_MODE);
  mqtt_client.unsubscribe(MQTT_TOPIC_STATE_BRIGHTNESS);

  publish_full_state();
}

/**
 * @brief Hook executed by backend context during active networking initialization.
 */
inline void on_mqtt_connected(bool session_present) {
  (void)session_present;

  mqtt_connected = true;
  mqtt_client.publish(MQTT_TOPIC_STATE_ONLINE, 1, true, "1");

  mqtt_client.subscribe(MQTT_TOPIC_CMD_MODE, 1);
  mqtt_client.subscribe(MQTT_TOPIC_CMD_BRIGHTNESS, 1);
  mqtt_client.subscribe(MQTT_TOPIC_CMD_STATE, 1);

  mqtt_client.subscribe(MQTT_TOPIC_STATE_MODE, 1);
  mqtt_client.subscribe(MQTT_TOPIC_STATE_BRIGHTNESS, 1);

  mqtt_restore_active = true;
  mqtt_restore_started_ts = millis();
}

/**
 * @brief Diagnostic fallback routing for server rejection handling callbacks.
 */
inline void on_mqtt_disconnected(AsyncMqttClientDisconnectReason reason) {
  (void)reason;
  mqtt_connected = false;
  mqtt_restore_active = false;
  if(wifi_connected) {
    mqtt_connect_due_ts = millis() + MQTT_RETRY_INTERVAL_MS;
  }
}

/**
 * @brief Aggregator interface to intercept topic content.
 */
inline void process_mqtt_message(const char* topic, const char* payload, bool retained) {
  if(strcmp(topic, MQTT_TOPIC_CMD_MODE) == 0) {
    if(strcmp(payload, "next") == 0) {
      change_to_next_mode();
      return;
    }
    parse_and_set_mode(payload, true);
    return;
  }
  if(strcmp(topic, MQTT_TOPIC_CMD_BRIGHTNESS) == 0) {
    if(strcmp(payload, "next") == 0) {
      change_to_next_brightness_level();
      return;
    }
    parse_and_set_brightness(payload, true);
    return;
  }
  if(strcmp(topic, MQTT_TOPIC_CMD_STATE) == 0) {
    publish_full_state();
    return;
  }
  if(!mqtt_restore_active || !retained) return;

  if(strcmp(topic, MQTT_TOPIC_STATE_MODE) == 0) {
    parse_and_set_mode(payload, false);
    return;
  }
  if(strcmp(topic, MQTT_TOPIC_STATE_BRIGHTNESS) == 0) {
    parse_and_set_brightness(payload, false);
    return;
  }
}

/**
 * @brief Callback wrapper to interpret streaming fragmented frames correctly from core hooks.
 */
inline void on_mqtt_message(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  if(index == 0) {
    strncpy(mqtt_rx_topic, topic, sizeof(mqtt_rx_topic) - 1);
    mqtt_rx_topic[sizeof(mqtt_rx_topic) - 1] = '\0';
    mqtt_rx_payload_len = 0;
  }
  size_t available = sizeof(mqtt_rx_payload) - 1 - mqtt_rx_payload_len;
  size_t to_copy = (len < available) ? len : available;
  memcpy(&mqtt_rx_payload[mqtt_rx_payload_len], payload, to_copy);
  mqtt_rx_payload_len += to_copy;

  if(index + len < total) return;
  mqtt_rx_payload[mqtt_rx_payload_len] = '\0';
  trim_and_lowercase(mqtt_rx_payload);
  process_mqtt_message(mqtt_rx_topic, mqtt_rx_payload, properties.retain);
}
