#ifndef DallasTemperature_h
#define DallasTemperature_h

#define DALLASTEMPLIBVERSION "3.8.1" // To be deprecated -> TODO remove in 4.0.0

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// set to true to include code for new and delete operators
#ifndef REQUIRESNEW
#define REQUIRESNEW false
#endif

// set to true to include code implementing alarm search functions
#ifndef REQUIRESALARMS
#define REQUIRESALARMS true
#endif

#include <inttypes.h>
#ifdef __STM32F1__
#include <OneWireSTM.h>
#else
#include <OneWire.h>
#endif

// Model IDs
#define DS18S20MODEL 0x10  // also DS1820
#define DS18B20MODEL 0x28  // also MAX31820
#define DS1822MODEL  0x22
#define DS1825MODEL  0x3B  // also MAX31850
#define DS28EA00MODEL 0x42

// Error Codes
// See https://github.com/milesburton/Arduino-Temperature-Control-Library/commit/ac1eb7f56e3894e855edc3353be4bde4aa838d41#commitcomment-75490966 for the 16bit implementation. Reverted due to microcontroller resource constraints.
#define DEVICE_DISCONNECTED_C -127
#define DEVICE_DISCONNECTED_F -196.6
#define DEVICE_DISCONNECTED_RAW -7040

#define DEVICE_FAULT_OPEN_C -254
#define DEVICE_FAULT_OPEN_F -425.199982
#define DEVICE_FAULT_OPEN_RAW -32512

#define DEVICE_FAULT_SHORTGND_C -253
#define DEVICE_FAULT_SHORTGND_F -423.399994
#define DEVICE_FAULT_SHORTGND_RAW -32384

#define DEVICE_FAULT_SHORTVDD_C -252
#define DEVICE_FAULT_SHORTVDD_F -421.599976
#define DEVICE_FAULT_SHORTVDD_RAW -32256

// For readPowerSupply on oneWire bus
// definition of nullptr for C++ < 11, using official workaround:
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2431.pdf
#if __cplusplus < 201103L
const class
{
public:
	template <class T>
	operator T *() const {
		return 0;
	}

	template <class C, class T>
	operator T C::*() const {
		return 0;
	}

private:
	void operator&() const;
} nullptr = {};
#endif

typedef uint8_t DeviceAddress[8];

class DallasTemperature {
public:

	DallasTemperature();
	DallasTemperature(OneWire*);
	DallasTemperature(OneWire*, uint8_t);

	void setOneWire(OneWire*);

	void setPullupPin(uint8_t);

	// initialise bus
	void begin(void);

	// returns the number of devices found on the bus
	uint8_t getDeviceCount(void);

	// returns the number of DS18xxx Family devices on bus
	uint8_t getDS18Count(void);

	// returns true if address is valid
	bool validAddress(const uint8_t*);

	// returns true if address is of the family of sensors the lib supports.
	bool validFamily(const uint8_t* deviceAddress);

	// finds an address at a given index on the bus
	bool getAddress(uint8_t*, uint8_t);

	// attempt to determine if the device at the given address is connected to the bus
	bool isConnected(const uint8_t*);

	// attempt to determine if the device at the given address is connected to the bus
	// also allows for updating the read scratchpad
	bool isConnected(const uint8_t*, uint8_t*);

	// read device's scratchpad
	bool readScratchPad(const uint8_t*, uint8_t*);

	// write device's scratchpad
	void writeScratchPad(const uint8_t*, const uint8_t*);

	// read device's power requirements
	bool readPowerSupply(const uint8_t* deviceAddress = nullptr);

	// get global resolution
	uint8_t getResolution();

	// set global resolution to 9, 10, 11, or 12 bits
	void setResolution(uint8_t);

	// returns the device resolution: 9, 10, 11, or 12 bits
	uint8_t getResolution(const uint8_t*);

	// set resolution of a device to 9, 10, 11, or 12 bits
	bool setResolution(const uint8_t*, uint8_t,
	                   bool skipGlobalBitResolutionCalculation = false);

	// sets/gets the waitForConversion flag
	void setWaitForConversion(bool);
	bool getWaitForConversion(void);

	// sets/gets the checkForConversion flag
	void setCheckForConversion(bool);
	bool getCheckForConversion(void);

	// convert from Celsius to Fahrenheit
	static float toFahrenheit(float);

	// convert from Fahrenheit to Celsius
	static float toCelsius(float);

	// convert from raw to Celsius
	static float rawToCelsius(int32_t);

	// convert from Celsius to raw
	static int16_t celsiusToRaw(float);

	// convert from raw to Fahrenheit
	static float rawToFahrenheit(int32_t);

	struct request_t {
		bool result;
		unsigned long timestamp;

		operator bool() {
			return result;
		}
	};

	enum device_error_code {
		device_ok                 = 0,
		device_connected          = 0,
		device_fault_open         = 1,
		device_fault_shortgnd     = 2,
		device_fault_shortvdd     = 4,
		device_fault_general      = 8,
		device_fault_disconnected = 16
	};

