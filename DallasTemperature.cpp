/*
MIT License

Copyright (c) 2024

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#if defined(PLATFORM_ID)  // Only defined if a Particle device
inline void yield() {
    Particle.process();
}
#elif ARDUINO >= 100
#include "Arduino.h"
#else
extern "C" {
#include "WConstants.h"
}
#endif

#include "DallasTemperature.h"

// OneWire commands
#define STARTCONVO      0x44  // Tells device to take a temperature reading
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

// Device resolution
#define TEMP_9_BIT  0x1F //  9 bit
#define TEMP_10_BIT 0x3F // 10 bit
#define TEMP_11_BIT 0x5F // 11 bit
#define TEMP_12_BIT 0x7F // 12 bit

// Constructors
DallasTemperature::DallasTemperature() {
#if REQUIRESALARMS
    setAlarmHandler(nullptr);
#endif
    useExternalPullup = false;
}

DallasTemperature::DallasTemperature(OneWire* _oneWire) : DallasTemperature() {
    setOneWire(_oneWire);
}

DallasTemperature::DallasTemperature(OneWire* _oneWire, uint8_t _pullupPin) : DallasTemperature(_oneWire) {
    setPullupPin(_pullupPin);
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

void DallasTemperature::setPullupPin(uint8_t _pullupPin) {
    useExternalPullup = true;
    pullupPin = _pullupPin;
    pinMode(pullupPin, OUTPUT);
    deactivateExternalPullup();
}

// Initialize the bus with retry logic
void DallasTemperature::begin(void) {
    DeviceAddress deviceAddress;
    
    for (uint8_t retry = 0; retry < MAX_INITIALIZATION_RETRIES; retry++) {
        _wire->reset_search();
        devices = 0;
        ds18Count = 0;
        
        // Add delay for bus stabilization
        delay(INITIALIZATION_DELAY_MS);
        
        while (_wire->search(deviceAddress)) {
            if (validAddress(deviceAddress)) {
                devices++;
                
                if (validFamily(deviceAddress)) {
                    ds18Count++;
                    
                    if (!parasite && readPowerSupply(deviceAddress)) {
                        parasite = true;
                    }
                    
                    uint8_t b = getResolution(deviceAddress);
                    if (b > bitResolution) {
                        bitResolution = b;
                    }
                }
            }
        }
        
        // If we found at least one device, exit retry loop
        if (devices > 0) {
            break;
        }
    }
}

// Device Information Methods
bool DallasTemperature::validFamily(const uint8_t* deviceAddress) {
    switch (deviceAddress[0]) {
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

bool DallasTemperature::validAddress(const uint8_t* deviceAddress) {
    return (_wire->crc8(const_cast<uint8_t*>(deviceAddress), 7) == deviceAddress[7]);
}

bool DallasTemperature::getAddress(uint8_t* deviceAddress, uint8_t index) {
    if (index < devices) {
        uint8_t depth = 0;
        
        _wire->reset_search();
        
        while (depth <= index && _wire->search(deviceAddress)) {
            if (depth == index && validAddress(deviceAddress)) {
                return true;
            }
            depth++;
        }
    }
    
    return false;
}

// Device Count Methods
uint8_t DallasTemperature::getDeviceCount(void) {
    return devices;
}

uint8_t DallasTemperature::getDS18Count(void) {
    return ds18Count;
}

// Alternative device count verification method
bool DallasTemperature::verifyDeviceCount(void) {
    uint8_t actualCount = 0;
    float temp;
    
    requestTemperatures();
    
    do {
        temp = getTempCByIndex(actualCount);
        if (temp > DEVICE_DISCONNECTED_C) {
            actualCount++;
        }
    } while (temp > DEVICE_DISCONNECTED_C && actualCount < 255);
    
    if (actualCount > devices) {
        devices = actualCount;
        begin();
        return true;
    }
    
    return false;
}

// Temperature reading with retry functionality
int32_t DallasTemperature::getTemp(const uint8_t* deviceAddress, byte retryCount) {
    ScratchPad scratchPad;
    byte retries = 0;
    
    while (retries++ <= retryCount) {
        if (isConnected(deviceAddress, scratchPad)) {
            return calculateTemperature(deviceAddress, scratchPad);
        }
    }
    
    return DEVICE_DISCONNECTED_RAW;
}

float DallasTemperature::getTempC(const uint8_t* deviceAddress, byte retryCount) {
    return rawToCelsius(getTemp(deviceAddress, retryCount));
}

float DallasTemperature::getTempF(const uint8_t* deviceAddress) {
    return rawToFahrenheit(getTemp(deviceAddress));
}

// Temperature request methods
request_t DallasTemperature::requestTemperatures(void) {
    request_t req = {};
    req.result = true;

    _wire->reset();
    _wire->skip();
    _wire->write(STARTCONVO, parasite);

    req.timestamp = millis();
    if (!waitForConversion) {
        return req;
    }
    
    blockTillConversionComplete(bitResolution, req.timestamp);
    return req;
}

request_t DallasTemperature::requestTemperaturesByAddress(const uint8_t* deviceAddress) {
    request_t req = {};
    uint8_t deviceBitResolution = getResolution(deviceAddress);
    
    if (deviceBitResolution == 0) {
        req.result = false;
        return req;
    }

    _wire->reset();
    _wire->select(deviceAddress);
    _wire->write(STARTCONVO, parasite);

    req.timestamp = millis();
    req.result = true;
    
    if (!waitForConversion) {
        return req;
    }

    blockTillConversionComplete(deviceBitResolution, req.timestamp);
    return req;
}

// Resolution control methods
void DallasTemperature::setResolution(uint8_t newResolution) {
    bitResolution = constrain(newResolution, 9, 12);
    
    DeviceAddress deviceAddress;
    _wire->reset_search();
    
    for (uint8_t i = 0; i < devices; i++) {
        if (_wire->search(deviceAddress) && validAddress(deviceAddress)) {
            setResolution(deviceAddress, bitResolution, true);
        }
    }
}

bool DallasTemperature::setResolution(const uint8_t* deviceAddress, uint8_t newResolution, bool skipGlobalBitResolutionCalculation) {
    bool success = false;
    
    // DS1820 and DS18S20 have no resolution configuration register
    if (deviceAddress[0] == DS18S20MODEL) {
        success = true;
    } else {
        newResolution = constrain(newResolution, 9, 12);
        uint8_t newValue = 0;
        ScratchPad scratchPad;

        if (isConnected(deviceAddress, scratchPad)) {
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

            if (scratchPad[CONFIGURATION] != newValue) {
                scratchPad[CONFIGURATION] = newValue;
                writeScratchPad(deviceAddress, scratchPad);
            }
            success = true;
        }
    }

    // Update global resolution if needed
    if (!skipGlobalBitResolutionCalculation && success) {
        bitResolution = newResolution;
        
        if (devices > 1) {
            DeviceAddress deviceAddr;
            _wire->reset_search();
            
            for (uint8_t i = 0; i < devices; i++) {
                if (bitResolution == 12) break;
                
                if (_wire->search(deviceAddr) && validAddress(deviceAddr)) {
                    uint8_t b = getResolution(deviceAddr);
                    if (b > bitResolution) bitResolution = b;
                }
            }
        }
    }

    return success;
}

// Utility methods
float DallasTemperature::toFahrenheit(float celsius) {
    return (celsius * 1.8f) + 32.0f;
}

float DallasTemperature::toCelsius(float fahrenheit) {
    return (fahrenheit - 32.0f) * 0.555555556f;
}

float DallasTemperature::rawToCelsius(int32_t raw) {
    if (raw <= DEVICE_DISCONNECTED_RAW) {
        return DEVICE_DISCONNECTED_C;
    }
    return (float)raw * 0.0078125f;  // 1/128
}

float DallasTemperature::rawToFahrenheit(int32_t raw) {
    if (raw <= DEVICE_DISCONNECTED_RAW) {
        return DEVICE_DISCONNECTED_F;
    }
    return (float)raw * 0.0140625f + 32.0f;  // 1/128*1.8 + 32
}

int16_t DallasTemperature::celsiusToRaw(float celsius) {
    return static_cast<int16_t>(celsius * 128.0f);
}

// Internal helper methods
bool DallasTemperature::isAllZeros(const uint8_t* const scratchPad, const size_t length) {
    for (size_t i = 0; i < length; i++) {
        if (scratchPad[i] != 0) {
            return false;
        }
    }
    return true;
}

void DallasTemperature::activateExternalPullup() {
    if (useExternalPullup) {
        digitalWrite(pullupPin, LOW);
    }
}

void DallasTemperature::deactivateExternalPullup() {
    if (useExternalPullup) {
        digitalWrite(pullupPin, HIGH);
    }
}

// Memory management if required
#if REQUIRESNEW
void* DallasTemperature::operator new(unsigned int size) {
    void* p = malloc(size);
    memset(p, 0, size);
    return p;
}

void DallasTemperature::operator delete(void* p) {
    free(p);
}
#endif