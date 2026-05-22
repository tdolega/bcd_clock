#!/usr/bin/env bash

# arduino-cli monitor -p /dev/ttyACM0 --config baudrate=115200
nix-shell -p arduino-cli --run "arduino-cli monitor -p /dev/ttyACM0 --config baudrate=115200"
