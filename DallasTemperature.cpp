#include "DallasTemperature.h"

// Constructors
DallasTemperature::DallasTemperature(OneWire* oneWire) : oneWire(oneWire) {}

// Initialization
void DallasTemperature::begin() {
    if (!oneWire) return;
    oneWire->reset_search();
    parasitePowerMode = false;
    deviceCount = 0;

    DeviceAddress address;
    while (oneWire->search(address)) {
        deviceCount++;
        if (readPowerSupply(address)) {
            parasitePowerMode = true;
        }
    }
    oneWire->reset_search();
}

// Get the total number of devices
uint8_t DallasTemperature::getDeviceCount() const {
    return deviceCount;
}

// Get the address of a device by index
bool DallasTemperature::getAddress(DeviceAddress deviceAddress, uint8_t index) const {
    if (!oneWire) return false;

    uint8_t count = 0;
    oneWire->reset_search();
    while (count <= index && oneWire->search(deviceAddress)) {
        if (count == index) return true;
        count++;
    }
    return false;
}

// Check if the bus is using parasite power
bool DallasTemperature::isParasitePowerMode() const {
    return parasitePowerMode;
}

// Read the power supply mode of a specific device or all devices
bool DallasTemperature::readPowerSupply(const DeviceAddress deviceAddress) const {
    if (!oneWire) return false;

    oneWire->reset();
    if (deviceAddress) {
        oneWire->select(deviceAddress);
    } else {
        oneWire->skip(); // Skip ROM command for all devices
    }
    oneWire->write(0xB4); // READ POWER SUPPLY command
    return (oneWire->read_bit() == 0);
}

// Reset search to start looking for devices again
void DallasTemperature::resetSearch() const {
    if (oneWire) oneWire->reset_search();
}
