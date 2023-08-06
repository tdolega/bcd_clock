//// INCLUDES

// offline
#include <ezButton.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// online
#include <WiFi.h>
#include <AsyncMqttClient.h>
#include "time.h"

//// DEFINES

#define ever (;;)
#define kHz  * 1000
#define s    * 1000
#define m    * 60 s

//// DYNAMIC CONFIG

const char* WIFI_SSID     = "CHANGE_ME_WIFI_SSID";
const char* WIFI_PASSWORD = "CHANGE_ME_WIFI_PASSWORD";
const char* HOSTNAME      = "bcdClock";

const auto  MQTT_HOST     = IPAddress(0, 0, 0, 0);
const int   MQTT_PORT     = 1883;
const char* MQTT_USER     = "CHANGE_ME_MQTT_USER";
const char* MQTT_PASSWORD = "CHANGE_ME_MQTT_PASSWORD";
const char* MQTT_TOPIC    = "bcdClock/temp";

const char* TIMEZONE_ENV  = "CET-1CEST,M3.5.0,M10.5.0/3"; // Europe/Warsaw according to https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
const char* NTP_SERVER    = "pool.ntp.org";

//// 50/50 CONFIG

const int PWM_FREQUENCY          = 10 kHz;
const int PWM_RESOLUTION         = 8;

const int THERMOMETER_RESOLUTION = 12; // 9-12 [bits]

const int MAIN_LOOP_DELAY_MS     = 20;

const int BUTTON_DEBOUNCE_MS     = MAIN_LOOP_DELAY_MS - 1;

const int LONG_PRESS_INITIAL_MS  = 800;
const int LONG_PRESS_REPEAT_MS   = 400;

const int THERMOMETER_READING_MS = 5 s; // don't try to read temperature more often than this

const int BACKGROUND_THREAD_CORE = 0;
const int BACKGROUND_THREAD_PRIORITY = 1; // 1 - low, 5 - high
const int BACKGROUND_THREAD_STACK_SIZE = 10000;

//// STATIC CONFIG

const int LEDS_COLS = 6;
const int LEDS_ROWS = 4;
const int NO = -1; // no LED on this position
const int GPIO_DIGITS[LEDS_COLS][LEDS_ROWS] = { // position to GPIO mapping
                    // PCB trace:
  {21, 17, NO, NO}, //  5  9
  { 4, 14, 16, 27}, // 11 16 10 17
  {19,  2, 13, NO}, //  6 12 14
  {18, 22, 26, 15}, //  7  2 18 13
  {23, 12,  3, NO}, //  1 15  4
  { 5,  1, 25, 33}  //  8  3 19 20
};
const int digit_to_BCD[10][LEDS_ROWS] = {
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

const int PWM_CHANNEL_LEDS_OFF = 0;
const int PWM_CHANNEL_LEDS_ON  = 1;

const int THERMOMETER_ONE_WIRE_IDX = 0;

//// ENUMS

typedef enum Brightness {
  B_FULL = 255,
  B_HIGH = 80,
  B_MEDIUM = 20,
  B_LOW = 1,
} Brightness;

typedef enum Modes {
  M_CLOCK,
  M_THERMOMETER,
  M_BLANK,
} Modes;

//// STARTUP CONFIG

Brightness brightness = B_LOW;
Modes mode = M_CLOCK;

//// VARIABLES

int LEDS[LEDS_COLS][LEDS_ROWS]; // LEDs states

int button_pressed_ts = 0; // 0: button is not pressed, >0: ts of button press start

float temperature_reading_celsius = 0.0;
int temperature_reading_celsius_floor = 0;
int temperature_request_ts = 0;

int last_displayed_seconds = -1024;
int last_displayed_temperature = -1024;

TaskHandle_t background_thread_handle = NULL;

//// OBJECTS

OneWire one_wire_thermometer(GPIO_PIN_THERMOMETER);
DallasTemperature thermometer(&one_wire_thermometer);

ezButton button(GPIO_PIN_BUTTON);

AsyncMqttClient mqtt_client;

//// FUNCTIONS

void change_to_next_brightness_level() {
  switch(brightness) {
    case B_FULL:       brightness = B_HIGH;    break;
    case B_HIGH:       brightness = B_MEDIUM;  break;
    case B_MEDIUM:     brightness = B_LOW;     break;
    case B_LOW:        brightness = B_FULL;    break;
  }
  ledcWrite(PWM_CHANNEL_LEDS_ON, brightness);
}

void change_to_next_mode() {
  switch(mode) {
    case M_CLOCK:        mode = M_THERMOMETER;     break;
    case M_THERMOMETER:  mode = M_BLANK;           break;
    case M_BLANK:        mode = M_CLOCK;           break;
  }
  // force update on next loop
  last_displayed_seconds = -1024;
  last_displayed_temperature = -1024;
  // clear screen
  set_all_leds(LOW);
}

void set_all_leds(int state) {
  for(int col = 0; col < LEDS_COLS; col++)
    for(int row = 0; row < LEDS_ROWS; row++)
      LEDS[col][row] = state;
  set_leds();
}

void set_digit(int col, int digit) {
  if(digit < 0 || digit > 9) return;
  for(int row = 0; row < LEDS_ROWS; row++)
    LEDS[col][row] = digit_to_BCD[digit][row];
}

void set_number(int group, int number) {
  set_digit(group * 2 + 0, number / 10);
  set_digit(group * 2 + 1, number % 10);
}

void set_leds() {
  for(int col = 0; col < LEDS_COLS; col++){
    for(int row = 0; row < LEDS_ROWS; row++) {
      if(GPIO_DIGITS[col][row] == NO) continue;
      int channel = LEDS[col][row] ? PWM_CHANNEL_LEDS_ON : PWM_CHANNEL_LEDS_OFF;
      ledcAttachPin(GPIO_DIGITS[col][row], channel);
    }
  }
}

void update_temperature() {
  if(millis() - temperature_request_ts < THERMOMETER_READING_MS) return;
  temperature_request_ts = millis();
  thermometer.requestTemperatures();
  temperature_reading_celsius = thermometer.getTempCByIndex(THERMOMETER_ONE_WIRE_IDX);
  temperature_reading_celsius_floor = floor(temperature_reading_celsius);
}

//// RUNTIME

void display_clock() {
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    LEDS[0][0] = HIGH;
    LEDS[0][1] = WiFi.isConnected() ? HIGH : LOW;
    set_leds();
    return;
  }

  // update screen only when needed
  if(last_displayed_seconds == timeinfo.tm_sec) return;
  last_displayed_seconds = timeinfo.tm_sec;

  set_number(0, timeinfo.tm_hour);
  set_number(1, timeinfo.tm_min);
  set_number(2, timeinfo.tm_sec);
  set_leds();
}

