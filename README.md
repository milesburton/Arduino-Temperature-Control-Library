[![Arduino CI](https://github.com/milesburton/Arduino-Temperature-Control-Library/workflows/Arduino%20CI/badge.svg)](https://github.com/marketplace/actions/arduino_ci)
[![Arduino-lint](https://github.com/milesburton/Arduino-Temperature-Control-Library/actions/workflows/arduino-lint.yml/badge.svg)](https://github.com/RobTillaart/AS5600/actions/workflows/arduino-lint.yml)
[![JSON check](https://github.com/milesburton/Arduino-Temperature-Control-Library/actions/workflows/jsoncheck.yml/badge.svg)](https://github.com/RobTillaart/AS5600/actions/workflows/jsoncheck.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-green.svg)](https://github.com/milesburton/Arduino-Temperature-Control-Library/blob/master/LICENSE)
[![GitHub release](https://img.shields.io/github/release/milesburton/Arduino-Temperature-Control-Library.svg?maxAge=3600)](https://github.com/milesburton/Arduino-Temperature-Control-Library/releases)


# Arduino Library for Maxim Temperature Integrated Circuits

## Usage

This library supports the following devices :


* DS18B20
* DS18S20 - Please note there appears to be an issue with this series.
* DS1822
* DS1820
* MAX31820
* MAX31850


You will need a pull-up resistor of about 5 KOhm between the 1-Wire data line
and your 5V power. If you are using the DS18B20, ground pins 1 and 3. The
centre pin is the data line '1-wire'.

In case of temperature conversion problems (result is `-85`), strong pull-up setup may be necessary. See section 
_Powering the DS18B20_ in 
[DS18B20 datasheet](https://datasheets.maximintegrated.com/en/ds/DS18B20.pdf) (page 7)
and use `DallasTemperature(OneWire*, uint8_t)` constructor.

We have included a "REQUIRESNEW" and "REQUIRESALARMS" definition. If you 
want to slim down the code feel free to use either of these by including



	#define REQUIRESNEW 

or 

	#define REQUIRESALARMS


at the top of DallasTemperature.h

Finally, please include OneWire from Paul Stoffregen in the library manager before you begin.

## Credits

The OneWire code has been derived from
http://www.arduino.cc/playground/Learning/OneWire.
Miles Burton <miles@mnetcs.com> originally developed this library.
Tim Newsome <nuisance@casualhacker.net> added support for multiple sensors on
the same bus.
Guil Barros [gfbarros@bappos.com] added getTempByAddress (v3.5)
   Note: these are implemented as getTempC(address) and getTempF(address)
Rob Tillaart [rob.tillaart@gmail.com] added async modus (v3.7.0)


## Website


Additional documentation may be found here
https://www.milesburton.com/w/index.php/Dallas_Temperature_Control_Library

# License

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
