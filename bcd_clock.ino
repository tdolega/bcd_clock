#include "src/config/globals.hpp"
#include "src/utils/helpers.hpp"
#include "src/hardware/led_control.hpp"
#include "src/display/modes.hpp"
#include "src/display/brightness.hpp"
#include "src/network/mqtt_client.hpp"
#include "src/network/ping_service.hpp"
#include "src/network/wifi_service.hpp"
#include "src/hardware/sensor_temp.hpp"
#include "src/hardware/input_button.hpp"
#include "src/display/display_modes.hpp"

/**
 * @brief Bootstrapper for hardware orchestration prior to executing cyclical routines.
 */
void setup() {
  for(int col = 0; col < LEDS_COLS; col++) {
    for(int row = 0; row < LEDS_ROWS; row++) {
      app.leds_channel_cache[col][row] = -2;
    }
  }

  set_all(LOW);
  apply_leds();

  app.button.setDebounceTime(BUTTON_DEBOUNCE_MS);

  configTime(0, 0, NTP_SERVER);
  setenv("TZ", TIMEZONE_ENV, 1);
  tzset();

  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.setHostname(HOSTNAME);
  WiFi.onEvent(on_wifi_connected, ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(on_wifi_disconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  app.mqtt_client.onConnect(on_mqtt_connected);
  app.mqtt_client.onDisconnect(on_mqtt_disconnected);
  app.mqtt_client.onMessage(on_mqtt_message);
  app.mqtt_client.setServer(MQTT_HOST, MQTT_PORT);
  app.mqtt_client.setCredentials(MQTT_USER, MQTT_PASSWORD);
  app.mqtt_client.setWill(MQTT_TOPIC_STATE_ONLINE, 1, true, "0");

  app.thermometer.begin();
  app.thermometer.setResolution(THERMOMETER_RESOLUTION);
  app.thermometer.setWaitForConversion(false);

  uint32_t now_ms = millis();
  app.temperature_last_cycle_ts = now_ms - THERMOMETER_READING_INTERVAL_MS;
  app.temperature_next_publish_ts = now_ms + MQTT_TEMPERATURE_PUBLISH_INTERVAL_MS;
  app.ping_next_due_ts = now_ms;
}

/**
 * @brief Central infinite execution loop traversing across active contexts repeatedly.
 */
void loop() {
  uint32_t now_ms = millis();

  handle_button(now_ms);

  ensure_mqtt_connected(now_ms);
  finish_restore_window_if_needed(now_ms);

  update_temperature_cycle(now_ms);
  publish_periodic_temperature(now_ms);
  update_ping_cycle(now_ms);

  switch(app.mode) {
    case M_CLOCK:       display_clock();       break;
    case M_THERMOMETER: display_temperature(); break;
    case M_PINGTEST:    display_pingtest();    break;
    case M_BLANK:       break;
  }

  delay(MAIN_LOOP_DELAY_MS);
}