	// Treat temperatures as their own distinct types
	struct celsius_unit_t;
	struct fahrenheit_unit_t;
	struct kelvin_unit_t;

	// NOTE: conversions back to raw units?
	struct raw_unit_t {
		int32_t raw;

		raw_unit_t() = default;

		celsius_unit_t in_celsius() {
			celsius_unit_t c;
			c.celsius = rawToCelsius(raw);
			return c;
		};

		kelvin_unit_t in_kelvin() {
			kelvin_unit_t k;
			k.kelvin = rawToCelsius(raw) + 273.15f;
			return k;
		};

		fahrenheit_unit_t in_fahrenheit() {
			fahrenheit_unit_t f;
			f.fahrenheit = rawToFahrenheit(raw);
			return f;
		};
	};

	struct celsius_unit_t {
		float celsius;
		
		celsius_unit_t() = default;

		celsius_unit_t(raw_unit_t r) {
			celsius = rawToCelsius(r.raw);
		}

		void from_raw(raw_unit_t r) {
			celsius = rawToCelsius(r.raw);
		}

		celsius_unit_t in_celsius() {
			return *this;
		};

		kelvin_unit_t in_kelvin() {
			kelvin_unit_t k;
			k.kelvin = celsius + 273.15f;
			return k;
		};

		fahrenheit_unit_t in_fahrenheit() {
			fahrenheit_unit_t f;
			f.fahrenheit = toFahrenheit(celsius);
			return f;
		};
	};

	struct fahrenheit_unit_t {
		float fahrenheit;

		fahrenheit_unit_t() = default;

		fahrenheit_unit_t(raw_unit_t r) {
			fahrenheit = rawToFahrenheit(r.raw);
		}

		void from_raw(raw_unit_t r) {
			fahrenheit = rawToFahrenheit(r.raw);
		}

		celsius_unit_t in_celsius() {
			celsius_unit_t c;
			c.celsius = toCelsius(fahrenheit);
			return c;
		};

		kelvin_unit_t in_kelvin() {
			kelvin_unit_t k;
			k.kelvin = toCelsius(fahrenheit) + 273.15f;
			return k;
		};

		fahrenheit_unit_t in_fahrenheit() {
			return *this;
		};
	};

	struct kelvin_unit_t {
		float kelvin;

		kelvin_unit_t() = default;

		kelvin_unit_t(raw_unit_t r) {
			kelvin = rawToCelsius(r.raw) + 273.15f;
		}

		void from_raw(raw_unit_t r) {
			kelvin = rawToCelsius(r.raw) + 273.15f;
		}

		celsius_unit_t in_celsius() {
			celsius_unit_t c;
			c.celsius = kelvin - 273.15f;
			return c;
		};

		kelvin_unit_t in_kelvin() {
			return *this;
		};

		fahrenheit_unit_t in_fahrenheit() {
			fahrenheit_unit_t f;
			f.fahrenheit = toFahrenheit(kelvin - 273.15f);
			return f;
		};
	};

	struct raw_result_t {
		raw_unit_t reading;
		uint32_t error_code;

		raw_result_t() = default;

		// NOTE: implicit conversion here is for backwards compatability
		operator int32_t() {
			return reading.raw;
		}
	};

	struct celsius_result_t {
		celsius_unit_t value;
		uint32_t error_code = DallasTemperature::device_error_code::device_ok;

		void from_raw_result(raw_result_t r) {
			value.celsius = rawToCelsius(r.reading.raw);
			error_code = r.error_code;
		}

		// NOTE: implicit conversion here is for backwards compatability
		operator float() {
			return value.celsius;
		};
	};

	struct fahrenheit_result_t {
		fahrenheit_unit_t value;
		uint32_t error_code = DallasTemperature::device_error_code::device_ok;

		void from_raw_result(raw_result_t r) {
			value.fahrenheit = rawToFahrenheit(r.reading.raw);
			error_code = r.error_code;
		}

		// NOTE: implicit conversion here is for backwards compatability
		operator float() {
			return value.fahrenheit;
		};
	};

	struct kelvin_result_t {
		kelvin_unit_t value;
		uint32_t error_code = DallasTemperature::device_error_code::device_ok;

		void from_raw_result(raw_result_t r) {
			value.kelvin = rawToCelsius(r.reading.raw) + 273.15f;
			error_code = r.error_code; 
		}

		// NOTE: not including implicit conversion for kelvin simply because currently
		// there are no functions that returns temperatures in kelvin no need to be
		// backwards compatible here
	};

	// sends command for all devices on the bus to perform a temperature conversion
	request_t requestTemperatures(void);

	// sends command for one device to perform a temperature conversion by address
	request_t requestTemperaturesByAddress(const uint8_t*);

	// sends command for one device to perform a temperature conversion by index
	request_t requestTemperaturesByIndex(uint8_t);

	// returns temperature raw value (12 bit integer of 1/128 degrees C)
	raw_result_t getTemp(const uint8_t*);

	// returns temperature in degrees C
	celsius_result_t getTempC(const uint8_t*);

