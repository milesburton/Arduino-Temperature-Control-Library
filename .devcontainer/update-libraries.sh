#!/bin/bash

echo "Updating arduino-cli core and index..."
arduino-cli core update-index
arduino-cli update

echo "Updating installed libraries..."
arduino-cli lib update-index
arduino-cli lib upgrade

# Update Arduino cores
echo "Updating ESP8266 and ESP32 cores..."
arduino-cli core install esp8266:esp8266
arduino-cli core install esp32:esp32

# List of libraries to ensure are installed/updated
LIBRARIES=(
    "OneWire"
    "ArduinoUnit"
)

echo "Checking and installing libraries..."
for lib in "${LIBRARIES[@]}"; do
    echo "Processing library: $lib"
    if ! arduino-cli lib list | grep -q "$lib"; then
        echo "Installing $lib..."
        arduino-cli lib install "$lib"
    else
        echo "$lib is already installed"
    fi
done

echo "Verifying all libraries are up to date..."
arduino-cli lib list

echo "Library update complete!"