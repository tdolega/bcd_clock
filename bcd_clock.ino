//// INCLUDES

// offline
#include <ezButton.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// online
#include <WiFi.h>
#include <AsyncMqttClient.h>
#include "time.h"
#include <ctype.h>
#include <string.h>
#include <ping/ping_sock.h>
#include <lwip/ip_addr.h>
#include "runtime_config.h"

//// DEFINES

#define kHz  * 1000
#define s    * 1000

//// CONFIG

const uint32_t PWM_FREQUENCY  = 10 kHz;
const uint8_t  PWM_RESOLUTION = 12; // 1-20 [bits]

const int THERMOMETER_RESOLUTION = 12; // 9-12 [bits]

const int ERROR_DISPLAY_VALUE = 77; // Zastępcza wartość BCD na wypadek błędów czujnika

const int MAIN_LOOP_DELAY_MS    = 2;
const int BUTTON_DEBOUNCE_MS    = 50; // Standardowy niezależny czas debouncingu
const int LONG_PRESS_INITIAL_MS = 800;
const int LONG_PRESS_REPEAT_MS  = 400;

const int LEDS_COLS = 6;
const int LEDS_ROWS = 4;
const int NO = -1;

const int GPIO_DIGITS[LEDS_COLS][LEDS_ROWS] = {
                    // PCB trace:
  {21, 17, NO, NO}, //  5  9
  { 4, 14, 16, 27}, // 11 16 10 17
  {19,  2, 13, NO}, //  6 12 14
  {18, 22, 26, 15}, //  7  2 18 13
  {23, 12,  3, NO}, //  1 15  4
  { 5,  1, 25, 33}  //  8  3 19 20
};

const int DIGIT_TO_BCD[10][LEDS_ROWS] = {
  { LOW,  LOW,  LOW,  LOW}, // 0
  {HIGH,  LOW,  LOW,  LOW}, // 1
  { LOW, HIGH,  LOW,  LOW}, // 2
  {HIGH, HIGH,  LOW,  LOW}, // 3
  { LOW,  LOW, HIGH,  LOW}, // 4
  {HIGH,  LOW, HIGH,  LOW}, // 5
  { LOW, HIGH, HIGH,  LOW}, // 6
  {HIGH, HIGH, HIGH,  LOW}, // 7
  { LOW,  LOW,  LOW, HIGH}, // 8
  {HIGH,  LOW,  LOW, HIGH}, // 9
};

const int GPIO_PIN_THERMOMETER = 32;
const int GPIO_PIN_BUTTON      = 35;

const int8_t PWM_CHANNEL_LEDS_OFF = 0;
const int8_t PWM_CHANNEL_LEDS_ON  = 1;

const int THERMOMETER_ONE_WIRE_IDX = 0;

const uint32_t PING_INTERVAL_MS = 10 s;
const uint32_t PING_TIMEOUT_MS = 1000;
const char* const PING_TARGET_IP = "8.8.8.8";
const int PING_NO_RESULT = -1;

//// STATE

typedef enum Brightness {
  B_FULL = (1 << PWM_RESOLUTION) - 1,
  B_HIGH = 128,
  B_MEDIUM = 8,
  B_LOW = 2,
} Brightness;

typedef enum Modes {
  M_CLOCK,
  M_THERMOMETER,
  M_PINGTEST,
  M_BLANK,
} Modes;

int leds_state[LEDS_COLS][LEDS_ROWS];
int8_t leds_channel_cache[LEDS_COLS][LEDS_ROWS];

Brightness brightness = B_MEDIUM;
Modes mode = M_CLOCK;

int last_displayed_seconds = -1;
int last_displayed_temperature_signature = -1024;
int last_clock_status_signature = -1024;
int last_displayed_ping_signature = -1024;

uint32_t button_pressed_ts = 0;
bool button_long_press_handled = false;

float temperature_celsius = 0.0f;
int temperature_celsius_abs_int = ERROR_DISPLAY_VALUE;
bool temperature_valid = false;

