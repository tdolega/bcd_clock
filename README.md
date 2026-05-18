# BCD Clock (ESP32)

## Opis
Projekt to zegar binarny BCD oparty o ESP32. Wyświetlacz składa się z 20 diod LED i pokazuje czas jako 6 cyfr BCD (`HH:MM:SS`). Urządzenie ma tryb termometru (DS18B20), sterowanie jednym przyciskiem oraz pełne sterowanie i raportowanie stanu przez MQTT.

## Konfiguracja
Firmware używa dwóch plików konfiguracyjnych:
- [runtime_config.h](runtime_config.h) - bezpieczne domyślne wartości i mapowanie makr na stałe.
- [runtime_config_local.h](runtime_config_local.h) - lokalne sekrety i nadpisania (ignorowany przez git).

Najważniejsze parametry konfiguracyjne:
- Wi-Fi (`CFG_WIFI_SSID`, `CFG_WIFI_PASSWORD`, `CFG_HOSTNAME`)
- MQTT broker i dane logowania (`CFG_MQTT_HOST_*`, `CFG_MQTT_PORT`, `CFG_MQTT_USER`, `CFG_MQTT_PASSWORD`)
- NTP i strefa czasowa (`CFG_NTP_SERVER`, `CFG_TIMEZONE_ENV`)
- topiki MQTT (`CFG_MQTT_TOPIC_*`)
- timingi retry/restore/publikacji i DS18B20 (`CFG_*_INTERVAL_MS`, `CFG_MQTT_RESTORE_WINDOW_MS`)

## Model runtime
Firmware działa jako pojedyncza pętla nieblokująca (single-thread event loop):
- brak osobnych tasków FreeRTOS dla logiki aplikacyjnej,
- brak blokujących pętli reconnect,
- brak blokujących odczytów czasu NTP w ścieżce UI,
- jeden właściciel obsługi czujnika DS18B20.

To eliminuje wyścigi między rdzeniami i poprawia stabilność przy długim uptime.

## Tryby pracy
Tryby (`Modes`):
- `M_CLOCK` - zegar,
- `M_THERMOMETER` - temperatura,
- `M_PINGTEST` - test opóźnienia ICMP do `8.8.8.8`,
- `M_BLANK` - wygaszony ekran.

Kolejność lokalnego przełączania: `clock -> thermometer -> pingtest -> blank -> clock`.

## Sterowanie lokalne (przycisk)
Przycisk na `GPIO35`:
- krótkie naciśnięcie (`< 400 ms`) zmienia tryb,
- długie przytrzymanie zmienia jasność cyklicznie,
- pierwszy skok jasności po `800 ms`, kolejne co `400 ms`.

Debounce: `50 ms`.

## Jasność LED
Poziomy jasności (`Brightness`):
- `full` = `4095`,
- `high` = `128`,
- `medium` = `8`,
- `low` = `2`.

PWM:
- częstotliwość `10 kHz`,
- rozdzielczość `12 bit`,
- kanał PWM: `PWM_CHANNEL_LEDS_ON = 1`.

Aktualizacja LED jest zoptymalizowana: przełączane są tylko te GPIO, których kanał ON/OFF naprawdę się zmienił.

Pętla główna:
- Opóźnienie pętli: `2 ms`.

## MQTT - sterowanie i stan
Urządzenie wspiera dwa typy topiców:

1. Komendy (`cmd`) - sterowanie zegarem.
2. Stan (`state`) - aktualny stan publikowany retained.

### Topic: komendy
- `bcdClock/cmd/mode`
  payload: `clock`, `thermometer`, `pingtest`, `ping`, `blank`, `temp`, `off`, `next`
- `bcdClock/cmd/brightness`
  payload: `full`, `high`, `medium`, `low`, `4095`, `128`, `8`, `2`, `next`
- `bcdClock/cmd/state`
  dowolny payload wyzwala natychmiastową publikację pełnego stanu

### Topic: stan
- `bcdClock/state/online` (`1` online, `0` offline, retained + Last Will)
- `bcdClock/state/mode` (`clock|thermometer|pingtest|blank`, retained)
- `bcdClock/state/brightness` (`full|high|medium|low`, retained)
- `bcdClock/state/temperature` (np. `23.75` lub `nan`, retained)
- `bcdClock/state/json` (JSON ze stanem, retained)

Przykład `state/json`:
```json
{"mode":"clock","brightness":"medium","temperature_c":23.75}
```

## Wskaźniki błędów i stanu na LED

Kiedy urządzenie czeka na zegarek systemowy (brak synchronizacji NTP), wyświetla diagnostykę zamiast czasu:

