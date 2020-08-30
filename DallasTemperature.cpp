// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

#include "DallasTemperature.h"

#if ARDUINO >= 100
#include "Arduino.h"
#else
extern "C" {
#include "WConstants.h"
}
#endif

// OneWire commands
#define STARTCONVO      0x44  // Tells device to take a temperature reading and put it on the scratchpad
#define COPYSCRATCH     0x48  // Copy scratchpad to EEPROM
#define READSCRATCH     0xBE  // Read from scratchpad
#define WRITESCRATCH    0x4E  // Write to scratchpad
#define RECALLSCRATCH   0xB8  // Recall from EEPROM to scratchpad
#define READPOWERSUPPLY 0xB4  // Determine if device needs parasite power
#define ALARMSEARCH     0xEC  // Query bus for devices with an alarm condition

// Scratchpad locations
#define TEMP_LSB        0
#define TEMP_MSB        1
#define HIGH_ALARM_TEMP 2
#define LOW_ALARM_TEMP  3
#define CONFIGURATION   4
#define INTERNAL_BYTE   5
#define COUNT_REMAIN    6
#define COUNT_PER_C     7
#define SCRATCHPAD_CRC  8

// DSROM FIELDS
#define DSROM_FAMILY    0
#define DSROM_CRC       7

// Device resolution
#define TEMP_9_BIT  0x1F //  9 bit
#define TEMP_10_BIT 0x3F // 10 bit
#define TEMP_11_BIT 0x5F // 11 bit
#define TEMP_12_BIT 0x7F // 12 bit

#define MAX_CONVERSION_TIMEOUT		750

// Alarm handler
#define NO_ALARM_HANDLER ((AlarmHandler *)0)


DallasTemperature::DallasTemperature() {
#if REQUIRESALARMS
	setAlarmHandler(NO_ALARM_HANDLER);
#endif
    useExternalPullup = false;
}

DallasTemperature::DallasTemperature(OneWire* _oneWire) : DallasTemperature() {
	setOneWire(_oneWire);
}

bool DallasTemperature::validFamily(const uint8_t* deviceAddress) {
	switch (deviceAddress[DSROM_FAMILY]) {
	case DS18S20MODEL:
	case DS18B20MODEL:
	case DS1822MODEL:
	case DS1825MODEL:
	case DS28EA00MODEL:
		return true;
	default:
		return false;
	}
}

/*
 * Constructs DallasTemperature with strong pull-up turned on. Strong pull-up is mandated in DS18B20 datasheet for parasitic
 * power (2 wires) setup. (https://datasheets.maximintegrated.com/en/ds/DS18B20.pdf, p. 7, section 'Powering the DS18B20').
 */
DallasTemperature::DallasTemperature(OneWire* _oneWire, uint8_t _pullupPin) : DallasTemperature(_oneWire) {
  setPullupPin(_pullupPin);
}

void DallasTemperature::setPullupPin(uint8_t _pullupPin) {
	useExternalPullup = true;
	pullupPin = _pullupPin;
	pinMode(pullupPin, OUTPUT);
	deactivateExternalPullup();
}

void DallasTemperature::setOneWire(OneWire* _oneWire) {

	_wire = _oneWire;
	devices = 0;
	ds18Count = 0;
	parasite = false;
	bitResolution = 9;
	waitForConversion = true;
	checkForConversion = true;
  autoSaveScratchPad = true;

}

// initialise the bus
void DallasTemperature::begin(void) {

	DeviceAddress deviceAddress;

	_wire->reset_search();
	devices = 0; // Reset the number of devices when we enumerate wire devices
	ds18Count = 0; // Reset number of DS18xxx Family devices

	while (_wire->search(deviceAddress)) {

		if (validAddress(deviceAddress)) {
			devices++;

			if (validFamily(deviceAddress)) {
				ds18Count++;

				if (!parasite && readPowerSupply(deviceAddress))
					parasite = true;

				uint8_t b = getResolution(deviceAddress);
				if (b > bitResolution) bitResolution = b;
			}
		}
	}
}