int ping_latency_ms = PING_NO_RESULT;
uint32_t ping_next_due_ts = 0;
bool ping_session_active = false;
esp_ping_handle_t ping_session_handle = NULL;
volatile bool ping_reply_received = false;
volatile uint32_t ping_reply_time_ms = 0;
volatile bool ping_session_finished = false;

bool temperature_conversion_in_progress = false;
uint32_t temperature_conversion_started_ts = 0;
uint32_t temperature_last_cycle_ts = 0;
uint32_t temperature_next_publish_ts = 0;

bool wifi_connected = false;

bool mqtt_connected = false;
uint32_t mqtt_connect_due_ts = 0;

bool mqtt_restore_active = false;
uint32_t mqtt_restore_started_ts = 0;

char mqtt_rx_topic[MQTT_RX_TOPIC_BUFFER_SIZE];
char mqtt_rx_payload[MQTT_RX_PAYLOAD_BUFFER_SIZE];
size_t mqtt_rx_payload_len = 0;

//// OBJECTS

OneWire one_wire_thermometer(GPIO_PIN_THERMOMETER);
DallasTemperature thermometer(&one_wire_thermometer);
ezButton button(GPIO_PIN_BUTTON);
AsyncMqttClient mqtt_client;

//// HELPERS

bool elapsed_ms(uint32_t now_ms, uint32_t last_ms, uint32_t period_ms) {
  return (uint32_t)(now_ms - last_ms) >= period_ms;
}

bool due_ms(uint32_t now_ms, uint32_t due_ts_ms) {
  return (int32_t)(now_ms - due_ts_ms) >= 0;
}

void trim_and_lowercase(char* buffer) {
  if(buffer == NULL) return;

  size_t len = strlen(buffer);
  size_t start = 0;

  while(start < len && isspace((unsigned char)buffer[start])) start++;
  while(len > start && isspace((unsigned char)buffer[len - 1])) len--;

  size_t out = 0;
  for(size_t i = start; i < len; i++) {
    buffer[out++] = (char)tolower((unsigned char)buffer[i]);
  }
  buffer[out] = '\0';
}

//// LED CONTROL

void set_all(int state) {
  for(int col = 0; col < LEDS_COLS; col++) {
    for(int row = 0; row < LEDS_ROWS; row++) {
      leds_state[col][row] = state;
    }
  }
}

void set_digit(int col, int digit) {
  if(col < 0 || col >= LEDS_COLS) return;
  if(digit < 0 || digit > 9) return;

  for(int row = 0; row < LEDS_ROWS; row++) {
    leds_state[col][row] = DIGIT_TO_BCD[digit][row];
  }
}

void set_number(int group, int number) {
  if(number < 0) number = 0;
  if(number > 99) number = 99;

  set_digit(group * 2 + 0, number / 10);
  set_digit(group * 2 + 1, number % 10);
}

void apply_leds() {
  for(int col = 0; col < LEDS_COLS; col++) {
    for(int row = 0; row < LEDS_ROWS; row++) {
      int gpio = GPIO_DIGITS[col][row];
      if(gpio == NO) continue;

      int8_t desired_channel = leds_state[col][row] ? PWM_CHANNEL_LEDS_ON : PWM_CHANNEL_LEDS_OFF;
      if(leds_channel_cache[col][row] == desired_channel) continue;

      ledcDetach(gpio);
      ledcAttachChannel(gpio, PWM_FREQUENCY, PWM_RESOLUTION, desired_channel);
      leds_channel_cache[col][row] = desired_channel;
    }
  }

  ledcWriteChannel(PWM_CHANNEL_LEDS_OFF, 0);
  ledcWriteChannel(PWM_CHANNEL_LEDS_ON, brightness);
}

//// MODE AND BRIGHTNESS