void display_temperature() {
  update_temperature();

  // update screen only when needed
  if (last_displayed_temperature == temperature_reading_celsius_floor) return;

  bool is_negative = temperature_reading_celsius < 0;
  LEDS[0][1] = LEDS[1][1] = is_negative ? HIGH : LOW; // minus sign
  set_number(1, temperature_reading_celsius_floor);
  set_leds();

  last_displayed_temperature = temperature_reading_celsius_floor;
}

void background_thread(void* pvParameters) {
  for ever {
    update_temperature();
    delay(35 s);
    mqtt_client.publish(MQTT_TOPIC, 1, true, String(temperature_reading_celsius, 2).c_str());
    delay(35 s);
  }
}

void mqtt_connect_loop() {
  while(!mqtt_client.connected()) {
    if(!WiFi.isConnected()) return;
    mqtt_client.connect();
    delay(10 s);
  }
}

void e_wifi_connected(WiFiEvent_t event, WiFiEventInfo_t info) {
  mqtt_connect_loop();
}

void e_wifi_disconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  //
}

void e_mqtt_disconnected(AsyncMqttClientDisconnectReason reason) {
  mqtt_connect_loop();
}

void setup() {
  // setup LEDs channels
  ledcSetup(PWM_CHANNEL_LEDS_OFF, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcSetup(PWM_CHANNEL_LEDS_ON , PWM_FREQUENCY, PWM_RESOLUTION);
  // setup channels brightness
  ledcWrite(PWM_CHANNEL_LEDS_OFF, 0);
  ledcWrite(PWM_CHANNEL_LEDS_ON , brightness);

  set_all_leds(LOW);

  // setup button
  button.setDebounceTime(BUTTON_DEBOUNCE_MS);

  // setup thermometer
  thermometer.begin();
  thermometer.setResolution(THERMOMETER_RESOLUTION);

  // setup Wi-Fi
  WiFi.onEvent(e_wifi_connected   , ARDUINO_EVENT_WIFI_STA_GOT_IP      );
  WiFi.onEvent(e_wifi_disconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  WiFi.setHostname(HOSTNAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // setup MQTT
  mqtt_client.onDisconnect(e_mqtt_disconnected);
  mqtt_client.setServer(MQTT_HOST, MQTT_PORT);
  mqtt_client.setCredentials(MQTT_USER, MQTT_PASSWORD);

  // setup time
  configTime(0, 0, NTP_SERVER);
  setenv("TZ", TIMEZONE_ENV, 1);
  tzset();

  xTaskCreatePinnedToCore(
    &background_thread,           // function
    "background_thread",          // name
    BACKGROUND_THREAD_STACK_SIZE, // stack size
    NULL,                         // parameters
    BACKGROUND_THREAD_PRIORITY,   // priority
    &background_thread_handle,    // task handle
    BACKGROUND_THREAD_CORE        // core
  );
}

void loop() {
  // handle button
  button.loop();

  if(button.isPressed()) {
    button_pressed_ts = millis();
  }

  if(button.isReleased()) {
    int pressed_for = millis() - button_pressed_ts;
    if (pressed_for < LONG_PRESS_REPEAT_MS) {
      change_to_next_mode();
    }
    button_pressed_ts = 0;
  }

  if(button_pressed_ts > 0) {
    int pressed_for = millis() - button_pressed_ts;
    if (pressed_for > LONG_PRESS_INITIAL_MS) {
      change_to_next_brightness_level();
      button_pressed_ts += LONG_PRESS_REPEAT_MS;
    }
  }

  // display
  switch(mode) {
    case M_CLOCK       : display_clock()       ;break;
    case M_THERMOMETER : display_temperature() ;break;
  }

  // chill
  delay(MAIN_LOOP_DELAY_MS);
}