// returns the number of devices found on the bus
uint8_t DallasTemperature::getDeviceCount(void) {
	return devices;
}

uint8_t DallasTemperature::getDS18Count(void) {
	return ds18Count;
}

// returns true if address is valid
bool DallasTemperature::validAddress(const uint8_t* deviceAddress) {
	return (_wire->crc8(deviceAddress, 7) == deviceAddress[DSROM_CRC]);
}

// finds an address at a given index on the bus
// returns true if the device was found
bool DallasTemperature::getAddress(uint8_t* deviceAddress, uint8_t index) {

	uint8_t depth = 0;

	_wire->reset_search();

	while (depth <= index && _wire->search(deviceAddress)) {
		if (depth == index && validAddress(deviceAddress))
			return true;
		depth++;
	}

	return false;

}

// attempt to determine if the device at the given address is connected to the bus
bool DallasTemperature::isConnected(const uint8_t* deviceAddress) {

	ScratchPad scratchPad;
	return isConnected(deviceAddress, scratchPad);

}

// attempt to determine if the device at the given address is connected to the bus
// also allows for updating the read scratchpad
bool DallasTemperature::isConnected(const uint8_t* deviceAddress,
		uint8_t* scratchPad) {
	bool b = readScratchPad(deviceAddress, scratchPad);
	return b && !isAllZeros(scratchPad) && (_wire->crc8(scratchPad, 8) == scratchPad[SCRATCHPAD_CRC]);
}

bool DallasTemperature::readScratchPad(const uint8_t* deviceAddress,
		uint8_t* scratchPad) {

	// send the reset command and fail fast
	int b = _wire->reset();
	if (b == 0)
		return false;

	_wire->select(deviceAddress);
	_wire->write(READSCRATCH);

	// Read all registers in a simple loop
	// byte 0: temperature LSB
	// byte 1: temperature MSB
	// byte 2: high alarm temp
	// byte 3: low alarm temp
	// byte 4: DS18S20: store for crc
	//         DS18B20 & DS1822: configuration register
	// byte 5: internal use & crc
	// byte 6: DS18S20: COUNT_REMAIN
	//         DS18B20 & DS1822: store for crc
	// byte 7: DS18S20: COUNT_PER_C
	//         DS18B20 & DS1822: store for crc
	// byte 8: SCRATCHPAD_CRC
	for (uint8_t i = 0; i < 9; i++) {
		scratchPad[i] = _wire->read();
	}

	b = _wire->reset();
	return (b == 1);
}

void DallasTemperature::writeScratchPad(const uint8_t* deviceAddress,
		const uint8_t* scratchPad) {

	_wire->reset();
	_wire->select(deviceAddress);
	_wire->write(WRITESCRATCH);
	_wire->write(scratchPad[HIGH_ALARM_TEMP]); // high alarm temp
	_wire->write(scratchPad[LOW_ALARM_TEMP]); // low alarm temp

	// DS1820 and DS18S20 have no configuration register
	if (deviceAddress[DSROM_FAMILY] != DS18S20MODEL)
		_wire->write(scratchPad[CONFIGURATION]);

  if (autoSaveScratchPad)
    saveScratchPad(deviceAddress);
  else
    _wire->reset();
}

// returns true if parasite mode is used (2 wire)
// returns false if normal mode is used (3 wire)
// if no address is given (or nullptr) it checks if any device on the bus
// uses parasite mode.
// See issue #145
bool DallasTemperature::readPowerSupply(const uint8_t* deviceAddress)
{
	bool parasiteMode = false;
	_wire->reset();
	if (deviceAddress == nullptr)
		_wire->skip();
	else
		_wire->select(deviceAddress);

	_wire->write(READPOWERSUPPLY);
	if (_wire->read_bit() == 0)
		parasiteMode = true;
	_wire->reset();
	return parasiteMode;
}

