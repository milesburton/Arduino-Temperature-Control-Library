#pragma once

#include <Arduino.h>

// Platform-specific OneWire include
#if defined(STM32)
#include <OneWireSTM.h> // STM32-specific OneWire implementation
#else
#include <OneWire.h>    // Default OneWire implementation
#endif

// Define DeviceAddress globally for compatibility
typedef uint8_t DeviceAddress[8];

class DallasTemperature {
public:
    // Library version
    static constexpr const char* LIB_VERSION = "4.0.1";

    // Constructors
    explicit DallasTemperature(OneWire* oneWire);

    // Initialization
    void begin();

    // Device Management
    uint8_t getDeviceCount() const;
    bool getAddress(DeviceAddress deviceAddress, uint8_t index) const;
    bool isParasitePowerMode() const;

    // Scratchpad Operations
    bool readPowerSupply(const DeviceAddress deviceAddress = nullptr) const;

private:
    OneWire* oneWire = nullptr;
    bool parasitePowerMode = false;
    uint8_t deviceCount = 0;

    // Helper Methods
    void resetSearch() const;
};
