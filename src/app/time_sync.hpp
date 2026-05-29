#pragma once

#include <Arduino.h>
#include <time.h>
#include <esp_sntp.h>
#include "../config/globals.hpp"
#include "../utils/helpers.hpp"

inline constexpr const char NTP_SERVER[] = "pool.ntp.org";
inline constexpr const char TZ_ENV[] = "CET-1CEST,M3.5.0,M10.5.0/3";

inline void time_sync_notification_cb(struct timeval *tv) {
  app.ntp_last_sync_ts = millis();
}

inline void configure_time_sync() {
  sntp_set_time_sync_notification_cb(time_sync_notification_cb);
  configTime(0, 0, NTP_SERVER);
  setenv("TZ", TZ_ENV, 1);
  tzset();
  sntp_set_sync_interval(NTP_SYNC_INTERVAL_MS);
  sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
}

inline bool is_time_synchronized() {
  time_t now;
  time(&now);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  return timeinfo.tm_year > (2020 - 1900);
}

inline bool is_ntp_stale(uint32_t now_ms) {
  if (!is_time_synchronized()) return true;
  return elapsed_ms(now_ms, app.ntp_last_sync_ts, NTP_SYNC_INTERVAL_MS * 2);
}