// set resolution of all devices to 9, 10, 11, or 12 bits
// if new resolution is out of range, it is constrained.
void DallasTemperature::setResolution(uint8_t newResolution) {

	bitResolution = constrain(newResolution, 9, 12);
	DeviceAddress deviceAddress;
	for (uint8_t i = 0; i < devices; i++) {
		getAddress(deviceAddress, i);
		setResolution(deviceAddress, bitResolution, true);
	}
}

/*  PROPOSAL */

// set resolution of a device to 9, 10, 11, or 12 bits
// if new resolution is out of range, 9 bits is used.
bool DallasTemperature::setResolution(const uint8_t* deviceAddress,
                                      uint8_t newResolution, bool skipGlobalBitResolutionCalculation) {

  bool success = false;

  // DS1820 and DS18S20 have no resolution configuration register
  if (deviceAddress[DSROM_FAMILY] == DS18S20MODEL)
  {
    success = true;
  }
  else
  {

   // handle the sensors with configuration register
    newResolution = constrain(newResolution, 9, 12);
    uint8_t newValue = 0;
    ScratchPad scratchPad;

    // we can only update the sensor if it is connected
    if (isConnected(deviceAddress, scratchPad))
    {
      switch (newResolution) {
        case 12:
          newValue = TEMP_12_BIT;
          break;
        case 11:
          newValue = TEMP_11_BIT;
          break;
        case 10:
          newValue = TEMP_10_BIT;
          break;
        case 9:
        default:
          newValue = TEMP_9_BIT;
          break;
      }

      // if it needs to be updated we write the new value
      if (scratchPad[CONFIGURATION] != newValue)
      {
		scratchPad[CONFIGURATION] = newValue;
        writeScratchPad(deviceAddress, scratchPad);
      }
      // done
      success = true;
    }
  }

  // do we need to update the max resolution used?
  if (skipGlobalBitResolutionCalculation == false)
  {
    bitResolution = newResolution;
    if (devices > 1)
    {
      for (uint8_t i = 0; i < devices; i++)
      {
        if (bitResolution == 12) break;
        DeviceAddress deviceAddr;
        getAddress(deviceAddr, i);
        uint8_t b = getResolution(deviceAddr);
        if (b > bitResolution) bitResolution = b;
      }
    }
  }

  return success;
}


// returns the global resolution
uint8_t DallasTemperature::getResolution() {
	return bitResolution;
}

// returns the current resolution of the device, 9-12
// returns 0 if device not found
uint8_t DallasTemperature::getResolution(const uint8_t* deviceAddress) {

	// DS1820 and DS18S20 have no resolution configuration register
	if (deviceAddress[DSROM_FAMILY] == DS18S20MODEL)
		return 12;

	ScratchPad scratchPad;
	if (isConnected(deviceAddress, scratchPad)) {
		switch (scratchPad[CONFIGURATION]) {
		case TEMP_12_BIT:
			return 12;

		case TEMP_11_BIT:
			return 11;

		case TEMP_10_BIT:
			return 10;

		case TEMP_9_BIT:
			return 9;
		}
	}
	return 0;

}


// sets the value of the waitForConversion flag
// TRUE : function requestTemperature() etc returns when conversion is ready
// FALSE: function requestTemperature() etc returns immediately (USE WITH CARE!!)
//        (1) programmer has to check if the needed delay has passed
//        (2) but the application can do meaningful things in that time
void DallasTemperature::setWaitForConversion(bool flag) {
	waitForConversion = flag;
}

// gets the value of the waitForConversion flag
bool DallasTemperature::getWaitForConversion() {
	return waitForConversion;
}

// sets the value of the checkForConversion flag
// TRUE : function requestTemperature() etc will 'listen' to an IC to determine whether a conversion is complete
// FALSE: function requestTemperature() etc will wait a set time (worst case scenario) for a conversion to complete
void DallasTemperature::setCheckForConversion(bool flag) {
	checkForConversion = flag;
}

// gets the value of the waitForConversion flag
bool DallasTemperature::getCheckForConversion() {
	return checkForConversion;
}

bool DallasTemperature::isConversionComplete() {
	uint8_t b = _wire->read_bit();
	return (b == 1);
}