| GPIO (pozycja) | Znaczenie | Stan |
|---|---|---|
| GPIO21 (H10, bit1) | Brak ważnego czasu z NTP | ON gdy czas nie zsynchronizowany, OFF po sync |
| GPIO17 (H10, bit2) | Status WiFi | ON gdy WiFi połączony, OFF gdy brak |
| GPIO4 (H1, bit1) | Status MQTT | ON gdy MQTT połączony, OFF gdy brak |

Przykład diagnostyki po starcie (zanim NTP się zsynchronizuje):
- GPIO21 świeci (czeka na czas)
- GPIO17 świeci lub nie (zależy od WiFi)
- GPIO4 świeci lub nie (zależy od MQTT)

### Błędy termometru

Jeśli czujnik DS18B20 zwraca błąd (np. rozłączony lub uszkodzony):
- Na wyświetlaczu termometru: `77` (kod błędu `ERROR_DISPLAY_VALUE`)
- Temperatura w MQTT: `nan` lub brak wartości
- Status LED: bez zmian (bity diagnostyczne nie reagują na błąd czujnika)

## Odtwarzanie stanu po restarcie
Po połączeniu z MQTT urządzenie:
1. Subskrybuje `state/mode` i `state/brightness`.
2. Akceptuje te topiki tylko jako retained i tylko w oknie restore.
3. Przywraca tryb i jasność z retained.
4. Po `MQTT_RESTORE_WINDOW_MS` wypisuje się z topików restore.
5. Publikuje pełny bieżący stan (`mode`, `brightness`, `temperature`, `json`).

To chroni przed przypadkowym traktowaniem topików `state/*` jako kanału sterującego po starcie.

## Łączność i odporność
### Wi-Fi
- brak blokujących pętli,
- retry co `CFG_WIFI_RETRY_INTERVAL_MS`,
- logika oparta o eventy `GOT_IP` i `DISCONNECTED`.

### MQTT
- brak blokujących reconnectów,
- retry co `CFG_MQTT_RETRY_INTERVAL_MS`,
- Last Will na `state/online`,
- normalizacja payloadu bez użycia `String`.

### NTP
- konfiguracja przez `configTime` + `TZ`,
- odczyt czasu przez `time()`/`localtime_r()` (bez blokowania UI),
- wykrywanie zsynchronizowanego czasu przez próg epoch (`CFG_NTP_VALID_EPOCH_THRESHOLD`).

### DS18B20 (termometr)
- odczyt nieblokujący (`setWaitForConversion(false)`),
- cykl request: co `5000 ms` (5 sekund),
- czekanie na konwersję: `800 ms`,
- publikacja temperatury do MQTT: co `70000 ms` (70 sekund).

### Ping test
- cel ICMP: `8.8.8.8`,
- interwał pomiaru: co `10000 ms` (10 sekund),
- timeout pojedynczego ping: `1000 ms`,
- wyświetlanie wartości: środkowy i prawy panel (`col2..col5`) jako liczba `0000..9999` ms,
- wskaźnik jakości: lewy panel, druga kolumna (`col1`, od dołu):
  - `< 25 ms` -> 1 LED,
  - `< 50 ms` -> 2 LED,
  - `< 100 ms` -> 3 LED,
  - `>= 100 ms` -> 4 LED,
- brak odpowiedzi (np. brak internetu): wyświetlacz trybu pingtest pozostaje wygaszony.

## Stan systemu po restarcie

Po załadowaniu się firmwaru urządzenie przechodzi przez następujące etapy (typowo 5–30 sekund):

| Czas | WiFi | MQTT | NTP | LED status | Co się wyświetla |
|------|------|------|-----|---|---|
| 0–500 ms | nieinicjalizowane | nieinicjalizowane | query wysłany | wszystkie OFF | (nic) |
| 1–2 s | ✓ połączony | czeka | czeka | GPIO17 ON, GPIO21 ON | diagnostyka: brak czasu |
| 3–5 s | ✓ | ✓ połączony | czeka | GPIO17 ON, GPIO4 ON, GPIO21 ON | diagnostyka: WiFi + MQTT, czeka czas |
| 5–30 s | ✓ | ✓ | ✓ **zsynchronizowany** | GPIO17 ON, GPIO4 ON, GPIO21 OFF | **zegar HH:MM:SS począwszy** |

**Po restarcie czasem widać świecący lewy dolny LED (GPIO21)** — to normalnie i oznacza, że urządzenie czeka na synchronizację czasu z serwera NTP. Jest to wskaźnik zdrowotnościowy, a nie błąd.

## Aktualizacja i dokładność czasu NTP

