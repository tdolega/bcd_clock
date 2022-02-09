#include <ezButton.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <AsyncMqttClient.h>

#define ever (;;)

/*
 * MESSAGES:
 * 2 left flashing: connecting to WiFi
 * all constant: trying to read time from the internet
 */

const char* HOME_SSID     = "CHANGE_ME_WIFI_SSID";
const char* HOME_PASSWORD = "CHANGE_ME_WIFI_PASSWORD";

const auto MQTT_HOST = IPAddress(0, 0, 0, 0);
const int MQTT_PORT = 1883;
const char* MQTT_USER     = "CHANGE_ME_MQTT_USER";
const char* MQTT_PASSWORD = "CHANGE_ME_MQTT_PASSWORD";
const char* MQTT_TOPIC = "zegarekBinarny/temperature";
AsyncMqttClient mqttClient;

const int GMT = +1; // Polska
const int GMT_OFFSET = GMT * 3600; 

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

const int GPIO_THERMOMETER = 32;
OneWire oneWireThermometer(GPIO_THERMOMETER);
DallasTemperature thermometer(&oneWireThermometer);

const int GPIO_BUTTON = 35;
ezButton button(GPIO_BUTTON);
const int DEBOUNCE_TIME = 50; // [ms]
int pressedTs = 0; // -1: brightness was changed, 0: button is not pressed, >0: button is pressed for x amount of time

typedef enum Modes {
  CLOCK,
  THERMOMETER,
  BLANK,
} Modes;
Modes mode = CLOCK;
void changeMode() {
    switch(mode) {
      case CLOCK:        mode = THERMOMETER;   break;
      case THERMOMETER:  mode = BLANK;         break;
      case BLANK:        mode = CLOCK;         break;
    }
}

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

TaskHandle_t mainThreadHandle;
TaskHandle_t mqttThreadHandle;

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

void modeClock() {
  timeClient.update();
  setNumber(0, timeClient.getHours());
  setNumber(1, timeClient.getMinutes());
  setNumber(2, timeClient.getSeconds());
}

void modeThermometer() {
  updateTemperature();
  bool isNegative = temperature < 0;
  LEDS[0][1] = LEDS[1][1] = isNegative ? HIGH : LOW;
  setNumber(1, abs(temperature));
}


void modeDisabled() {
  setAll(LOW);
  delay(1000 * 60);
}

//// REAL TIME THREAD

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

void mainThread(void* pvParameters) {
  for ever {
    switch(mode) {
      case CLOCK:
        modeClock();
        delay(1000);
        break;
      case THERMOMETER:
        modeThermometer();
        delay(2000);
        break;
      case BLANK:
        modeDisabled();
        break;
    }
    delay(1);
  }
}
void createMainThread() {
  xTaskCreatePinnedToCore(mainThread, "mainThread", 10000, NULL, 2, &mainThreadHandle, 0);
}

void mqttThread(void* pvParameters) {
  for ever {
    updateTemperature();
    mqttClient.publish(MQTT_TOPIC, 1, true, String(temperature).c_str());
    delay(1000 * 60);
  }
}

//// MAIN

void setup() {
//  Serial.begin(115200);
  
  for(int col = 0; col < LEDS_COLS; col++) {
    for(int row = 0; row < LEDS_ROWS; row++) {
      if(GPIO_DIGITS[col][row] == NO) continue;
      pinMode(GPIO_DIGITS[col][row], OUTPUT);
    }
  }
  setAll(LOW);

  button.setDebounceTime(DEBOUNCE_TIME);

  thermometer.begin();
  thermometer.setResolution(9);

  WiFi.begin(HOME_SSID, HOME_PASSWORD);
  while(WiFi.status() != WL_CONNECTED) {
    LEDS[0][0] = !LEDS[0][0];
    LEDS[0][1] = !LEDS[0][0];
    setLeds();
    delay(250);
  }
  
  timeClient.begin();
  timeClient.setTimeOffset(GMT_OFFSET);

  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCredentials(MQTT_USER, MQTT_PASSWORD);
  mqttClient.connect();

  createMainThread();
  xTaskCreatePinnedToCore(mqttThread, "mqttThread", 10000, NULL, 1, &mqttThreadHandle, 0);
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
      
      if(mainThreadHandle)
        vTaskDelete(mainThreadHandle);
      setAll(LOW);
      createMainThread();
    }
    pressedTs = 0;
  }
  
  //// LEDS
  setLeds();
  switch(brightness) {
    case B_FULL:
      delay(1);
      break;
    case B_HIGH:
      delay(1);
      disableLeds();
      delay(5);
      break;
    case B_MEDIUM:
      ets_delay_us(20);
      disableLeds();
      delay(1);
      break;
    case B_LOW:
      disableLeds();
//      delay(1);
      ets_delay_us(1 * 1000);
      break;
  }
}