// sends command for all devices on the bus to perform a temperature conversion
void DallasTemperature::requestTemperatures() {

	_wire->reset();
	_wire->skip();
	_wire->write(STARTCONVO, parasite);

	// ASYNC mode?
	if (!waitForConversion)
		return;
	blockTillConversionComplete(bitResolution);

}

// sends command for one device to perform a temperature by address
// returns FALSE if device is disconnected
// returns TRUE  otherwise
bool DallasTemperature::requestTemperaturesByAddress(
		const uint8_t* deviceAddress) {

	uint8_t bitResolution = getResolution(deviceAddress);
	if (bitResolution == 0) {
		return false; //Device disconnected
	}

	_wire->reset();
	_wire->select(deviceAddress);
	_wire->write(STARTCONVO, parasite);

	// ASYNC mode?
	if (!waitForConversion)
		return true;

	blockTillConversionComplete(bitResolution);

	return true;

}

// Continue to check if the IC has responded with a temperature
void DallasTemperature::blockTillConversionComplete(uint8_t bitResolution) {

  if (checkForConversion && !parasite) {
    unsigned long start = millis();
    while (!isConversionComplete() && (millis() - start < MAX_CONVERSION_TIMEOUT ))
      yield();
  } else {
    unsigned long delms = millisToWaitForConversion(bitResolution);
    activateExternalPullup();
    delay(delms);
    deactivateExternalPullup();
  }

}

// returns number of milliseconds to wait till conversion is complete (based on IC datasheet)
int16_t DallasTemperature::millisToWaitForConversion(uint8_t bitResolution) {

	switch (bitResolution) {
	case 9:
		return 94;
	case 10:
		return 188;
	case 11:
		return 375;
	default:
		return 750;
	}

}

// Sends command to one device to save values from scratchpad to EEPROM by index
// Returns true if no errors were encountered, false indicates failure
bool DallasTemperature::saveScratchPadByIndex(uint8_t deviceIndex) {
  
  DeviceAddress deviceAddress;
  if (!getAddress(deviceAddress, deviceIndex)) return false;
  
  return saveScratchPad(deviceAddress);
  
}

// Sends command to one or more devices to save values from scratchpad to EEPROM
// If optional argument deviceAddress is omitted the command is send to all devices
// Returns true if no errors were encountered, false indicates failure
bool DallasTemperature::saveScratchPad(const uint8_t* deviceAddress) {
  
  if (_wire->reset() == 0)
    return false;
  
  if (deviceAddress == nullptr)
    _wire->skip();
  else
    _wire->select(deviceAddress);
  
  _wire->write(COPYSCRATCH,parasite);

  // Specification: NV Write Cycle Time is typically 2ms, max 10ms
  // Waiting 20ms to allow for sensors that take longer in practice
  if (!parasite) {
    delay(20);
  } else {
    activateExternalPullup();
    delay(20);
    deactivateExternalPullup();
  }
  
  return _wire->reset() == 1;
  
}

// Sends command to one device to recall values from EEPROM to scratchpad by index
// Returns true if no errors were encountered, false indicates failure
bool DallasTemperature::recallScratchPadByIndex(uint8_t deviceIndex) {

  DeviceAddress deviceAddress;
  if (!getAddress(deviceAddress, deviceIndex)) return false;
  
  return recallScratchPad(deviceAddress);

}

// Sends command to one or more devices to recall values from EEPROM to scratchpad
// If optional argument deviceAddress is omitted the command is send to all devices
// Returns true if no errors were encountered, false indicates failure
bool DallasTemperature::recallScratchPad(const uint8_t* deviceAddress) {
  
  if (_wire->reset() == 0)
    return false;
  
  if (deviceAddress == nullptr)
    _wire->skip();
  else
    _wire->select(deviceAddress);
  
  _wire->write(RECALLSCRATCH,parasite);

  // Specification: Strong pullup only needed when writing to EEPROM (and temp conversion)
  unsigned long start = millis();
  while (_wire->read_bit() == 0) {
    // Datasheet doesn't specify typical/max duration, testing reveals typically within 1ms
    if (millis() - start > 20) return false;
    yield();
  }
  
  return _wire->reset() == 1;
  
}

