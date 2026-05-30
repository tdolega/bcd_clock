# BCD Clock (ESP32 + Matter)

A standalone BCD (Binary-Coded Decimal) clock and environmental thermometer based on the ESP32 microcontroller. The firmware drives a 20-LED matrix directly via hardware PWM and integrates with local Smart Home ecosystems exclusively through the IPv6-based Matter protocol.

## Runtime Architecture

The firmware is designed around a single-thread, non-blocking event loop pattern. To ensure long-term stability and prevent core race conditions, it strictly avoids:
- FreeRTOS application-level task branching.
- Blocking Wi-Fi or Matter reconnect loops.
- Blocking NTP time queries in the UI rendering path.

The hardware sensor (DS18B20) is managed by a state machine that handles asynchronous conversions, ensuring the main loop runs consistently with a 2ms delay.

## Matter Integration

The device operates locally without cloud dependencies, exposing two endpoints:
1. **Dimmable Light:** Controls the global PWM duty cycle of the LED matrix.
2. **Temperature Sensor:** Reports ambient temperature.

**Network behavior:** Temperature reports are throttled. A report is pushed only if the delta exceeds 0.1°C and at least 5 seconds have passed since the last report. A mandatory heartbeat report is dispatched every 60 seconds to prevent controller timeout.
**Static Pairing Code:** `34970112332`

## Time Synchronization & RTC Accuracy

Timekeeping relies on the internal ESP32 RTC, synchronized via SNTP.

- **NTP Sync:** The device queries `pool.ntp.org` on boot and subsequently every 6 hours.
- **Timezone:** Hardcoded to CET/CEST via POSIX environment variable (`CET-1CEST,M3.5.0,M10.5.0/3`).
- **RTC Drift:** If Wi-Fi is lost, the clock falls back to the local ESP32 oscillator. Due to hardware tolerances (~20 ppm), the clock will drift by approximately 1.7 seconds per day without network access. Reconnection triggers an immediate step adjustment.

## Physical Interface (GPIO35)

A single hardware button with 50ms software debounce handles local control:
- **Short press:** Toggles display mode (`Clock` ↔ `Thermometer`).
- **Long press (> 800ms):** Cycles hardware brightness levels.
- **15 rapid clicks:** Executes Matter decommission, factory reset, and reboots the ESP32.

## Boot Sequence & Diagnostics

The firmware implements a strict fail-safe visualization. Normal BCD rendering is suspended if critical operational requirements are not met. Diagnostic codes are displayed on the first two LED columns (H10, H1).

### Boot Timeline
| Time | Subsystem State | LED Matrix Output |
| :--- | :--- | :--- |
| **0 - 800 ms** | Enforcing synchronous DS18B20 read to satisfy initial Matter cluster constraints. | All LEDs OFF. If sensor fails to respond, bottom-left LED blinks every 250ms (Heartbeat lock). |
| **800 ms - 5 s** | Wi-Fi connection attempt. Matter stack initialization. | First two columns display `DIAG_CODE_NTP` and `DIAG_CODE_WIFI` alternately. |
| **5 s+ (Online)** | Wi-Fi connected, NTP synchronized. | Normal operation. BCD Clock starts rendering. |

### Hard Diagnostic Codes
If an error occurs during runtime, the screen goes blank and displays one of the following codes. Multiple errors are rotated every 1000ms.

| Signal (Cols 0 & 1) | Error Code | Trigger Condition |
| :--- | :--- | :--- |
| **Row 0 (BCD 1)** | `DIAG_CODE_WIFI` | Complete loss of Wi-Fi connection. |
| **Row 1 (BCD 2)** | `DIAG_CODE_NTP` | Valid epoch not reached, or 12 hours since last successful sync. |
| **Row 2 (BCD 4)** | `DIAG_CODE_TEMP` | Sensor disconnected or returning invalid values for 3 consecutive cycles (15s). |

*Note: A temporary sensor read failure (e.g., bus noise) during Thermometer mode renders the decimal value `77`. If the error persists beyond 15 seconds, the device escalates to `DIAG_CODE_TEMP`.*

## Hardware & Pinout

### Single-Channel PWM Architecture
To avoid exhausting hardware timers, the firmware utilizes a single 10 kHz, 12-bit PWM channel. The application logic dynamically attaches (`ledcAttachChannel`) and detaches pins from this channel based on the required BCD grid state, cached to prevent unnecessary hardware calls.

### LED Matrix Mapping (`GPIO_DIGITS`)
Columns are mapped Left to Right (Hours, Minutes, Seconds).
Rows are mapped Bottom to Top (BCD weights 1, 2, 4, 8).

| Digit Column | Weight 1 (row0) | Weight 2 (row1) | Weight 4 (row2) | Weight 8 (row3) |
| :--- | :--- | :--- | :--- | :--- |
| **H10** | GPIO21 | GPIO17 | NO | NO |
| **H1** | GPIO4 | GPIO14 | GPIO16 | GPIO27 |
| **M10** | GPIO19 | GPIO2 | GPIO13 | NO |
| **M1** | GPIO18 | GPIO22 | GPIO26 | GPIO15 |
| **S10** | GPIO23 | GPIO12 | GPIO3 | NO |
| **S1** | GPIO5 | GPIO1 | GPIO25 | GPIO33 |

**UART WARNING:** Pins `GPIO1` (TX) and `GPIO3` (RX) are physically wired to the LED matrix. The standard Serial interface is explicitly terminated (`Serial.end()`) to prevent the Matter core's internal logging routines from causing severe matrix flickering. Hardware debugging must be done via the diagnostic LED codes.

## Development Environment

The project relies on a Nix flake environment for deterministic compilation.

**Build Requirements:**
Create `src/config/wifi_secrets_local.h` before compiling:
```cpp
#pragma once
inline constexpr const char WIFI_SSID[] = "SSID";
inline constexpr const char WIFI_PASSWORD[] = "PASS";

```

**Tooling:**

* `./flash_bcd.sh` - Compiles and flashes via `arduino-cli` utilizing the `rainmaker` partition scheme (disables debug core). Port detection is automatic.
* `./erase_esp.sh` - Wipes ESP32 flash memory, required for resolving stale Matter commissioning states.
* `./dump_code.sh` / `./load_code.sh` - Repository state management scripts.
