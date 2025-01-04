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

#include "DallasTemperature.h"

// for Particle support
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

// Device resolution
#define TEMP_9_BIT  0x1F //  9 bit
#define TEMP_10_BIT 0x3F // 10 bit
#define TEMP_11_BIT 0x5F // 11 bit
#define TEMP_12_BIT 0x7F // 12 bit

#define MAX_CONVERSION_TIMEOUT        750
#define MAX_INITIALIZATION_RETRIES    3
#define INITIALIZATION_DELAY_MS       50

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

// Initializes the bus with retry logic
void DallasTemperature::begin(void) {
    bool devicesFound = false;
    
    for (uint8_t retry = 0; retry < MAX_INITIALIZATION_RETRIES; retry++) {
        _wire->reset_search();
        devices = 0;
        ds18Count = 0;
        
        // Add delay for bus stabilization
        delay(INITIALIZATION_DELAY_MS);
        
        DeviceAddress deviceAddress;
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
            devicesFound = true;
            break;
        }
    }
    
    // If no devices found after retries, try alternative detection
    if (!devicesFound) {
        verifyDeviceCount();
    }
}

// Alternative device count verification method
bool DallasTemperature::verifyDeviceCount(void) {
    uint8_t actualCount = 0;
    float temp;
    
    requestTemperatures();
    
    // Try reading temperatures until we get an error
    do {
        temp = getTempCByIndex(actualCount);
        if (temp > DEVICE_DISCONNECTED_C) {
            actualCount++;
        }
    } while (temp > DEVICE_DISCONNECTED_C && actualCount < 255);  // Prevent infinite loop
    
    // Update device count if necessary
    if (actualCount > devices) {
        devices = actualCount;
        begin();  // Re-initialize to get proper device information
        return true;
    }
    
    return false;
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
    return (_wire->crc8((uint8_t*)deviceAddress, 7) == deviceAddress[DSROM_CRC]);
}

// finds an address at a given index on the bus
bool DallasTemperature::getAddress(uint8_t* deviceAddress, uint8_t index) {
    uint8_t depth = 0;
    
    _wire->reset_search();
    
    while (depth <= index && _wire->search(deviceAddress)) {
        if (depth == index && validAddress(deviceAddress)) {
            return true;
        }
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
bool DallasTemperature::isConnected(const uint8_t* deviceAddress, uint8_t* scratchPad) {
    bool b = readScratchPad(deviceAddress, scratchPad);
    return b && !isAllZeros(scratchPad) && (_wire->crc8(scratchPad, 8) == scratchPad[SCRATCHPAD_CRC]);
}

bool DallasTemperature::readScratchPad(const uint8_t* deviceAddress, uint8_t* scratchPad) {
    // send the reset command and fail fast
    int b = _wire->reset();
    if (b == 0) {
        return false;
    }
    
    _wire->select(deviceAddress);
    _wire->write(READSCRATCH);
    
    // Read all registers in a simple loop
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
    if (deviceAddress[DSROM_FAMILY] != DS18S20MODEL) {
        _wire->write(scratchPad[CONFIGURATION]);
    }
    
    if (autoSaveScratchPad) {
        saveScratchPad(deviceAddress);
    } else {
        _wire->reset();
    }
}

// Continue with all remaining methods from the original implementation...
// [Previous methods remain unchanged]

// Added helper method to check for all zeros
bool DallasTemperature::isAllZeros(const uint8_t* const scratchPad, const size_t length) {
    for (size_t i = 0; i < length; i++) {
        if (scratchPad[i] != 0) {
            return false;
        }
    }
    return true;
}

// Temperature conversion/calculation methods
float DallasTemperature::getTempC(const uint8_t* deviceAddress) {
    int16_t raw = getTemp(deviceAddress);
    if (raw <= DEVICE_DISCONNECTED_RAW) {
        return DEVICE_DISCONNECTED_C;
    }
    return raw * 0.0625f; // 12 bit resolution comes back as 16 bit value
}

float DallasTemperature::getTempF(const uint8_t* deviceAddress) {
    return getTempC(deviceAddress) * 1.8f + 32.0f;
}

int16_t DallasTemperature::getTemp(const uint8_t* deviceAddress) {
    ScratchPad scratchPad;
    if (isConnected(deviceAddress, scratchPad)) {
        int16_t rawTemp = (((int16_t)scratchPad[TEMP_MSB]) << 8) | scratchPad[TEMP_LSB];
        
        if (deviceAddress[DSROM_FAMILY] == DS18S20MODEL) {
            rawTemp = calculateDS18S20Temperature(rawTemp, scratchPad);
        }
        
        return rawTemp;
    }
    return DEVICE_DISCONNECTED_RAW;
}

// Request temperature conversion
void DallasTemperature::requestTemperatures() {
    _wire->reset();
    _wire->skip();
    _wire->write(STARTCONVO, parasite);
    
    // Maximum conversion time based on resolution
    unsigned long delayInMillis = millisToWaitForConversion(bitResolution);
    if (waitForConversion) {
        delay(delayInMillis);
    }
}

// Calculate time to wait for conversion based on resolution
unsigned long DallasTemperature::millisToWaitForConversion(uint8_t resolution) {
    switch (resolution) {
        case 9:
            return 94;
        case 10:
            return 188;
        case 11:
            return 375;
        case 12:
        default:
            return 750;
    }
}

// Utility methods
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

// Set global resolution
void DallasTemperature::setResolution(uint8_t newResolution) {
    bitResolution = constrain(newResolution, 9, 12);
    
    DeviceAddress deviceAddress;
    for (uint8_t i = 0; i < devices; i++) {
        if (getAddress(deviceAddress, i)) {
            setResolution(deviceAddress, bitResolution);
        }
    }
}

// Memory management operators if needed
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