// Sets the autoSaveScratchPad flag
void DallasTemperature::setAutoSaveScratchPad(bool flag) {
  autoSaveScratchPad = flag;
}

// Gets the autoSaveScratchPad flag
bool DallasTemperature::getAutoSaveScratchPad() {
  return autoSaveScratchPad;
}

void DallasTemperature::activateExternalPullup() {
	if(useExternalPullup)
		digitalWrite(pullupPin, LOW);
}

void DallasTemperature::deactivateExternalPullup() {
	if(useExternalPullup)
		digitalWrite(pullupPin, HIGH);
}

// sends command for one device to perform a temp conversion by index
bool DallasTemperature::requestTemperaturesByIndex(uint8_t deviceIndex) {

	DeviceAddress deviceAddress;
	getAddress(deviceAddress, deviceIndex);

	return requestTemperaturesByAddress(deviceAddress);

}

// Fetch temperature for device index
float DallasTemperature::getTempCByIndex(uint8_t deviceIndex) {

	DeviceAddress deviceAddress;
	if (!getAddress(deviceAddress, deviceIndex)) {
		return DEVICE_DISCONNECTED_C;
	}
	return getTempC((uint8_t*) deviceAddress);
}

// Fetch temperature for device index
float DallasTemperature::getTempFByIndex(uint8_t deviceIndex) {

	DeviceAddress deviceAddress;

	if (!getAddress(deviceAddress, deviceIndex)) {
		return DEVICE_DISCONNECTED_F;
	}

	return getTempF((uint8_t*) deviceAddress);

}

// reads scratchpad and returns fixed-point temperature, scaling factor 2^-7
int16_t DallasTemperature::calculateTemperature(const uint8_t* deviceAddress,
		uint8_t* scratchPad) {

	int16_t fpTemperature = (((int16_t) scratchPad[TEMP_MSB]) << 11)
			| (((int16_t) scratchPad[TEMP_LSB]) << 3);

	/*
	 DS1820 and DS18S20 have a 9-bit temperature register.

	 Resolutions greater than 9-bit can be calculated using the data from
	 the temperature, and COUNT REMAIN and COUNT PER °C registers in the
	 scratchpad.  The resolution of the calculation depends on the model.

	 While the COUNT PER °C register is hard-wired to 16 (10h) in a
	 DS18S20, it changes with temperature in DS1820.

	 After reading the scratchpad, the TEMP_READ value is obtained by
	 truncating the 0.5°C bit (bit 0) from the temperature data. The
	 extended resolution temperature can then be calculated using the
	 following equation:

	                                  COUNT_PER_C - COUNT_REMAIN
	 TEMPERATURE = TEMP_READ - 0.25 + --------------------------
	                                         COUNT_PER_C

	 Hagai Shatz simplified this to integer arithmetic for a 12 bits
	 value for a DS18S20, and James Cameron added legacy DS1820 support.

	 See - http://myarduinotoy.blogspot.co.uk/2013/02/12bit-result-from-ds18s20.html
	 */

	if ((deviceAddress[DSROM_FAMILY] == DS18S20MODEL) && (scratchPad[COUNT_PER_C] != 0)) {
		fpTemperature = ((fpTemperature & 0xfff0) << 3) - 32
				+ (((scratchPad[COUNT_PER_C] - scratchPad[COUNT_REMAIN]) << 7)
						/ scratchPad[COUNT_PER_C]);
	}

	return fpTemperature;
}

// returns temperature in 1/128 degrees C or DEVICE_DISCONNECTED_RAW if the
// device's scratch pad cannot be read successfully.
// the numeric value of DEVICE_DISCONNECTED_RAW is defined in
// DallasTemperature.h. It is a large negative number outside the
// operating range of the device
int16_t DallasTemperature::getTemp(const uint8_t* deviceAddress) {

	ScratchPad scratchPad;
	if (isConnected(deviceAddress, scratchPad))
		return calculateTemperature(deviceAddress, scratchPad);
	return DEVICE_DISCONNECTED_RAW;

}

