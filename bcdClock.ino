#include <ezButton.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <AsyncMqttClient.h>

#define ever (;;)

const char* HOME_SSID     = "CHANGE_ME_WIFI_SSID";
const char* HOME_PASSWORD = "CHANGE_ME_WIFI_PASSWORD";

const auto MQTT_HOST = IPAddress(0, 0, 0, 0);
const int MQTT_PORT = 1883;
const char* MQTT_USER     = "CHANGE_ME_MQTT_USER";
const char* MQTT_PASSWORD = "CHANGE_ME_MQTT_PASSWORD";
const char* MQTT_TOPIC    = "bcdClock/temp";
AsyncMqttClient mqttClient;

const int LEDS_COLS = 6;
const int LEDS_ROWS = 4;
int LEDS[LEDS_COLS][LEDS_ROWS]; // LEDs states
const int NO = -1; // no LED on this position
const int GPIO_DIGITS[LEDS_COLS][LEDS_ROWS] = { // position to GPIO mapping
                    // PCB trace:
  {21, 17, NO, NO}, // 5 9
  { 4, 14, 16, 27}, // 11 16 10 17
  {19,  2, 13, NO}, // 6 12 14
  {18, 22, 26, 15}, // 7 2 18 13
  {23, 12,  3, NO}, // 1 15 4
  { 5,  1, 25, 33}  // 8 3 19 20
};
const int digitToBCD[10][LEDS_ROWS] = {
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

typedef enum Brightness {
  B_FULL,
  B_HIGH,
  B_MEDIUM,
  B_LOW,
} Brightness;
Brightness brightness = B_HIGH;
void changeBrightness() {
  switch(brightness) {
    case B_FULL:    brightness = B_HIGH;    break;
    case B_HIGH:    brightness = B_MEDIUM;  break;
    case B_MEDIUM:  brightness = B_LOW;     break;
    case B_LOW:     brightness = B_FULL;    break;
  }
}

typedef enum Modes {
  CLOCK,
  THERMOMETER,
  BLANK,
  ERR,
} Modes;
Modes mode = ERR;
Modes lastMode = CLOCK;
void changeMode() {
    switch(mode) {
      case CLOCK:        mode = THERMOMETER;   break;
      case THERMOMETER:  mode = BLANK;         break;
      case BLANK:        mode = CLOCK;         break;
      case ERR:          break;
    }
    createMainThread();
}

int err = 0; // 0 - WiFi, 1 - network

const int GPIO_THERMOMETER = 32;
OneWire oneWireThermometer(GPIO_THERMOMETER);
DallasTemperature thermometer(&oneWireThermometer);

const int GPIO_BUTTON = 35;
ezButton button(GPIO_BUTTON);
const int DEBOUNCE_TIME = 50; // [ms]
int pressedTs = 0; // 0: button is not pressed, >0: button is pressed for x amount of time

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

TaskHandle_t mainThreadHandle = NULL;
TaskHandle_t mqttThreadHandle = NULL;

float temperature = 0;
int temperatureTs = 0; // timestamp of last temperature reading

//// HELPERS

void setAll(int state) {
  for(int col = 0; col < LEDS_COLS; col++)
    for(int row = 0; row < LEDS_ROWS; row++)
      LEDS[col][row] = state;
}

void setDigit(int col, int digit) {
  if(digit < 0 || digit > 9) return;
  for(int row = 0; row < LEDS_ROWS; row++)
    LEDS[col][row] = digitToBCD[digit][row];
}

void setNumber(int group, int number) {
  setDigit(group * 2 + 0, number / 10);
  setDigit(group * 2 + 1, number % 10);
}

void updateTemperature() {
  if(millis() - temperatureTs < 1000) return;
  temperatureTs = millis();
  thermometer.requestTemperatures();
  temperature = thermometer.getTempCByIndex(0);
}

//// MODES

void modeClock(void* pvParameters) {
  for ever {
    timeClient.update();
    setNumber(0, timeClient.getHours());
    setNumber(1, timeClient.getMinutes());
    setNumber(2, timeClient.getSeconds());
    delay(100);
  }
}

void modeThermometer(void* pvParameters) {
  for ever {
    updateTemperature();
    bool isNegative = temperature < 0;
    LEDS[0][1] = LEDS[1][1] = isNegative ? HIGH : LOW;
    setNumber(1, abs(temperature));
    delay(2000);
  }
}

void modeErr(void* pvParameters) {
  for ever {
    LEDS[err][0] = !LEDS[err][0];
    LEDS[err][1] = !LEDS[err][0];
    setLeds();
    delay(250);
  }
}

void setLeds() {
  for(int col = 0; col < LEDS_COLS; col++){
    for(int row = 0; row < LEDS_ROWS; row++) {
      if(GPIO_DIGITS[col][row] == NO) continue;
      digitalWrite(GPIO_DIGITS[col][row], LEDS[col][row]);
    }
  }
}

void disableLeds() {
  for(int col = 0; col < LEDS_COLS; col++){
    for(int row = 0; row < LEDS_ROWS; row++) {
      if(GPIO_DIGITS[col][row] == NO) continue;
      digitalWrite(GPIO_DIGITS[col][row], LOW);
    }
  }
}

void (*getModePtr(void))(void* pvParameters) {
  switch(mode) {
    case THERMOMETER:
      return &modeThermometer;
    case ERR:
      return &modeErr;
    case CLOCK:
    default: 
      return &modeClock;
  }
}

void createMainThread() {
  if(mainThreadHandle) {
    vTaskDelete(mainThreadHandle);
    mainThreadHandle = NULL;
  }
  setAll(LOW);

  if(mode == BLANK) return;

  xTaskCreatePinnedToCore(getModePtr(), "mainThread", 10000, NULL, 2, &mainThreadHandle, 0);
}

void createMqttThread() {
  if(mqttThreadHandle) {
    vTaskDelete(mqttThreadHandle);
    mqttThreadHandle = NULL;
  }
   
  xTaskCreatePinnedToCore(mqttThread, "mqttThread", 10000, NULL, 1, &mqttThreadHandle, 0);
}

void mqttThread(void* pvParameters) {
  mqttClient.connect();
  for ever {
    updateTemperature();
    mqttClient.publish(MQTT_TOPIC, 1, true, String(temperature).c_str());
    delay(1000 * 60); // 1 min
  }
}

int getGmtOffset() {
  putenv("TZ=Europe/Warsaw");
  time_t rawtime = timeClient.getEpochTime();
  struct tm timeinfo;
  time(&rawtime);
  localtime_r(&rawtime, &timeinfo);
  int isdaylighttime = timeinfo.tm_isdst;
  return isdaylighttime ? 1 : 2;
}

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  if(!timeClient.isTimeSet()) {
    err = 4;
    setAll(LOW);
    while(!timeClient.isTimeSet()) {
      timeClient.forceUpdate();
      delay(100);
    }
  }
  timeClient.setTimeOffset(getGmtOffset() * 3600);
  mode = lastMode;
  createMainThread();
  createMqttThread();
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  err = 0;
  if(mode != ERR) lastMode = mode;
  if(mode != BLANK) {
    mode = ERR;
    createMainThread();
  }
  WiFi.reconnect();
}

