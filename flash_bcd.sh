#!/usr/bin/env bash

echo "Wykrywanie portu..."
# Próba znalezienia pliku /dev/ttyUSB0 lub /dev/ttyACM0
PORT=$(find /dev -name "ttyUSB*" -o -name "ttyACM*" | head -n 1)

if [ -z "$PORT" ]; then
    echo "Nie znaleziono portu USB z ESP32 (ttyUSB0, ttyACM0 itp.)."
    echo "Upewnij się, że urządzenie jest podłączone."
    echo "Jeśli Twój port to np. /dev/ttyS0, użyj go jawnie w wywołaniu arduino-cli."
    exit 1
fi

echo "Zatwierdzony port: $PORT"
echo "Rozpoczynam kompilację i wgrywanie przy użyciu arduino-cli..."

nix-shell -p arduino-cli --run \
  "arduino-cli compile --fqbn esp32:esp32:esp32:PartitionScheme=huge_app --libraries /home/tsd/1/code/arduino/libraries -u -p $PORT bcd_clock.ino"

if [ $? -eq 0 ]; then
    echo ""
    echo "=== Operacja pomyślnie zakończona! Wgrano Matter BCD ==="
    echo "Zerknij na ekran urządzenia."
    echo "Możesz odpalić port na podgląd komendą 'picocom -b 115200 $PORT'"
else
    echo "Błąd podczas wgrywania."
fi
