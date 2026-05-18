#!/usr/bin/env bash

echo "Detecting port..."
# Try to find /dev/ttyUSB0 or /dev/ttyACM0
PORT=$(find /dev -name "ttyUSB*" -o -name "ttyACM*" | head -n 1)

if [ -z "$PORT" ]; then
    echo "USB port with ESP32 not found (ttyUSB0, ttyACM0 etc.)."
    echo "Make sure the device is connected."
    echo "If your port is e.g. /dev/ttyS0, use it explicitly in the arduino-cli call."
    exit 1
fi

echo "Confirmed port: $PORT"
echo "Starting compilation and flashing using arduino-cli..."

nix-shell -p arduino-cli --run \
  "arduino-cli compile --fqbn esp32:esp32:esp32:PartitionScheme=huge_app --libraries /home/tsd/1/code/arduino/libraries -u -p $PORT bcd_clock.ino"

if [ $? -eq 0 ]; then
    echo ""
    echo "=== Operation finished successfully! Matter BCD flashed ==="
    echo "Look at the device screen."
    echo "You can open the port monitor with the command 'picocom -b 115200 $PORT'"
else
    echo "Error during flashing."
fi
