#include "DallasTemperature.h"

#if ARDUINO >= 100
#include "Arduino.h"
#else
extern "C" {
#include "WConstants.h"
}
#endif

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
#define TEMP_9_BIT  0x1F
#define TEMP_10_BIT 0x3F
#define TEMP_11_BIT 0x5F
#define TEMP_12_BIT 0x7F

#define NO_ALARM_HANDLER ((AlarmHandler *)0)

// DSROM FIELDS
#define DSROM_FAMILY    0
#define DSROM_CRC       7

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

void DallasTemperature::activateExternalPullup() {
    if (useExternalPullup) digitalWrite(pullupPin, LOW);
}

void DallasTemperature::deactivateExternalPullup() {
    if (useExternalPullup) digitalWrite(pullupPin, HIGH);
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

uint8_t DallasTemperature::getDeviceCount(void) {
    return devices;
}

uint8_t DallasTemperature::getDS18Count(void) {
    return ds18Count;
}

bool DallasTemperature::isConnected(const uint8_t* deviceAddress) {
    ScratchPad scratchPad;
    return isConnected(deviceAddress, scratchPad);
}

bool DallasTemperature::isConnected(const uint8_t* deviceAddress, uint8_t* scratchPad) {
    bool b = readScratchPad(deviceAddress, scratchPad);
    return b && !isAllZeros(scratchPad) && (_wire->crc8(scratchPad, 8) == scratchPad[SCRATCHPAD_CRC]);
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

bool DallasTemperature::isParasitePowerMode(void) {
    return parasite;
}

bool DallasTemperature::isAllZeros(const uint8_t* const scratchPad, const size_t length) {
    for (size_t i = 0; i < length; i++) {
        if (scratchPad[i] != 0) return false;
    }
    return true;
}

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

bool DallasTemperature::saveScratchPad(const uint8_t* deviceAddress) {
    if (_wire->reset() == 0) return false;
    
    if (deviceAddress == nullptr)
        _wire->skip();
    else
        _wire->select(deviceAddress);
    
    _wire->write(COPYSCRATCH, parasite);
    
    // Specification: NV Write Cycle Time is typically 2ms, max 10ms
    // Waiting 20ms to allow for sensors that take longer in practice
    if (!parasite) {
        delay(20);
    } else {
        activateExternalPullup();
        delay(20);
        deactivateExternalPullup();
    }
    
    return (_wire->reset() == 1);
}

bool DallasTemperature::recallScratchPad(const uint8_t* deviceAddress) {
    if (_wire->reset() == 0) return false;
    
    if (deviceAddress == nullptr)
        _wire->skip();
    else
        _wire->select(deviceAddress);
    
    _wire->write(RECALLSCRATCH, parasite);
    
    // Specification: Strong pullup only needed when writing to EEPROM
    unsigned long start = millis();
    while (_wire->read_bit() == 0) {
        if (millis() - start > 20) return false;
        yield();
    }
    
    return (_wire->reset() == 1);
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
    return getTempC((uint8_t*)deviceAddress);
}

float DallasTemperature::getTempFByIndex(uint8_t index) {
    DeviceAddress deviceAddress;
    if (!getAddress(deviceAddress, index)) {
        return DEVICE_DISCONNECTED_F;
    }
    return getTempF((uint8_t*)deviceAddress);
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

uint8_t DallasTemperature::getResolution() {
    return bitResolution;
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

float DallasTemperature::toFahrenheit(float celsius) {
    return (celsius * 1.8f) + 32.0f;
}

float DallasTemperature::toCelsius(float fahrenheit) {
    return (fahrenheit - 32.0f) * 0.555555556f;
}

float DallasTemperature::rawToCelsius(int32_t raw) {
    if (raw <= DEVICE_DISCONNECTED_RAW)
        return DEVICE_DISCONNECTED_C;
    return (float)raw * 0.0078125f;  // 1/128
}

float DallasTemperature::rawToFahrenheit(int32_t raw) {
    if (raw <= DEVICE_DISCONNECTED_RAW)
        return DEVICE_DISCONNECTED_F;
    return rawToCelsius(raw) * 1.8f + 32.0f;
}

int16_t DallasTemperature::celsiusToRaw(float celsius) {
    return static_cast<int16_t>(celsius * 128.0f);
}

uint16_t DallasTemperature::millisToWaitForConversion(uint8_t bitResolution) {
    switch (bitResolution) {
        case 9:  return 94;
        case 10: return 188;
        case 11: return 375;
        default: return 750;
    }
}

uint16_t DallasTemperature::millisToWaitForConversion() {
    return millisToWaitForConversion(bitResolution);
}

void DallasTemperature::setWaitForConversion(bool flag) {
    waitForConversion = flag;
}

bool DallasTemperature::getWaitForConversion() {
    return waitForConversion;
}

void DallasTemperature::setCheckForConversion(bool flag) {
    checkForConversion = flag;
}

bool DallasTemperature::getCheckForConversion() {
    return checkForConversion;
}

bool DallasTemperature::isConversionComplete() {
    uint8_t b = _wire->read_bit();
    return (b == 1);
}

void DallasTemperature::setAutoSaveScratchPad(bool flag) {
    autoSaveScratchPad = flag;
}

bool DallasTemperature::getAutoSaveScratchPad() {
    return autoSaveScratchPad;
}

DallasTemperature::request_t DallasTemperature::requestTemperatures() {
    request_t req = {};
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
    
    if (!waitForConversion) return req;
    
    blockTillConversionComplete(deviceBitResolution, req.timestamp);
    return req;
}

DallasTemperature::request_t DallasTemperature::requestTemperaturesByIndex(uint8_t index) {
    DeviceAddress deviceAddress;
    getAddress(deviceAddress, index);
    return requestTemperaturesByAddress(deviceAddress);
}

void DallasTemperature::blockTillConversionComplete(uint8_t bitResolution) {
    unsigned long start = millis();
    blockTillConversionComplete(bitResolution, start);
}

void DallasTemperature::blockTillConversionComplete(uint8_t bitResolution, unsigned long start) {
    if (checkForConversion && !parasite) {
        while (!isConversionComplete() && ((unsigned long)(millis() - start) < (unsigned long)MAX_CONVERSION_TIMEOUT)) {
            yield();
        }
    } else {
        unsigned long delayInMillis = millisToWaitForConversion(bitResolution);
        activateExternalPullup();
        delay(delayInMillis);
        deactivateExternalPullup();
    }
}

void DallasTemperature::blockTillConversionComplete(uint8_t bitResolution, request_t req) {
    if (req.result) {
        blockTillConversionComplete(bitResolution, req.timestamp);
    }
}

int32_t DallasTemperature::calculateTemperature(const uint8_t* deviceAddress, uint8_t* scratchPad) {
    int32_t fpTemperature = 0;

    // looking thru the spec sheets of all supported devices, bit 15 is always the signing bit
    int32_t neg = 0x0;
    if (scratchPad[TEMP_MSB] & 0x80)
        neg = 0xFFF80000;

    // detect MAX31850
    if (deviceAddress[0] == DS1825MODEL && scratchPad[CONFIGURATION] & 0x80) {
        if (scratchPad[TEMP_LSB] & 1) { // Fault Detected
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
        // We must mask out bit 1 (reserved) and 0 (fault) on TEMP_LSB
        fpTemperature = (((int32_t)scratchPad[TEMP_MSB]) << 11)
                       | (((int32_t)scratchPad[TEMP_LSB] & 0xFC) << 3)
                       | neg;
    } else {
        fpTemperature = (((int16_t)scratchPad[TEMP_MSB]) << 11)
                       | (((int16_t)scratchPad[TEMP_LSB]) << 3)
                       | neg;
    }

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
    fpTemperature = (((fpTemperature & 0xfff0) << 3) - 32
                    + (((scratchPad[COUNT_PER_C] - scratchPad[COUNT_REMAIN]) << 7)
                       / scratchPad[COUNT_PER_C])) | neg;
  }

    return fpTemperature;
}

#if REQUIRESALARMS

void DallasTemperature::setAlarmHandler(const AlarmHandler* handler) {
    _AlarmHandler = handler;
}

void DallasTemperature::setHighAlarmTemp(const uint8_t* deviceAddress, int8_t celsius) {
    // make sure the alarm temperature is within the device's range
    if (celsius > 125) celsius = 125;
    else if (celsius < -55) celsius = -55;

    ScratchPad scratchPad;
    if (isConnected(deviceAddress, scratchPad)) {
        scratchPad[HIGH_ALARM_TEMP] = (uint8_t)celsius;
        writeScratchPad(deviceAddress, scratchPad);
    }
}

void DallasTemperature::setLowAlarmTemp(const uint8_t* deviceAddress, int8_t celsius) {
    // make sure the alarm temperature is within the device's range
    if (celsius > 125) celsius = 125;
    else if (celsius < -55) celsius = -55;

    ScratchPad scratchPad;
    if (isConnected(deviceAddress, scratchPad)) {
        scratchPad[LOW_ALARM_TEMP] = (uint8_t)celsius;
        writeScratchPad(deviceAddress, scratchPad);
    }
}

int8_t DallasTemperature::getHighAlarmTemp(const uint8_t* deviceAddress) {
    ScratchPad scratchPad;
    if (isConnected(deviceAddress, scratchPad))
        return (int8_t)scratchPad[HIGH_ALARM_TEMP];
    return DEVICE_DISCONNECTED_C;
}

int8_t DallasTemperature::getLowAlarmTemp(const uint8_t* deviceAddress) {
    ScratchPad scratchPad;
    if (isConnected(deviceAddress, scratchPad))
        return (int8_t)scratchPad[LOW_ALARM_TEMP];
    return DEVICE_DISCONNECTED_C;
}

void DallasTemperature::resetAlarmSearch() {
    alarmSearchJunction = -1;
    alarmSearchExhausted = 0;
    for (uint8_t i = 0; i < 7; i++) {
        alarmSearchAddress[i] = 0;
    }
}

bool DallasTemperature::alarmSearch(uint8_t* newAddr) {
    uint8_t i;
    int8_t lastJunction = -1;
    uint8_t done = 1;

    if (alarmSearchExhausted)
        return false;

    if (!_wire->reset())
        return false;

    _wire->write(ALARMSEARCH);

    for (i = 0; i < 64; i++) {
        uint8_t a = _wire->read_bit();
        uint8_t nota = _wire->read_bit();
        uint8_t ibyte = i / 8;
        uint8_t ibit = 1 << (i & 7);

        if (a && nota)
            return false;

        if (!a && !nota) {
            if (i == alarmSearchJunction) {
                a = 1;
                alarmSearchJunction = lastJunction;
            } else if (i < alarmSearchJunction) {
                if (alarmSearchAddress[ibyte] & ibit) {
                    a = 1;
                } else {
                    a = 0;
                    done = 0;
                    lastJunction = i;
                }
            } else {
                a = 0;
                alarmSearchJunction = i;
                done = 0;
            }
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

bool DallasTemperature::hasAlarm(const uint8_t* deviceAddress) {
    ScratchPad scratchPad;
    if (isConnected(deviceAddress, scratchPad)) {
        int8_t temp = calculateTemperature(deviceAddress, scratchPad) >> 7;
        return (temp <= (int8_t)scratchPad[LOW_ALARM_TEMP] || 
                temp >= (int8_t)scratchPad[HIGH_ALARM_TEMP]);
    }
    return false;
}

bool DallasTemperature::hasAlarm(void) {
    DeviceAddress deviceAddress;
    resetAlarmSearch();
    return alarmSearch(deviceAddress);
}

void DallasTemperature::processAlarms(void) {
    if (!hasAlarmHandler())
        return;

    resetAlarmSearch();
    DeviceAddress alarmAddr;

    while (alarmSearch(alarmAddr)) {
        if (validAddress(alarmAddr)) {
            _AlarmHandler(alarmAddr);
        }
    }
}

bool DallasTemperature::hasAlarmHandler() {
    return (_AlarmHandler != NO_ALARM_HANDLER);
}

#endif

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

void DallasTemperature::setUserData(const uint8_t* deviceAddress, int16_t data) {
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

void DallasTemperature::setUserDataByIndex(uint8_t deviceIndex, int16_t data) {
    DeviceAddress deviceAddress;
    if (getAddress(deviceAddress, deviceIndex)) {
        setUserData((uint8_t*)deviceAddress, data);
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

int16_t DallasTemperature::getUserDataByIndex(uint8_t deviceIndex) {
    DeviceAddress deviceAddress;
    getAddress(deviceAddress, deviceIndex);
    return getUserData((uint8_t*)deviceAddress);
}