// returns temperature in degrees C or DEVICE_DISCONNECTED_C if the
// device's scratch pad cannot be read successfully.
// the numeric value of DEVICE_DISCONNECTED_C is defined in
// DallasTemperature.h. It is a large negative number outside the
// operating range of the device
float DallasTemperature::getTempC(const uint8_t* deviceAddress) {
	return rawToCelsius(getTemp(deviceAddress));
}

// returns temperature in degrees F or DEVICE_DISCONNECTED_F if the
// device's scratch pad cannot be read successfully.
// the numeric value of DEVICE_DISCONNECTED_F is defined in
// DallasTemperature.h. It is a large negative number outside the
// operating range of the device
float DallasTemperature::getTempF(const uint8_t* deviceAddress) {
	return rawToFahrenheit(getTemp(deviceAddress));
}

// returns true if the bus requires parasite power
bool DallasTemperature::isParasitePowerMode(void) {
	return parasite;
}

// IF alarm is not used one can store a 16 bit int of userdata in the alarm
// registers. E.g. an ID of the sensor.
// See github issue #29

// note if device is not connected it will fail writing the data.
void DallasTemperature::setUserData(const uint8_t* deviceAddress,
		int16_t data) {
	// return when stored value == new value
	if (getUserData(deviceAddress) == data)
		return;

	ScratchPad scratchPad;
	if (isConnected(deviceAddress, scratchPad)) {
		scratchPad[HIGH_ALARM_TEMP] = data >> 8;
		scratchPad[LOW_ALARM_TEMP] = data & 255;
		writeScratchPad(deviceAddress, scratchPad);
	}
}

int16_t DallasTemperature::getUserData(const uint8_t* deviceAddress) {
	int16_t data = 0;
	ScratchPad scratchPad;
	if (isConnected(deviceAddress, scratchPad)) {
		data = scratchPad[HIGH_ALARM_TEMP] << 8;
		data += scratchPad[LOW_ALARM_TEMP];
	}
	return data;
}

// note If address cannot be found no error will be reported.
int16_t DallasTemperature::getUserDataByIndex(uint8_t deviceIndex) {
	DeviceAddress deviceAddress;
	getAddress(deviceAddress, deviceIndex);
	return getUserData((uint8_t*) deviceAddress);
}

void DallasTemperature::setUserDataByIndex(uint8_t deviceIndex, int16_t data) {
	DeviceAddress deviceAddress;
	getAddress(deviceAddress, deviceIndex);
	setUserData((uint8_t*) deviceAddress, data);
}

// Convert float Celsius to Fahrenheit
float DallasTemperature::toFahrenheit(float celsius) {
	return (celsius * 1.8f) + 32.0f;
}

// Convert float Fahrenheit to Celsius
float DallasTemperature::toCelsius(float fahrenheit) {
	return (fahrenheit - 32.0f) * 0.555555556f;
}

// convert from raw to Celsius
float DallasTemperature::rawToCelsius(int16_t raw) {

	if (raw <= DEVICE_DISCONNECTED_RAW)
		return DEVICE_DISCONNECTED_C;
	// C = RAW/128
	return (float) raw * 0.0078125f;

}

// convert from raw to Fahrenheit
float DallasTemperature::rawToFahrenheit(int16_t raw) {

	if (raw <= DEVICE_DISCONNECTED_RAW)
		return DEVICE_DISCONNECTED_F;
	// C = RAW/128
	// F = (C*1.8)+32 = (RAW/128*1.8)+32 = (RAW*0.0140625)+32
	return ((float) raw * 0.0140625f) + 32.0f;

}

// Returns true if all bytes of scratchPad are '\0'
bool DallasTemperature::isAllZeros(const uint8_t * const scratchPad, const size_t length) {
	for (size_t i = 0; i < length; i++) {
		if (scratchPad[i] != 0) {
			return false;
		}
	}

	return true;
}

#if REQUIRESALARMS