const char* mode_to_string(Modes current_mode) {
  switch(current_mode) {
    case M_CLOCK:       return "clock";
    case M_THERMOMETER: return "thermometer";
    case M_PINGTEST:    return "pingtest";
    case M_BLANK:       return "blank";
  }
  return "clock";
}

const char* brightness_to_string(Brightness current_brightness) {
  switch(current_brightness) {
    case B_FULL:   return "full";
    case B_HIGH:   return "high";
    case B_MEDIUM: return "medium";
    case B_LOW:    return "low";
  }
  return "medium";
}

void invalidate_display_cache() {
  last_displayed_seconds = -1;
  last_displayed_temperature_signature = -1024;
  last_clock_status_signature = -1024;
  last_displayed_ping_signature = -1024;
}

void publish_mode_state();
void publish_brightness_state();
void publish_temperature_state();
void publish_state_json();
void publish_full_state();

void set_mode(Modes new_mode, bool publish_state = true) {
  if(mode == new_mode) {
    if(publish_state) {
      publish_mode_state();
      publish_state_json();
    }
    return;
  }

  mode = new_mode;
  invalidate_display_cache();

  set_all(LOW);
  apply_leds();

  if(publish_state) {
    publish_mode_state();
    publish_state_json();
  }
}

void change_to_next_mode() {
  switch(mode) {
    case M_CLOCK:       set_mode(M_THERMOMETER); break;
    case M_THERMOMETER: set_mode(M_PINGTEST);    break;
    case M_PINGTEST:    set_mode(M_BLANK);       break;
    case M_BLANK:       set_mode(M_CLOCK);       break;
  }
}

void set_brightness(Brightness new_brightness, bool publish_state = true) {
  if(brightness == new_brightness) {
    if(publish_state) {
      publish_brightness_state();
      publish_state_json();
    }
    return;
  }

  brightness = new_brightness;
  ledcWriteChannel(PWM_CHANNEL_LEDS_ON, brightness);

  if(publish_state) {
    publish_brightness_state();
    publish_state_json();
  }
}

void change_to_next_brightness_level() {
  switch(brightness) {
    case B_FULL:   set_brightness(B_HIGH);   break;
    case B_HIGH:   set_brightness(B_MEDIUM); break;
    case B_MEDIUM: set_brightness(B_LOW);    break;
    case B_LOW:    set_brightness(B_FULL);   break;
  }
}

bool parse_and_set_mode(const char* token, bool publish_state = true) {
  if(strcmp(token, "clock") == 0 || strcmp(token, "m_clock") == 0) {
    set_mode(M_CLOCK, publish_state);
    return true;
  }

  if(strcmp(token, "thermometer") == 0 || strcmp(token, "m_thermometer") == 0 || strcmp(token, "temp") == 0) {
    set_mode(M_THERMOMETER, publish_state);
    return true;
  }

  if(strcmp(token, "pingtest") == 0 || strcmp(token, "m_pingtest") == 0 || strcmp(token, "ping") == 0) {
    set_mode(M_PINGTEST, publish_state);
    return true;
  }

  if(strcmp(token, "blank") == 0 || strcmp(token, "m_blank") == 0 || strcmp(token, "off") == 0) {
    set_mode(M_BLANK, publish_state);
    return true;
  }

  return false;
}

bool parse_and_set_brightness(const char* token, bool publish_state = true) {
  if(strcmp(token, "full") == 0 || strcmp(token, "4095") == 0) {
    set_brightness(B_FULL, publish_state);
    return true;
  }

  if(strcmp(token, "high") == 0 || strcmp(token, "128") == 0) {
    set_brightness(B_HIGH, publish_state);
    return true;
  }

  if(strcmp(token, "medium") == 0 || strcmp(token, "8") == 0) {
    set_brightness(B_MEDIUM, publish_state);
    return true;
  }

  if(strcmp(token, "low") == 0 || strcmp(token, "2") == 0) {
    set_brightness(B_LOW, publish_state);
    return true;
  }

  return false;
}

