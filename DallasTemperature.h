#pragma once

#include <array>
#include <cstdint>
#include <Arduino.h>
#ifdef __STM32F1__
#include <OneWireSTM.h>
#else
#include <OneWire.h>
#endif

namespace Dallas {

class DallasTemperature {
public:
    static constexpr const char* LIB_VERSION = "4.0.0";

    using DeviceAddress = std::array<uint8_t, 8>;
    using ScratchPad = std::array<uint8_t, 9>;

    struct Request {
        bool result;
        unsigned long timestamp;
        operator bool() const { return result; }
    };

    // Constructors
    DallasTemperature();
    explicit DallasTemperature(OneWire* oneWire);
    DallasTemperature(OneWire* oneWire, uint8_t pullupPin);

    // Configuration
    void begin();
    void setOneWire(OneWire* oneWire);
    void setPullupPin(uint8_t pullupPin);

    // Device Management
    uint8_t getDeviceCount() const;
    uint8_t getDS18Count() const;
    bool isConnected(const DeviceAddress& address) const;
    bool isConnected(const DeviceAddress& address, ScratchPad& scratchPad) const;

    // Temperature Reading
    Request requestTemperatures();
    Request requestTemperaturesByAddress(const DeviceAddress& address);
    Request requestTemperaturesByIndex(uint8_t index);
    float getTempC(const DeviceAddress& address, uint8_t retryCount = 0) const;
    float getTempF(const DeviceAddress& address, uint8_t retryCount = 0) const;

    // Utility Methods
    static float toFahrenheit(float celsius);
    static float toCelsius(float fahrenheit);
    static float rawToCelsius(int32_t raw);
    static float rawToFahrenheit(int32_t raw);

private:
    bool parasitePowerMode = false;
    uint8_t bitResolution = 9;
    OneWire* oneWire = nullptr;
    uint8_t pullupPin = 0;

    bool readScratchPad(const DeviceAddress& address, ScratchPad& scratchPad) const;
    bool readPowerSupply(const DeviceAddress& address) const;
    void activateExternalPullup() const;
    void deactivateExternalPullup() const;

    int32_t calculateTemperature(const DeviceAddress& address, const ScratchPad& scratchPad) const;
};

} // namespace Dallas
