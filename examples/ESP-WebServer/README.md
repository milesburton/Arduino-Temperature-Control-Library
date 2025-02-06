# 🌡️ Arduino Sketch: DS18B20 Sensor via Wi-Fi (REST Endpoints & Dashboard)

This **example Sketch** demonstrates how to use the [Arduino Temperature Control Library](https://github.com/milesburton/Arduino-Temperature-Control-Library) on an **ESP8266** or **ESP32** to read temperature data from **Maxim (Dallas) DS18B20** sensors. The Sketch publishes the readings via Wi-Fi in two ways:

1. **REST Endpoints** - Ideal for Node-RED, Home Assistant, or other automation platforms.
2. **A Human-Friendly Dashboard** - A simple web interface, powered by [Chart.js](https://www.chartjs.org/) and [Tailwind CSS](https://tailwindcss.com/), displaying current and historical temperatures.

---

## 🔎 Features
- Reads from one or more **DS18B20** temperature sensors
- Configurable **polling interval** (in milliseconds) and **history length** (number of readings)
- **Lightweight dashboard** that visualises the last N readings
- **REST endpoints** for easy integration:
  - `/temperature` - current readings
  - `/sensors` - sensor addresses
  - `/history` - historical data

---

## 📚 Potential Use Cases
- **Node-RED** automation flows: Perform regular HTTP GET requests against `/temperature` or `/history`
- **Home Assistant** integrations: Use built-in REST sensors to track temperature over time
- **Extensible to other sensor types** (humidity, light, pressure, etc.) by following the same approach

---

## 🛠️ Getting Started
1. **Clone or download** this repository
2. **Open the Sketch** (the `.ino` file) in the Arduino IDE (or other environment)
3. **Install dependencies**:
   - [Arduino Temperature Control Library](https://github.com/milesburton/Arduino-Temperature-Control-Library)
   - ESP8266 or ESP32 core for Arduino
4. **Set your Wi-Fi credentials** in the code:
```cpp
const char* ssid = "YourNetwork";
const char* password = "YourPassword";
```
5. **Adjust** the interval and history:
```cpp
// Configuration
const unsigned long READ_INTERVAL = 10000; // e.g. 10 seconds
const int HISTORY_LENGTH = 360; // 1 hour at 10-second intervals
```
6. **Connect** the DS18B20 sensor(s) to the ESP, using OneWire with a pull-up resistor
7. **Upload** the Sketch to your device
8. **Check** the serial monitor for the IP
9. **Navigate** to that IP in your browser to see the chart-based interface

---

## ❓ Questions & Support
- **Library Matters**: If you have issues with the **Arduino Temperature Control Library** itself, please open a ticket in the [official repository](https://github.com/milesburton/Arduino-Temperature-Control-Library/issues)
- **This Sketch**: For help customising this example, dealing with Wi-Fi issues, or setting up the chart dashboard, and other device-specific tweaks, please visit the [Arduino Forum](https://forum.arduino.cc/). You will find many friendly developers there ready to help.

---

## 📜 License
This project is distributed under the [MIT License](https://opensource.org/licenses/MIT).

---

We hope you find this Sketch useful for monitoring temperatures - both in a machine-readable (REST) and human-friendly (web dashboard) format. Happy hacking! 🚀
