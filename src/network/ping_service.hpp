#pragma once
#include "../config/globals.hpp"
#include "../utils/helpers.hpp"

/**
 * @brief Halts an existing and active ICMP ping testing suite operation completely.
 */
inline void stop_ping_session() {
  if(!ping_session_active || ping_session_handle == NULL) return;

  esp_ping_stop(ping_session_handle);
  esp_ping_delete_session(ping_session_handle);
  ping_session_handle = NULL;
  ping_session_active = false;
}

inline void on_ping_success(esp_ping_handle_t hdl, void* args) {
  uint32_t elapsed_ms = 0;
  uint32_t size = sizeof(elapsed_ms);
  if(esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_ms, size) != ESP_OK) return;

  ping_reply_time_ms = elapsed_ms;
  ping_reply_received = true;
}

inline void on_ping_timeout(esp_ping_handle_t hdl, void* args) {
  // Empty implementation as in original
}

inline void on_ping_end(esp_ping_handle_t hdl, void* args) {
  ping_session_finished = true;
}

inline void start_ping_session() {
  esp_ping_config_t ping_config = ESP_PING_DEFAULT_CONFIG();
  ping_config.count = 1;
  ping_config.interval_ms = 1;
  ping_config.timeout_ms = PING_TIMEOUT_MS;

  if(!ipaddr_aton(PING_TARGET_IP, &ping_config.target_addr)) return;

  esp_ping_callbacks_t ping_callbacks;
  memset(&ping_callbacks, 0, sizeof(ping_callbacks));
  ping_callbacks.on_ping_success = on_ping_success;
  ping_callbacks.on_ping_timeout = on_ping_timeout;
  ping_callbacks.on_ping_end = on_ping_end;

  ping_reply_received = false;
  ping_reply_time_ms = 0;
  ping_session_finished = false;

  if(esp_ping_new_session(&ping_config, &ping_callbacks, &ping_session_handle) != ESP_OK) return;
  if(esp_ping_start(ping_session_handle) != ESP_OK) {
    esp_ping_delete_session(ping_session_handle);
    ping_session_handle = NULL;
    return;
  }

  ping_session_active = true;
}

/**
 * @brief Evaluates whether it's appropriate to trigger ICMP process right now, scheduling bounds.
 */
inline void update_ping_cycle(uint32_t now_ms) {
  if(mode != M_PINGTEST) {
    stop_ping_session();
    return;
  }

  if(!wifi_connected) {
    ping_latency_ms = PING_NO_RESULT;
    stop_ping_session();
    ping_next_due_ts = now_ms;
    return;
  }

  if(ping_session_active && ping_session_finished) {
    ping_latency_ms = ping_reply_received ? (int)ping_reply_time_ms : PING_NO_RESULT;
    stop_ping_session();
    ping_next_due_ts = now_ms + PING_INTERVAL_MS;
    return;
  }

  if(ping_session_active) return;
  if(!due_ms(now_ms, ping_next_due_ts)) return;

  start_ping_session();
  if(!ping_session_active) {
    ping_latency_ms = PING_NO_RESULT;
    ping_next_due_ts = now_ms + PING_INTERVAL_MS;
  }
}
