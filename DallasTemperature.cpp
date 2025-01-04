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

#define NO_ALARM_HANDLER ((DallasTemperature::AlarmHandler *)0)

// Constructors
DallasTemperature::DallasTemperature() {
    _wire = nullptr;
    devices = 0;
    ds18Count = 0;
    parasite = false;
    bitResolution = 9;
    waitForConversion = true;
    checkForConversion = true;
    autoSaveScratchPad = true;
    useExternalPullup = false;
#if REQUIRESALARMS
    setAlarmHandler(NO_ALARM_HANDLER);
    alarmSearchJunction = -1;
    alarmSearchExhausted = 0;
#endif
}

DallasTemperature::DallasTemperature(OneWire* _oneWire) : DallasTemperature() {
    setOneWire(_oneWire);
}

DallasTemperature::DallasTemperature(OneWire* _oneWire, uint8_t _pullupPin) : DallasTemperature(_oneWire) {
    setPullupPin(_pullupPin);
}

// Setup & Configuration
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

void DallasTemperature::begin(void) {
    DeviceAddress deviceAddress;
    
    for (uint8_t retry = 0; retry < MAX_INITIALIZATION_RETRIES; retry++) {
        _wire->reset_search();
        devices = 0;
        ds18Count = 0;
        
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
        
        if (devices > 0) break;
    }
}

// Device Information
uint8_t DallasTemperature::getDeviceCount(void) {
    return devices;
}

uint8_t DallasTemperature::getDS18Count(void) {
    return ds18Count;
}

bool DallasTemperature::validAddress(const uint8_t* deviceAddress) {
    return (_wire->crc8(const_cast<uint8_t*>(deviceAddress), 7) == deviceAddress[7]);
}

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

bool DallasTemperature::isConnected(const uint8_t* deviceAddress) {
    ScratchPad scratchPad;
    return isConnected(deviceAddress, scratchPad);
}

bool DallasTemperature::isConnected(const uint8_t* deviceAddress, uint8_t* scratchPad) {
    bool b = readScratchPad(deviceAddress, scratchPad);
    return b && !isAllZeros(scratchPad) && (_wire->crc8(scratchPad, 8) == scratchPad[SCRATCHPAD_CRC]);
}

// Scratchpad operations
bool DallasTemperature::readScratchPad(const uint8_t* deviceAddress, uint8_t* scratchPad) {
    int b = _wire->reset();
    if (b == 0) return false;
    
    _wire->select(deviceAddress);
    _wire->write(READSCRATCH);
    
    for (uint8_t i = 0; i < 9; i++) {
        scratchPad[i] = _wire->read();
    }
    
    b = _wire->reset();
    return (b == 1);
}

void DallasTemperature::writeScratchPad(const uint8_t* deviceAddress, const uint8_t* scratchPad) {
    _wire->reset();
    _wire->select(deviceAddress);
    _wire->write(WRITESCRATCH);
    _wire->write(scratchPad[HIGH_ALARM_TEMP]); // high alarm temp
    _wire->write(scratchPad[LOW_ALARM_TEMP]); // low alarm temp
    
    // DS1820 and DS18S20 have no configuration register
    if (deviceAddress[0] != DS18S20MODEL) {
        _wire->write(scratchPad[CONFIGURATION]);
    }
    
    if (autoSaveScratchPad) {
        saveScratchPad(deviceAddress);
    } else {
        _wire->reset();
    }
}

bool DallasTemperature::readPowerSupply(const uint8_t* deviceAddress) {
    bool parasiteMode = false;
    _wire->reset();
    if (deviceAddress == nullptr) {
        _wire->skip();
    } else {
        _wire->select(deviceAddress);
    }
    
    _wire->write(READPOWERSUPPLY);
    if (_wire->read_bit() == 0) {
        parasiteMode = true;
    }
    _wire->reset();
    return parasiteMode;
}

// Resolution operations
uint8_t DallasTemperature::getResolution() {
    return bitResolution;
}

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

uint8_t DallasTemperature::getResolution(const uint8_t* deviceAddress) {
    if (deviceAddress[0] == DS18S20MODEL) return 12;
    
    ScratchPad scratchPad;
    if (isConnected(deviceAddress, scratchPad)) {
        if (deviceAddress[0] == DS1825MODEL && scratchPad[CONFIGURATION] & 0x80) {
            return 12;
        }
        
        switch (scratchPad[CONFIGURATION]) {
            case TEMP_12_BIT: return 12;
            case TEMP_11_BIT: return 11;
            case TEMP_10_BIT: return 10;
            case TEMP_9_BIT: return 9;
        }
    }
    return 0;
}

bool DallasTemperature::setResolution(const uint8_t* deviceAddress, uint8_t newResolution, bool skipGlobalBitResolutionCalculation) {
    bool success = false;
    
    if (deviceAddress[0] == DS18S20MODEL) {
        success = true;
    } else {
        newResolution = constrain(newResolution, 9, 12);
        uint8_t newValue = 0;
        ScratchPad scratchPad;
        
        if (isConnected(deviceAddress, scratchPad)) {
            switch (newResolution) {
                case 12: newValue = TEMP_12_BIT; break;
                case 11: newValue = TEMP_11_BIT; break;
                case 10: newValue = TEMP_10_BIT; break;
                case 9:
                default: newValue = TEMP_9_BIT; break;
            }
            
            if (scratchPad[CONFIGURATION] != newValue) {
                scratchPad[CONFIGURATION] = newValue;
                writeScratchPad(deviceAddress, scratchPad);
            }
            success = true;
        }
    }
    
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

// Temperature operations
DallasTemperature::request_t DallasTemperature::requestTemperatures() {
    DallasTemperature::request_t req = {};
    req.result = true;
    
    _wire->reset();
    _wire->skip();
    _wire->write(STARTCONVO, parasite);
    
    req.timestamp = millis();
    if (!waitForConversion) return req;
    
    blockTillConversionComplete(bitResolution, req.timestamp);
    return req;
}

DallasTemperature::request_t DallasTemperature::requestTemperaturesByAddress(const uint8_t* deviceAddress) {
    DallasTemperature::request_t req = {};
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
    
    if (!waitForConversion) return req;
    
    blockTillConversionComplete(deviceBitResolution, req.timestamp);
    return req;
}

DallasTemperature::request_t DallasTemperature::requestTemperaturesByIndex(uint8_t index) {
    DeviceAddress deviceAddress;
    getAddress(deviceAddress, index);
    return requestTemperaturesByAddress(deviceAddress);
}

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

float DallasTemperature::getTempCByIndex(uint8_t index) {
    DeviceAddress deviceAddress;
    if (!getAddress(deviceAddress, index)) {
        return DEVICE_DISCONNECTED_C;
    }
    return getTempC(deviceAddress);
}

float DallasTemperature::getTempFByIndex(uint8_t index) {
    DeviceAddress deviceAddress;
    if (!getAddress(deviceAddress, index)) {
        return DEVICE_DISCONNECTED_F;
    }
    return getTempF(deviceAddress);
}

// Conversion operations
void DallasTemperature::setWaitForCon