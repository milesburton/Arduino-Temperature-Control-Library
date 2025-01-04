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

// Returns the number of devices found on the bus
uint8_t DallasTemperature::getDeviceCount(void) {
    return devices;
}

uint8_t DallasTemperature::getDS18Count(void) {
    return ds18Count;
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

bool DallasTemperature::validAddress(const uint8_t* deviceAddress) {
    return (_wire->crc8(const_cast<uint8_t*>(deviceAddress), 7) == deviceAddress[7]);
}

bool DallasTemperature::getAddress(uint8_t* deviceAddress, uint8_t index) {
    if (index < devices) {
        uint8_t depth = 0;
        
        _wire->reset_