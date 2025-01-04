#include "DallasTemperature.h"

#if ARDUINO >= 100
#include "Arduino.h"
#else
extern "C" {
#include "WConstants.h"
}
#endif

// OneWire commands
#define STARTCONVO      0x44
#define COPYSCRATCH     0x48
#define READSCRATCH     0xBE
#define WRITESCRATCH    0x4E
#define RECALLSCRATCH   0xB8
#define READPOWERSUPPLY 0xB4
#define ALARMSEARCH     0xEC

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
#define TEMP_9_BIT  0x1F
#define TEMP_10_BIT 0x3F
#define TEMP_11_BIT 0x5F
#define TEMP_12_BIT 0x7F

#define NO_ALARM_HANDLER ((AlarmHandler *)0)

// ============================ Initialization and Configuration ============================
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

// ============================ Address and Device Validation ============================
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

uint8_t DallasTemperature::getDeviceCount(void) {
    return devices;
}

uint8_t DallasTemperature::getDS18Count(void) {
    return ds18Count;
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

// ============================ Scratchpad Management ============================
bool DallasTemperature::saveScratchPad(const uint8_t* deviceAddress) {
    if (!resetAndSelect(deviceAddress)) return false;

    _wire->write(COPYSCRATCH, parasite);
    delayForNVWriteCycle();
    return _wire->reset() == 1;
}

bool DallasTemperature::saveScratchPadByIndex(uint8_t index) {
    DeviceAddress deviceAddress;
    if (!getAddress(deviceAddress, index)) return false;
    return saveScratchPad(deviceAddress);
}

void DallasTemperature::writeScratchPad(const uint8_t* deviceAddress, const uint8_t* scratchPad) {
    if (!resetAndSelect(deviceAddress)) return;

    _wire->write(WRITESCRATCH);
    _wire->write(scratchPad[HIGH_ALARM_TEMP]);
    _wire->write(scratchPad[LOW_ALARM_TEMP]);

    if (deviceAddress[0] != DS18S20MODEL) {
        _wire->write(scratchPad[CONFIGURATION]);
    }

    if (autoSaveScratchPad) {
        saveScratchPad(deviceAddress);
    } else {
        _wire->reset();
    }
}

bool DallasTemperature::readScratchPad(const uint8_t* deviceAddress, uint8_t* scratchPad) {
    if (!resetAndSelect(deviceAddress)) return false;

    _wire->write(READSCRATCH);
    for (uint8_t i = 0; i < 9; i++) {
        scratchPad[i] = _wire->read();
    }

    return _wire->reset() == 1;
}

// ============================ Temperature Operations ============================
DallasTemperature::request_t DallasTemperature::requestTemperatures() {
    request_t req = {};
    req.result = true;

    _wire->reset();
    _wire->skip();
    _wire->write(STARTCONVO, parasite);

    req.timestamp = millis();
    if (waitForConversion) blockTillConversionComplete(bitResolution, req.timestamp);
    return req;
}

DallasTemperature::request_t DallasTemperature::requestTemperaturesByIndex(uint8_t index) {
    DeviceAddress deviceAddress;
    if (!getAddress(deviceAddress, index)) {
        request_t req = { .result = false };
        return req;
    }
    return requestTemperaturesByAddress(deviceAddress);
}

// ============================ Utilities ============================
bool DallasTemperature::resetAndSelect(const uint8_t* deviceAddress) {
    if (_wire->reset() == 0) return false;
    if (deviceAddress) _wire->select(deviceAddress);
    else _wire->skip();
    return true;
}

void DallasTemperature::delayForNVWriteCycle() {
    if (parasite) {
        activateExternalPullup();
        delay(20);
        deactivateExternalPullup();
    } else {
        delay(20);
    }
}

uint16_t DallasTemperature::millisToWaitForConversion(uint8_t bitResolution) {
    switch (bitResolution) {
        case 9:  return 94;
        case 10: return 188;
        case 11: return 375;
        default: return 750;
    }
}

// ============================ Temperature Conversions ============================
float DallasTemperature::toCelsius(float fahrenheit) {
    return (fahrenheit - 32.0f) * 0.555555556f;
}

float DallasTemperature::toFahrenheit(float celsius) {
    return (celsius * 1.8f) + 32.0f;
}

float DallasTemperature::rawToCelsius(int32_t raw) {
    if (raw <= DEVICE_DISCONNECTED_RAW) return DEVICE_DISCONNECTED_C;
    return (float)raw * 0.0078125f;
}

float DallasTemperature::rawToFahrenheit(int32_t raw) {
    if (raw <= DEVICE_DISCONNECTED_RAW) return DEVICE_DISCONNECTED_F;
    return rawToCelsius(raw) * 1.8f + 32.0f;
}

// ============================ Helper Functions ============================
int32_t DallasTemperature::calculateTemperature(const uint8_t* deviceAddress, uint8_t* scratchPad) {
    int32_t fpTemperature = 0;

    int32_t neg = 0x0;
    if (scratchPad[TEMP_MSB] & 0x80) neg = 0xFFF80000;

    if (deviceAddress[0] == DS1825MODEL && scratchPad[CONFIGURATION] & 0x80) {
        if (scratchPad[TEMP_LSB] & 1) {
            if (scratchPad[HIGH_ALARM_TEMP] & 1) {
                return DEVICE_FAULT_OPEN_RAW;
            } else if (scratchPad[HIGH_ALARM_TEMP] >> 1 & 1) {
                return DEVICE_FAULT_SHORTGND_RAW;
            } else if (scratchPad[HIGH_ALARM_TEMP] >> 2 & 1) {
                return DEVICE_FAULT_SHORTVDD_RAW;
            } else {
                return DEVICE_DISCONNECTED_RAW;
            }
        }
        fpTemperature = (((int32_t)scratchPad[TEMP_MSB]) << 11) |
                        (((int32_t)scratchPad[TEMP_LSB] & 0xFC) << 3) |
                        neg;
    } else {
        fpTemperature = (((int16_t)scratchPad[TEMP_MSB]) << 11) |
                        (((int16_t)scratchPad[TEMP_LSB]) << 3) |
                        neg;
    }

    return fpTemperature;
}