/*

 ALARMS:

 TH and TL Register Format

 BIT 7 BIT 6 BIT 5 BIT 4 BIT 3 BIT 2 BIT 1 BIT 0
 S    2^6   2^5   2^4   2^3   2^2   2^1   2^0

 Only bits 11 through 4 of the temperature register are used
 in the TH and TL comparison since TH and TL are 8-bit
 registers. If the measured temperature is lower than or equal
 to TL or higher than or equal to TH, an alarm condition exists
 and an alarm flag is set inside the DS18B20. This flag is
 updated after every temperature measurement; therefore, if the
 alarm condition goes away, the flag will be turned off after
 the next temperature conversion.

 */

// sets the high alarm temperature for a device in degrees Celsius
// accepts a float, but the alarm resolution will ignore anything
// after a decimal point.  valid range is -55C - 125C
void DallasTemperature::setHighAlarmTemp(const uint8_t* deviceAddress,
		int8_t celsius) {

	// return when stored value == new value
	if (getHighAlarmTemp(deviceAddress) == celsius)
		return;

	// make sure the alarm temperature is within the device's range
	if (celsius > 125)
		celsius = 125;
	else if (celsius < -55)
		celsius = -55;

	ScratchPad scratchPad;
	if (isConnected(deviceAddress, scratchPad)) {
		scratchPad[HIGH_ALARM_TEMP] = (uint8_t) celsius;
		writeScratchPad(deviceAddress, scratchPad);
	}

}

// sets the low alarm temperature for a device in degrees Celsius
// accepts a float, but the alarm resolution will ignore anything
// after a decimal point.  valid range is -55C - 125C
void DallasTemperature::setLowAlarmTemp(const uint8_t* deviceAddress,
		int8_t celsius) {

	// return when stored value == new value
	if (getLowAlarmTemp(deviceAddress) == celsius)
		return;

	// make sure the alarm temperature is within the device's range
	if (celsius > 125)
		celsius = 125;
	else if (celsius < -55)
		celsius = -55;

	ScratchPad scratchPad;
	if (isConnected(deviceAddress, scratchPad)) {
		scratchPad[LOW_ALARM_TEMP] = (uint8_t) celsius;
		writeScratchPad(deviceAddress, scratchPad);
	}

}

// returns a int8_t with the current high alarm temperature or
// DEVICE_DISCONNECTED for an address
int8_t DallasTemperature::getHighAlarmTemp(const uint8_t* deviceAddress) {

	ScratchPad scratchPad;
	if (isConnected(deviceAddress, scratchPad))
		return (int8_t) scratchPad[HIGH_ALARM_TEMP];
	return DEVICE_DISCONNECTED_C;

}

// returns a int8_t with the current low alarm temperature or
// DEVICE_DISCONNECTED for an address
int8_t DallasTemperature::getLowAlarmTemp(const uint8_t* deviceAddress) {

	ScratchPad scratchPad;
	if (isConnected(deviceAddress, scratchPad))
		return (int8_t) scratchPad[LOW_ALARM_TEMP];
	return DEVICE_DISCONNECTED_C;

}

// resets internal variables used for the alarm search
void DallasTemperature::resetAlarmSearch() {

	alarmSearchJunction = -1;
	alarmSearchExhausted = 0;
	for (uint8_t i = 0; i < 7; i++) {
		alarmSearchAddress[i] = 0;
	}

}