//// MQTT PUBLISH

void publish_mode_state() {
  if(!mqtt_connected) return;
  mqtt_client.publish(MQTT_TOPIC_STATE_MODE, 1, true, mode_to_string(mode));
}

void publish_brightness_state() {
  if(!mqtt_connected) return;
  mqtt_client.publish(MQTT_TOPIC_STATE_BRIGHTNESS, 1, true, brightness_to_string(brightness));
}

void publish_temperature_state() {
  if(!mqtt_connected) return;

  char payload[32];
  if(temperature_valid) {
    snprintf(payload, sizeof(payload), "%.2f", temperature_celsius);
  } else {
    snprintf(payload, sizeof(payload), "nan");
  }
  mqtt_client.publish(MQTT_TOPIC_STATE_TEMPERATURE, 1, true, payload);
}

void publish_state_json() {
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

void publish_full_state() {
  publish_mode_state();
  publish_brightness_state();
  publish_temperature_state();
  publish_state_json();
}

//// CONNECTIVITY

void ensure_mqtt_connected(uint32_t now_ms) {
  if(!wifi_connected) return;
  if(mqtt_connected) return;
  if(!due_ms(now_ms, mqtt_connect_due_ts)) return;

  mqtt_client.connect();
  mqtt_connect_due_ts = now_ms + MQTT_RETRY_INTERVAL_MS;
}

//// SENSOR

void stop_ping_session() {
  if(!ping_session_active || ping_session_handle == NULL) return;

  esp_ping_stop(ping_session_handle);
  esp_ping_delete_session(ping_session_handle);
  ping_session_handle = NULL;
  ping_session_active = false;
}

void on_ping_success(esp_ping_handle_t hdl, void* args) {
  (void)args;

  uint32_t elapsed_ms = 0;
  uint32_t size = sizeof(elapsed_ms);
  if(esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_ms, size) != ESP_OK) return;

  ping_reply_time_ms = elapsed_ms;
  ping_reply_received = true;
}

void on_ping_timeout(esp_ping_handle_t hdl, void* args) {
  (void)hdl;
  (void)args;
}

void on_ping_end(esp_ping_handle_t hdl, void* args) {
  (void)hdl;
  (void)args;
  ping_session_finished = true;
}