### Synchronizacja NTP
- Na starcie urządzenie wysyła query do `pool.ntp.org` (parametr `CFG_NTP_SERVER`)
- Weryfikacja synchronizacji: czas musi przekroczyć próg epoch `1700000000` (Nov 9, 2023 01:33:20 UTC)
- Bez dostępu do WiFi: query nigdy się nie powiedzie; zegar nie będzie działać
- ESP32 SDK obsługuje wewnętrznie regularne odświeżanie czasu (typowo co 60–120 minut)

### Dokładność czasu
- **Na bieżąco (po NTP sync):** dokładność zależy od latencji sieci i ESP32 RTC (typowo <1 ms)
- **Lokalny drift RTC:** bez WiFi, zegar dryfuje z dokładnością wewnętrznego oscylatora ESP32 (~±20 ppm)
  - Przesunięcie teoretyczne: ~1.7 sekundy na dobę
  - Oscylacja zależy od temperatury otoczenia
- **Brak kompensacji driftu w kodzie:** firmware nie zawiera kalibracji ani slew rate adjustment
- **Stabilność po reconneccie WiFi:** Po ponownym połączeniu i NTP sync, zegar aktualizuje się skokowo (nie gradualnie)

### Praktyczne implikacje
- Zegar jest precyzyjny krótkookresowo (minuty–godziny) dzięki NTP
- Długoterminowa dokładność (>24h bez WiFi) może się pogorszyć o kilka sekund
- Jeśli krytyczna jest wysoka precyzja, zaleca się WiFi zawsze dostępny i regularne resynchronizacje

## Zachowanie w różnych scenariuszach

### Brak WiFi na starcie
- Urządzenie wysyła query NTP, ale bez WiFi nigdy się nie zsynchronizuje
- Wyświetla diagnostykę: GPIO21 (brak czasu) i GPIO17 (brak WiFi)
- Zegar nie będzie działać; wyświetlacz zostaje disabled
- Po podłączeniu WiFi: automatycznie próbuje MQTT co 5 sekund, a NTP synchronizuje się w ciągu kilkunastu sekund

### Strata WiFi podczas pracy
- Zegar **zamiast czasu** natychmiast przełącza na tryb diagnostyki (GPIO17 OFF)
- MQTT odłącza się i urządzenie czeka na WiFi reconnect (`WiFi.setAutoReconnect(true)`)
- Po powrocie WiFi: MQTT reconnectuje się, a czas jest utrzymywany lokalnie (nie potrzeba ponownej synchronizacji)

### WiFi aktywny, ale MQTT broker niedostępny
- Zegar **pracuje normalnie** (czas jest utrzymywany lokalnie)
- MQTT reconnectuje się co 5 sekund, aż do powodzenia
- Publikacja stanu wznawia się automatycznie po reconneccie

### Tryb pingtest bez internetu
- Brak odpowiedzi ICMP powoduje brak wyświetlania wyniku (`brak LED`),
- Kolejny test wykonywany jest co 10 sekund,
- Po odzyskaniu internetu wynik pojawia się automatycznie przy kolejnym cyklu ping.

### Restart urządzenia
- Wszystkie LED gasną na 500 ms (inicjalizacja)
- W kolejnym 1–3 sekundach: pojawia się diagnostyka (GPIO21, GPIO17, czasem GPIO4)
- W 5–30 sekund (typowo 10–15 s): NTP synchronizuje się i zegar rozpoczyna pracę

**Wskazówka:** Długi czas synchronizacji NTP (>30 s) może oznaczać problem z WiFi (szybkość poboru, błędy DNS) lub konfiguracyjny NTP server

## Logika pętli
### `setup`
- inicjalizacja map LED i cache kanałów,
- konfiguracja przycisku,
- konfiguracja czasu (query NTP natychmiast),
- start Wi-Fi (asynchroniczny),
- konfiguracja klienta MQTT,
- inicjalizacja DS18B20 (nieblokujący odczyt).

### `loop`
- obsługa przycisku,
- nieblokujące utrzymanie Wi-Fi,
- nieblokujące utrzymanie MQTT (retry co 5 s jeśli WiFi OK),
- obsługa okna restore MQTT (pierwsze 1500 ms po MQTT connect),
- cykl odczytu DS18B20 (request co 5 s, wait 800 ms na konwersję),
- publikacja okresowa temperatury (co 70 s),
- cykl pingtest (ICMP do 8.8.8.8 co 10 s),
- render wybranego trybu (clock/thermometer/pingtest/blank),
- opóźnienie pętli: `2 ms`.

## Pinout
### Dodatkowe piny
- DS18B20 (1-Wire): `GPIO32`
- przycisk: `GPIO35`

### Mapa LED (`GPIO_DIGITS[col][row]`)
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
- `AsyncMqttClient`
- `time.h`
