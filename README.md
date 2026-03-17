# Karta projektu: BCD Clock (ESP32)

## 1. Opis projektu
Projekt to zegar binarny BCD oparty o mikrokontroler ESP32. Wyświetlacz składa się z 20 diod LED ułożonych jako 6 cyfr BCD (godziny, minuty, sekundy). Urządzenie dodatkowo mierzy temperaturę czujnikiem DS18B20 i publikuje ją przez MQTT.

Aktualnie dostępne funkcje:
- wyświetlanie czasu HH:MM:SS w kodzie BCD,
- synchronizacja czasu przez NTP (Wi-Fi),
- tryb wyświetlania temperatury,
- tryb wygaszenia wyświetlacza,
- zmiana jasności LED przytrzymaniem przycisku,
- okresowa publikacja temperatury do brokera MQTT.

## 2. Tryby pracy i obsługa przycisku
W kodzie są 3 tryby pracy:
- `M_CLOCK` - wyświetlanie czasu,
- `M_THERMOMETER` - wyświetlanie temperatury,
- `M_BLANK` - wygaszony ekran.

Kolejność przełączania trybów:
`M_CLOCK` -> `M_THERMOMETER` -> `M_BLANK` -> `M_CLOCK`.

Obsługa przycisku (`GPIO35`):
- krótkie naciśnięcie (w praktyce: czas naciśnięcia < 400 ms) przełącza tryb,
- dłuższe przytrzymanie zmienia jasność cyklicznie,
- pierwszy skok jasności po 800 ms, kolejne co 400 ms podczas trzymania.

Debounce przycisku: 19 ms.

## 3. Start i logika programu
### Sekcja `setup`
Program podczas startu:
1. Konfiguruje przycisk (`ezButton`).
2. Ustawia NTP i strefę czasową (`configTime`, `setenv("TZ", ...)`, `tzset`).
3. Rejestruje zdarzenia Wi-Fi i uruchamia połączenie Wi-Fi.
4. Konfiguruje klienta MQTT (serwer, port, dane logowania).
5. Inicjalizuje czujnik DS18B20 i wykonuje pierwszy pomiar.
6. Tworzy zadanie w tle przypięte do rdzenia 0 (`xTaskCreatePinnedToCore`).

### Sekcja `loop`
Pętla główna:
- obsługuje przycisk,
- wywołuje funkcję wyświetlania zależnie od trybu,
- wykonuje opóźnienie 20 ms.

### Zadanie w tle `background_thread`
W pętli nieskończonej:
1. Aktualizacja temperatury,
2. opóźnienie 35 s,
3. publikacja temperatury MQTT (QoS 1, retained = true),
4. opóźnienie 35 s.

W praktyce publikacja następuje co około 70 sekund.

## 4. Wyświetlanie czasu i temperatury
### Tryb zegara (`display_clock`)
- Czas pobierany jest przez `getLocalTime`.
- Ekran aktualizuje się tylko gdy zmieni się sekunda (`tm_sec`).
- Gdy `getLocalTime` zwróci błąd: zapalany jest wskaźnik błędu (`LEDS[0][0] = HIGH`) oraz wskaźnik stanu Wi-Fi (`LEDS[0][1]` zależne od `WiFi.isConnected()`).

### Tryb termometru (`display_temperature`)
- Pomiary temperatury nie są wykonywane częściej niż co 5 sekund.
- Wyświetlana jest część całkowita wartości bezwzględnej temperatury.
- Znak minus sygnalizowany jest LED-ami `LEDS[0][1]` i `LEDS[1][1]`.
- Dla wartości spoza zakresu (warunek z kodu) ustawiana jest wartość awaryjna 77.

## 5. Jasność i sterowanie LED
Poziomy jasności (`Brightness`):
- `B_FULL = 4095`,
- `B_HIGH = 128`,
- `B_MEDIUM = 8`,
- `B_LOW = 2`.

Parametry PWM:
- częstotliwość: 10 kHz,
- rozdzielczość: 12 bit,
- kanały: `PWM_CHANNEL_LEDS_OFF = 0`, `PWM_CHANNEL_LEDS_ON = 1`.

Stan diod jest trzymany w tablicy `LEDS[6][4]`. Funkcja `apply_leds()` mapuje stan logiczny LED na kanał PWM ON/OFF dla odpowiedniego GPIO.

## 6. Pinout wyświetlacza LED (aktualny)
Układ logiczny cyfr:
- kolumny `0..5`: `H10`, `H1`, `M10`, `M1`, `S10`, `S1`,
- wiersze `0..3`: bity BCD `1`, `2`, `4`, `8`.

Mapowanie `GPIO_DIGITS[col][row]`:

| Kolumna (cyfra) | row0 (bit1) | row1 (bit2) | row2 (bit4) | row3 (bit8) |
|---|---:|---:|---:|---:|
| col0 (H10) | GPIO21 | GPIO17 | NO | NO |
| col1 (H1)  | GPIO4  | GPIO14 | GPIO16 | GPIO27 |
| col2 (M10) | GPIO19 | GPIO2  | GPIO13 | NO |
| col3 (M1)  | GPIO18 | GPIO22 | GPIO26 | GPIO15 |
| col4 (S10) | GPIO23 | GPIO12 | GPIO3  | NO |
| col5 (S1)  | GPIO5  | GPIO1  | GPIO25 | GPIO33 |

`NO` oznacza brak diody LED na danej pozycji siatki.

## 7. Pozostałe piny, komunikacja i biblioteki
Piny:
- DS18B20 (1-Wire): `GPIO32`,
- przycisk: `GPIO35`.

Konfiguracja sieci/czasu/MQTT z kodu:
- host Wi-Fi: `CHANGE_ME_WIFI_SSID`,
- hostname urządzenia: `bcdClock`,
- NTP: `pool.ntp.org`,
- strefa czasowa: `CET-1CEST,M3.5.0,M10.5.0/3`,
- broker MQTT: `192.168.0.50:1883`,
- temat MQTT: `bcdClock/temp`.

Użyte biblioteki:
- `ezButton`
- `OneWire`
- `DallasTemperature`
- `WiFi.h`
- `AsyncMqttClient`
- `time.h`