	// returns temperature in degrees F
	fahrenheit_result_t getTempF(const uint8_t*);

	// Get temperature for device index (slow)
	celsius_result_t getTempCByIndex(uint8_t);

	// Get temperature for device index (slow)
	fahrenheit_result_t getTempFByIndex(uint8_t);

	// returns true if the bus requires parasite power
	bool isParasitePowerMode(void);

	// Is a conversion complete on the wire? Only applies to the first sensor on the wire.
	bool isConversionComplete(void);

	static uint16_t millisToWaitForConversion(uint8_t);

	uint16_t millisToWaitForConversion();

	// Sends command to one device to save values from scratchpad to EEPROM by index
	// Returns true if no errors were encountered, false indicates failure
	bool saveScratchPadByIndex(uint8_t);

	// Sends command to one or more devices to save values from scratchpad to EEPROM
	// Returns true if no errors were encountered, false indicates failure
	bool saveScratchPad(const uint8_t* = nullptr);

	// Sends command to one device to recall values from EEPROM to scratchpad by index
	// Returns true if no errors were encountered, false indicates failure
	bool recallScratchPadByIndex(uint8_t);

	// Sends command to one or more devices to recall values from EEPROM to scratchpad
	// Returns true if no errors were encountered, false indicates failure
	bool recallScratchPad(const uint8_t* = nullptr);

	// Sets the autoSaveScratchPad flag
	void setAutoSaveScratchPad(bool);

	// Gets the autoSaveScratchPad flag
	bool getAutoSaveScratchPad(void);

#if REQUIRESALARMS

	typedef void AlarmHandler(const uint8_t*);

	// sets the high alarm temperature for a device
	// accepts a int8_t.  valid range is -55C - 125C
	void setHighAlarmTemp(const uint8_t*, int8_t);

	// sets the low alarm temperature for a device
	// accepts a int8_t.  valid range is -55C - 125C
	void setLowAlarmTemp(const uint8_t*, int8_t);

	// returns a int8_t with the current high alarm temperature for a device
	// in the range -55C - 125C
	int8_t getHighAlarmTemp(const uint8_t*);

	// returns a int8_t with the current low alarm temperature for a device
	// in the range -55C - 125C
	int8_t getLowAlarmTemp(const uint8_t*);

	// resets internal variables used for the alarm search
	void resetAlarmSearch(void);

	// search the wire for devices with active alarms
	bool alarmSearch(uint8_t*);

	// returns true if ia specific device has an alarm
	bool hasAlarm(const uint8_t*);

	// returns true if any device is reporting an alarm on the bus
	bool hasAlarm(void);

	// runs the alarm handler for all devices returned by alarmSearch()
	void processAlarms(void);

	// sets the alarm handler
	void setAlarmHandler(const AlarmHandler *);

	// returns true if an AlarmHandler has been set
	bool hasAlarmHandler();

#endif

	// if no alarm handler is used the two bytes can be used as user data
	// example of such usage is an ID.
	// note if device is not connected it will fail writing the data.
	// note if address cannot be found no error will be reported.
	// in short use carefully
	void setUserData(const uint8_t*, int16_t);
	void setUserDataByIndex(uint8_t, int16_t);
	int16_t getUserData(const uint8_t*);
	int16_t getUserDataByIndex(uint8_t);

#if REQUIRESNEW

	// initialize memory area
	void* operator new(unsigned int);

	// delete memory reference
	void operator delete(void*);

#endif

	void blockTillConversionComplete(uint8_t);
	void blockTillConversionComplete(uint8_t, unsigned long);
	void blockTillConversionComplete(uint8_t, request_t);

private:
	typedef uint8_t ScratchPad[9];

	// parasite power on or off
	bool parasite;

	// external pullup
	bool useExternalPullup;
	uint8_t pullupPin;

	// used to determine the delay amount needed to allow for the
	// temperature conversion to take place
	uint8_t bitResolution;

	// used to requestTemperature with or without delay
	bool waitForConversion;

	// used to requestTemperature to dynamically check if a conversion is complete
	bool checkForConversion;

	// used to determine if values will be saved from scratchpad to EEPROM on every scratchpad write
	bool autoSaveScratchPad;

	// count of devices on the bus
	uint8_t devices;

	// count of DS18xxx Family devices on bus
	uint8_t ds18Count;

	// Take a pointer to one wire instance
	OneWire* _wire;

	// reads scratchpad and returns the raw temperature
	raw_result_t calculateTemperature(const uint8_t*, uint8_t*);


	// Returns true if all bytes of scratchPad are '\0'
	bool isAllZeros(const uint8_t* const scratchPad, const size_t length = 9);

	// External pullup control
	void activateExternalPullup(void);
	void deactivateExternalPullup(void);

#if REQUIRESALARMS

	// required for alarmSearch
	uint8_t alarmSearchAddress[8];
	int8_t alarmSearchJunction;
	uint8_t alarmSearchExhausted;

	// the alarm handler function pointer
	AlarmHandler *_AlarmHandler;

#endif

};
#endif
