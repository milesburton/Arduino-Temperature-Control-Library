#include "DallasTemperature.h"

namespace Dallas {

// Constructors
DallasTemperature::DallasTemperature() = default;

DallasTemperature::DallasTemperature(OneWire* oneWire) : oneWire(oneWire) {}

DallasTemperature::DallasTemperature(OneWire* oneWire, uint8_t pullupPin)
    : oneWire(oneWire), pullupPin(pullupPin) {}

// Configuration
void DallasTemperature::begin() {
    if (!oneWire) return;
    oneWire->reset_search();
    parasitePowerMode = false;
    bitResolution = 9;
}

void DallasTemperature::setOneWire(OneWire* oneWire) {
    this->oneWire = oneWire;
}

void DallasTemperature::setPullupPin(uint8_t pullupPin) {
    this->pullupPin = pullupPin;
    pinMode(pullupPin, OUTPUT);
    deactivateExternalPullup();
}

// Device Management
uint8_t DallasTemperature::getDeviceCount() const {
    uint8_t count = 0;
    DeviceAddress address;
    oneWire->reset_search();
    while (oneWire->search(address.data())) {
        count++;
    }
    return count;
}

uint8_t DallasTemperature::getDS18Count() const {
    uint8_t count = 0;
    DeviceAddress address;
    oneWire->reset_search();
    while (oneWire->search(address.data())) {
        if (address[0] == 0x28) { // Match DS18B20 family code
            count++;
        }
    }
    return count;
}

bool DallasTemperature::isConnected(const DeviceAddress& address) const {
    ScratchPad scratchPad;
    return isConnected(address, scratchPad);
}

bool DallasTemperature::isConnected(const DeviceAddress& address, ScratchPad& scratchPad) const {
    return readScratchPad(address, scratchPad);
}

// Temperature Reading
DallasTemperature::Request DallasTemperature::requestTemperatures() {
    Request req = {true, millis()};
    oneWire->reset();
    oneWire->skip();
    oneWire->write(0x44, parasitePowerMode); // Start temperature conversion
    delay(750); // Wait for conversion
    return req;
}

float DallasTemperature::getTempC(const DeviceAddress& address, uint8_t retryCount) const {
    ScratchPad scratchPad;
    if (!isConnected(address, scratchPad)) {
        return -127.0f; // Disconnected
    }
    return rawToCelsius(calculateTemperature(address, scratchPad));
}

float DallasTemperature::getTempF(const DeviceAddress& address, uint8_t retryCount) const {
    return toFahrenheit(getTempC(address, retryCount));
}

// Utility Methods
float DallasTemperature::toFahrenheit(float celsius) {
    return (celsius * 1.8f) + 32.0f;
}

float DallasTemperature::toCelsius(float fahrenheit) {
    return (fahrenheit - 32.0f) * 0.555555556f;
}

float DallasTemperature::rawToCelsius(int32_t raw) {
    return raw * 0.0078125f;
}

float DallasTemperature::rawToFahrenheit(int32_t raw) {
    return toFahrenheit(rawToCelsius(raw));
}

// Internal Methods
bool DallasTemperature::readScratchPad(const DeviceAddress& address, ScratchPad& scratchPad) const {
    oneWire->reset();
    oneWire->select(address.data());
    oneWire->write(0xBE); // Read Scratchpad
    for (auto& byte : scratchPad) {
        byte = oneWire->read();
    }
    return true;
}

bool DallasTemperature::readPowerSupply(const DeviceAddress& address) const {
    oneWire->reset();
    oneWire->select(address.data());
    oneWire->write(0xB4); // Read Power Supply
    return oneWire->read_bit();
}

void DallasTemperature::activateExternalPullup() const {
    if (pullupPin) {
        digitalWrite(pullupPin, LOW);
    }
}

void DallasTemperature::deactivateExternalPullup() const {
    if (pullupPin) {
        digitalWrite(pullupPin, HIGH);
    }
}

int32_t DallasTemperature::calculateTemperature(const DeviceAddress& address, const ScratchPad& scratchPad) const {
    int32_t rawTemp = ((static_cast<int16_t>(scratchPad[1]) << 8) | scratchPad[0]);
    return rawTemp;
}

} // namespace Dallas
