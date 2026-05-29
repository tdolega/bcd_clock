# BCD Clock (ESP32 + Matter)

## Opis
Zegar BCD oparty o ESP32 z dwoma trybami: zegar oraz termometr (DS18B20). Urządzenie wystawia w Matter dwa endpointy:

- **Dimmable Light** – regulacja jasności wyświetlacza,
- **Temperature Sensor** – raport temperatury.

## Pairing code
Stały kod parowania: **34970112332**.

## Tryby pracy
- `M_CLOCK` – zegar (`HH:MM:SS`),
- `M_THERMOMETER` – temperatura w stopniach Celsjusza.

## Sterowanie przyciskiem (GPIO35)
- **Krótki klik** – przełącza zegar/termometr.
- **Długie przytrzymanie** – zmienia jasność cyklicznie: `2 → 8 → 128 → 4095`.
- **15 kliknięć** w krótkim czasie – przywraca ustawienia fabryczne (Matter decommission).

## Jasność LED
PWM:
- częstotliwość `10 kHz`,
- rozdzielczość `12 bit`,
- kanał PWM: `PWM_CHANNEL_LEDS_ON = 1`.

## Termometr (DS18B20)
- GPIO: `GPIO32`,
- odczyt nieblokujący (`setWaitForConversion(false)`),
- cykl odczytu co `5000 ms`,
- czas konwersji `800 ms`.

## Wi-Fi i sekretne dane
Dane Wi-Fi są w pliku lokalnym:

- `src/config/wifi_secrets_local.h`

Plik jest ignorowany przez Git i nie trafia do repozytorium.

## Pinout LED (`GPIO_DIGITS[col][row]`)
Kolumny: `H10 H1 M10 M1 S10 S1`
Wiersze: bity BCD `1 2 4 8`

| Kolumna (cyfra) | row0 (bit1) | row1 (bit2) | row2 (bit4) | row3 (bit8) |
|---|---:|---:|---:|---:|
| col0 (H10) | GPIO21 | GPIO17 | NO | NO |
| col1 (H1)  | GPIO4  | GPIO14 | GPIO16 | GPIO27 |
| col2 (M10) | GPIO19 | GPIO2  | GPIO13 | NO |
| col3 (M1)  | GPIO18 | GPIO22 | GPIO26 | GPIO15 |
| col4 (S10) | GPIO23 | GPIO12 | GPIO3  | NO |
| col5 (S1)  | GPIO5  | GPIO1  | GPIO25 | GPIO33 |

`NO` oznacza brak diody na danej pozycji siatki.

## Biblioteki
- `ezButton`
- `OneWire`
- `DallasTemperature`
- `WiFi.h`
- `Matter`
- `time.h`