// This is a modified version of the OneWire::search method.
//
// Also added the OneWire search fix documented here:
// http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1238032295
//
// Perform an alarm search. If this function returns a '1' then it has
// enumerated the next device and you may retrieve the ROM from the
// OneWire::address variable. If there are no devices, no further
// devices, or something horrible happens in the middle of the
// enumeration then a 0 is returned.  If a new device is found then
// its address is copied to newAddr.  Use
// DallasTemperature::resetAlarmSearch() to start over.
bool DallasTemperature::alarmSearch(uint8_t* newAddr) {

	uint8_t i;
	int8_t lastJunction = -1;
	uint8_t done = 1;

	if (alarmSearchExhausted)
		return false;
	if (!_wire->reset())
		return false;

	// send the alarm search command
	_wire->write(0xEC, 0);

	for (i = 0; i < 64; i++) {

		uint8_t a = _wire->read_bit();
		uint8_t nota = _wire->read_bit();
		uint8_t ibyte = i / 8;
		uint8_t ibit = 1 << (i & 7);

		// I don't think this should happen, this means nothing responded, but maybe if
		// something vanishes during the search it will come up.
		if (a && nota)
			return false;

		if (!a && !nota) {
			if (i == alarmSearchJunction) {
				// this is our time to decide differently, we went zero last time, go one.
				a = 1;
				alarmSearchJunction = lastJunction;
			} else if (i < alarmSearchJunction) {

				// take whatever we took last time, look in address
				if (alarmSearchAddress[ibyte] & ibit) {
					a = 1;
				} else {
					// Only 0s count as pending junctions, we've already exhausted the 0 side of 1s
					a = 0;
					done = 0;
					lastJunction = i;
				}
			} else {
				// we are blazing new tree, take the 0
				a = 0;
				alarmSearchJunction = i;
				done = 0;
			}
			// OneWire search fix
			// See: http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1238032295
		}

		if (a)
			alarmSearchAddress[ibyte] |= ibit;
		else
			alarmSearchAddress[ibyte] &= ~ibit;

		_wire->write_bit(a);
	}

	if (done)
		alarmSearchExhausted = 1;
	for (i = 0; i < 8; i++)
		newAddr[i] = alarmSearchAddress[i];
	return true;

}

// returns true if device address might have an alarm condition
// (only an alarm search can verify this)
bool DallasTemperature::hasAlarm(const uint8_t* deviceAddress) {

	ScratchPad scratchPad;
	if (isConnected(deviceAddress, scratchPad)) {

		int8_t temp = calculateTemperature(deviceAddress, scratchPad) >> 7;

		// check low alarm
		if (temp <= (int8_t) scratchPad[LOW_ALARM_TEMP])
			return true;

		// check high alarm
		if (temp >= (int8_t) scratchPad[HIGH_ALARM_TEMP])
			return true;
	}

	// no alarm
	return false;

}

// returns true if any device is reporting an alarm condition on the bus
bool DallasTemperature::hasAlarm(void) {

	DeviceAddress deviceAddress;
	resetAlarmSearch();
	return alarmSearch(deviceAddress);
}

// runs the alarm handler for all devices returned by alarmSearch()
// unless there no _AlarmHandler exist.
void DallasTemperature::processAlarms(void) {

if (!hasAlarmHandler())
{
	return;
}

	resetAlarmSearch();
	DeviceAddress alarmAddr;

	while (alarmSearch(alarmAddr)) {
		if (validAddress(alarmAddr)) {
			_AlarmHandler(alarmAddr);
		}
	}
}

// sets the alarm handler
void DallasTemperature::setAlarmHandler(const AlarmHandler *handler) {
	_AlarmHandler = handler;
}

// checks if AlarmHandler has been set.
bool DallasTemperature::hasAlarmHandler()
{
  return _AlarmHandler != NO_ALARM_HANDLER;
}

#endif

#if REQUIRESNEW

// MnetCS - Allocates memory for DallasTemperature. Allows us to instance a new object
void* DallasTemperature::operator new(unsigned int size) { // Implicit NSS obj size

	void * p;// void pointer
	p = malloc(size);// Allocate memory
	memset((DallasTemperature*)p,0,size);// Initialise memory

	//!!! CANT EXPLICITLY CALL CONSTRUCTOR - workaround by using an init() methodR - workaround by using an init() method
	return (DallasTemperature*) p;// Cast blank region to NSS pointer
}

// MnetCS 2009 -  Free the memory used by this instance
void DallasTemperature::operator delete(void* p) {

	DallasTemperature* pNss = (DallasTemperature*) p; // Cast to NSS pointer
	pNss->~DallasTemperature();// Destruct the object

	free(p);// Free the memory
}

#endif