void MQTTDisconnected(AsyncMqttClientDisconnectReason reason) {
  while(WiFi.isConnected()) {
    if(mqttClient.connected()) return;
    createMqttThread();
    delay(1000 * 30);
  }
}

//// MAIN

void setup() {
//  Serial.begin(115200); Serial.println("setup");
  
  for(int col = 0; col < LEDS_COLS; col++) {
    for(int row = 0; row < LEDS_ROWS; row++) {
      if(GPIO_DIGITS[col][row] == NO) continue;
      pinMode(GPIO_DIGITS[col][row], OUTPUT);
    }
  }
  setAll(LOW);

  button.setDebounceTime(DEBOUNCE_TIME);
  
  WiFi.setHostname("bcdClock");
  WiFi.onEvent(WiFiStationConnected, SYSTEM_EVENT_STA_CONNECTED);
  WiFi.onEvent(WiFiStationDisconnected, SYSTEM_EVENT_STA_DISCONNECTED);
  WiFi.begin(HOME_SSID, HOME_PASSWORD);

  timeClient.begin();
  
  thermometer.begin();
  thermometer.setResolution(9);

  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCredentials(MQTT_USER, MQTT_PASSWORD);
  mqttClient.onDisconnect(MQTTDisconnected);

  createMainThread();
}

void loop() {  
  //// BUTTON
  if(pressedTs > 0) {
    int pressedFor = millis() - pressedTs;
    if(pressedFor > 1000) {
      changeBrightness();
      pressedTs = millis() - 500;
    }
  }
  
  button.loop();
  if(button.isPressed())
    pressedTs = millis();
    
  if(button.isReleased()) {
    int pressedFor = millis() - pressedTs;
    if(pressedFor < 500) {
      changeMode();
    }
    pressedTs = 0;
  }
  
  //// LEDS
  setLeds();
  switch(brightness) {
    case B_FULL:
      break;
    case B_HIGH:
      delay(1);
      disableLeds();
      delay(4);
      break;
    case B_MEDIUM:
      ets_delay_us(20);
      disableLeds();
      break;
    case B_LOW:
      disableLeds();
      break;
  }
  delay(1);
}
