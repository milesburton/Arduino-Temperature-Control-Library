
# 🌡️ Arduino Temperature Control Library

[![Arduino CI](https://github.com/milesburton/Arduino-Temperature-Control-Library/workflows/Arduino%20CI/badge.svg)](https://github.com/marketplace/actions/arduino_ci)
[![Arduino-lint](https://github.com/milesburton/Arduino-Temperature-Control-Library/actions/workflows/arduino-lint.yml/badge.svg)](https://github.com/milesburton/Arduino-Temperature-Control-Library/actions/workflows/arduino-lint.yml)
[![JSON check](https://github.com/milesburton/Arduino-Temperature-Control-Library/actions/workflows/jsoncheck.yml/badge.svg)](https://github.com/milesburton/Arduino-Temperature-Control-Library/actions/workflows/jsoncheck.yml)
[![GitHub issues](https://img.shields.io/github/issues/milesburton/Arduino-Temperature-Control-Library.svg)](https://github.com/milesburton/Arduino-Temperature-Control-Library/issues)

[![License: MIT](https://img.shields.io/badge/license-MIT-green.svg)](https://github.com/milesburton/Arduino-Temperature-Control-Library/blob/master/LICENSE)
[![GitHub release](https://img.shields.io/github/release/milesburton/Arduino-Temperature-Control-Library.svg?maxAge=3600)](https://github.com/milesburton/Arduino-Temperature-Control-Library/releases)
[![Commits since latest](https://img.shields.io/github/commits-since/milesburton/Arduino-Temperature-Control-Library/latest)](https://github.com/milesburton/Arduino-Temperature-Control-Library/commits/master)

A robust and feature-complete Arduino library for Maxim Temperature Integrated Circuits.


## 📌 Supported Devices

- DS18B20
- DS18S20 (⚠️ Known issues with this series)
- DS1822
- DS1820
- MAX31820
- MAX31850

## 🚀 Installation

### Using Arduino IDE Library Manager (Recommended)
1. Open Arduino IDE
2. Go to Tools > Manage Libraries...
3. Search for "DallasTemperature"
4. Click Install
5. Also install the required "OneWire" library by Paul Stoffregen using the same method

### Manual Installation
1. Download the latest release from [GitHub releases](https://github.com/milesburton/Arduino-Temperature-Control-Library/releases)
2. In Arduino IDE, go to Sketch > Include Library > Add .ZIP Library...
3. Select the downloaded ZIP file
4. Repeat steps 1-3 for the required "OneWire" library

## 📝 Basic Usage

1. **Hardware Setup**
   - Connect a 4k7 Ω pull-up resistor between the 1-Wire data line and 5V power. Note this applies to the Arduino platform, for ESP32 and 8266 you'll need to adjust the resistor value accordingly.
   - For DS18B20: Ground pins 1 and 3 (the centre pin is the data line)
   - For reliable readings, see pull-up requirements in the [DS18B20 datasheet](https://datasheets.maximintegrated.com/en/ds/DS18B20.pdf) (page 7)

2. **Code Example**
   ```cpp
   #include <OneWire.h>
   #include <DallasTemperature.h>

   // Data wire is connected to GPIO 4
   #define ONE_WIRE_BUS 4

   OneWire oneWire(ONE_WIRE_BUS);
   DallasTemperature sensors(&oneWire);

   void setup(void) {
     Serial.begin(9600);
     sensors.begin();
   }

   void loop(void) { 
      sensors.requestTemperatures(); 
      
      delay(750); 
      
      float tempC = sensors.getTempCByIndex(0);
      Serial.print("Temperature: ");
      Serial.print(tempC);
      Serial.println("°C");
      delay(1000);
   }
   ```

## 🛠️ Advanced Features

- Multiple sensors on the same bus
- Temperature conversion by address (`getTempC(address)` and `getTempF(address)`)
- Asynchronous mode (added in v3.7.0)
- Configurable resolution

### Configuration Options

You can slim down the code by defining the following at the top of DallasTemperature.h:

```cpp
#define REQUIRESNEW      // Use if you want to minimise code size
#define REQUIRESALARMS   // Use if you need alarm functionality
```

## 📚 Additional Documentation

Visit our [Wiki](https://www.milesburton.com/w/index.php/Dallas_Temperature_Control_Library) for detailed documentation.

## 🔧 Library Development

If you want to contribute to the library development:

### Using Dev Container
The project includes a development container configuration for VS Code that provides a consistent development environment.

1. **Prerequisites**
   - Visual Studio Code
   - Docker
   - VS Code Remote - Containers extension

2. **Development Commands**
   Within the dev container, use:
   - `arduino-build` - Compile the library and examples
   - `arduino-test` - Run the test suite
   - `arduino-build-test` - Complete build and test process

   > Note: Currently compiling against arduino:avr:uno environment

## ✨ Credits

- Original development by Miles Burton <mail@milesburton.com>
- Multiple sensor support by Tim Newsome <nuisance@casualhacker.net>
- Address-based temperature reading by Guil Barros gfbarros@bappos.com
- Async mode by Rob Tillaart rob.tillaart@gmail.com

## 📄 License

MIT License | Copyright (c) 2025 Miles Burton

Full license text available in [LICENSE](LICENSE) file.