void start_ping_session() {
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

void update_ping_cycle(uint32_t now_ms) {
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

bool is_valid_temperature(float value) {
  if(value == DEVICE_DISCONNECTED_C) return false;
  if(value < -55.0f || value > 125.0f) return false;
  return true;
}

void update_temperature_cycle(uint32_t now_ms) {
  if(!temperature_conversion_in_progress) {
    if(!elapsed_ms(now_ms, temperature_last_cycle_ts, THERMOMETER_READING_INTERVAL_MS)) return;

    thermometer.requestTemperatures();
    temperature_conversion_in_progress = true;
    temperature_conversion_started_ts = now_ms;
    return;
  }

  if(!elapsed_ms(now_ms, temperature_conversion_started_ts, THERMOMETER_CONVERSION_WAIT_MS)) return;

  temperature_conversion_in_progress = false;
  temperature_last_cycle_ts = now_ms;

  float reading = thermometer.getTempCByIndex(THERMOMETER_ONE_WIRE_IDX);
  if(!is_valid_temperature(reading)) {
    temperature_valid = false;
    return;
  }

  temperature_valid = true;
  temperature_celsius = reading;
  temperature_celsius_abs_int = abs((int)round(reading));
}

void publish_periodic_temperature(uint32_t now_ms) {
  if(!mqtt_connected) return;
  if(!due_ms(now_ms, temperature_next_publish_ts)) return;

  publish_temperature_state();
  publish_state_json();
  temperature_next_publish_ts = now_ms + MQTT_TEMPERATURE_PUBLISH_INTERVAL_MS;
}

//// DISPLAY

bool is_time_synchronized(struct tm* out_timeinfo) {
  time_t epoch = time(NULL);
  if(epoch < NTP_VALID_EPOCH_THRESHOLD) return false;
  localtime_r(&epoch, out_timeinfo);
  return true;
}

void display_clock() {
  struct tm timeinfo;
  bool time_ok = is_time_synchronized(&timeinfo);

  if(!time_ok) {
    int status_signature = (wifi_connected ? 1 : 0) | (mqtt_connected ? 2 : 0);
    if(last_clock_status_signature == status_signature) return;

    set_all(LOW);
    leds_state[0][0] = HIGH; // no valid time
    leds_state[0][1] = wifi_connected ? HIGH : LOW;
    leds_state[1][0] = mqtt_connected ? HIGH : LOW;
    apply_leds();

    last_clock_status_signature = status_signature;
    last_displayed_seconds = -1;
    return;
  }

  if(last_displayed_seconds == timeinfo.tm_sec) return;

  last_displayed_seconds = timeinfo.tm_sec;
  last_clock_status_signature = -1024;

  set_number(0, timeinfo.tm_hour);
  set_number(1, timeinfo.tm_min);
  set_number(2, timeinfo.tm_sec);
  apply_leds();
}

void display_temperature() {
  int value = temperature_valid ? temperature_celsius_abs_int : ERROR_DISPLAY_VALUE;
  if(value < 0 || value > 99) value = ERROR_DISPLAY_VALUE;

  bool negative = temperature_valid ? (temperature_celsius < 0.0f) : false;
  int signature = negative ? -value : value;

  if(last_displayed_temperature_signature == signature) return;
  last_displayed_temperature_signature = signature;

  set_all(LOW);
  leds_state[0][1] = leds_state[1][1] = negative ? HIGH : LOW;
  set_number(1, value);
  apply_leds();
}

void display_pingtest() {
  int signature = ping_latency_ms;
  if(last_displayed_ping_signature == signature) return;
  last_displayed_ping_signature = signature;

  set_all(LOW);

  if(ping_latency_ms >= 0) {
    int value = ping_latency_ms;
    if(value > 9999) value = 9999;

    set_digit(2, (value / 1000) % 10);
    set_digit(3, (value / 100) % 10);
    set_digit(4, (value / 10) % 10);
    set_digit(5, value % 10);

    int quality_leds = 1;
    if(value >= 100) {
      quality_leds = 4;
    } else if(value >= 50) {
      quality_leds = 3;
    } else if(value >= 25) {
      quality_leds = 2;
    }

    for(int row = 0; row < quality_leds; row++) {
      leds_state[1][row] = HIGH;
    }
  }

  apply_leds();
}

//// INPUT

void handle_button(uint32_t now_ms) {
  button.loop();

  if(button.isPressed()) {
    button_pressed_ts = now_ms;
    button_long_press_handled = false;
  }

  if(button.isReleased()) {
    if(!button_long_press_handled) {
      change_to_next_mode();
    }
    button_pressed_ts = 0;
  }

  if(button_pressed_ts == 0) return;

  uint32_t pressed_for = now_ms - button_pressed_ts;
  if(pressed_for >= (uint32_t)LONG_PRESS_INITIAL_MS) {
    button_long_press_handled = true;
    change_to_next_brightness_level();
    button_pressed_ts += LONG_PRESS_REPEAT_MS;
  }
}

//// MQTT CALLBACKS

void on_mqtt_connected(bool session_present) {
  (void)session_present;

  mqtt_connected = true;
  mqtt_client.publish(MQTT_TOPIC_STATE_ONLINE, 1, true, "1");

  mqtt_client.subscribe(MQTT_TOPIC_CMD_MODE, 1);
  mqtt_client.subscribe(MQTT_TOPIC_CMD_BRIGHTNESS, 1);
  mqtt_client.subscribe(MQTT_TOPIC_CMD_STATE, 1);

  // Subscribe only during restore window, then unsubscribe.
  mqtt_client.subscribe(MQTT_TOPIC_STATE_MODE, 1);
  mqtt_client.subscribe(MQTT_TOPIC_STATE_BRIGHTNESS, 1);

  mqtt_restore_active = true;
  mqtt_restore_started_ts = millis();
}

void on_mqtt_disconnected(AsyncMqttClientDisconnectReason reason) {
  (void)reason;

  mqtt_connected = false;
  mqtt_restore_active = false;

  if(wifi_connected) {
    mqtt_connect_due_ts = millis() + MQTT_RETRY_INTERVAL_MS;
  }
}

void process_mqtt_message(const char* topic, const char* payload, bool retained) {
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

  // State topics are accepted only during restore and only from retained data.
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

void on_mqtt_message(
  char* topic,
  char* payload,
  AsyncMqttClientMessageProperties properties,
  size_t len,
  size_t index,
  size_t total
) {
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

//// WIFI CALLBACKS

void on_wifi_connected(WiFiEvent_t event, WiFiEventInfo_t info) {
  (void)event;
  (void)info;

  wifi_connected = true;
  mqtt_connect_due_ts = millis();
  ping_next_due_ts = millis();
}

void on_wifi_disconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  (void)event;
  (void)info;

  wifi_connected = false;
  mqtt_connected = false;
  mqtt_restore_active = false;
  ping_latency_ms = PING_NO_RESULT;
  stop_ping_session();
}

//// MQTT RESTORE

void finish_restore_window_if_needed(uint32_t now_ms) {
  if(!mqtt_restore_active) return;
  if(!elapsed_ms(now_ms, mqtt_restore_started_ts, MQTT_RESTORE_WINDOW_MS)) return;

  mqtt_restore_active = false;
  mqtt_client.unsubscribe(MQTT_TOPIC_STATE_MODE);
  mqtt_client.unsubscribe(MQTT_TOPIC_STATE_BRIGHTNESS);

  publish_full_state();
}

//// SETUP/LOOP

void setup() {
  for(int col = 0; col < LEDS_COLS; col++) {
    for(int row = 0; row < LEDS_ROWS; row++) {
      leds_channel_cache[col][row] = -1;
    }
  }

  set_all(LOW);
  apply_leds();

  button.setDebounceTime(BUTTON_DEBOUNCE_MS);

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

  mqtt_client.onConnect(on_mqtt_connected);
  mqtt_client.onDisconnect(on_mqtt_disconnected);
  mqtt_client.onMessage(on_mqtt_message);
  mqtt_client.setServer(MQTT_HOST, MQTT_PORT);
  mqtt_client.setCredentials(MQTT_USER, MQTT_PASSWORD);
  mqtt_client.setWill(MQTT_TOPIC_STATE_ONLINE, 1, true, "0");

  thermometer.begin();
  thermometer.setResolution(THERMOMETER_RESOLUTION);
  thermometer.setWaitForConversion(false);

  uint32_t now_ms = millis();
  temperature_last_cycle_ts = now_ms - THERMOMETER_READING_INTERVAL_MS;
  temperature_next_publish_ts = now_ms + MQTT_TEMPERATURE_PUBLISH_INTERVAL_MS;
  ping_next_due_ts = now_ms;
}

void loop() {
  uint32_t now_ms = millis();

  handle_button(now_ms);

  ensure_mqtt_connected(now_ms);
  finish_restore_window_if_needed(now_ms);

  update_temperature_cycle(now_ms);
  publish_periodic_temperature(now_ms);
  update_ping_cycle(now_ms);

  switch(mode) {
    case M_CLOCK:       display_clock();       break;
    case M_THERMOMETER: display_temperature(); break;
    case M_PINGTEST:    display_pingtest();    break;
    case M_BLANK:       break; // LEDy są czyszczone w momencie zmiany trybu (set_mode)
  }

  delay(MAIN_LOOP_DELAY_MS);
}
