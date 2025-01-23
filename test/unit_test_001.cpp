//    FILE: unit_test_001.cpp
//  AUTHOR: Miles Burton / Rob Tillaart
//    DATE: 2021-01-10
// PURPOSE: unit tests for the Arduino-Temperature-Control-Library
//          https://github.com/MilesBurton/Arduino-Temperature-Control-Library


#include <ArduinoUnitTests.h>
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Mock pin for testing
#define ONE_WIRE_BUS 2

unittest_setup() {
    fprintf(stderr, "VERSION: %s\n", DALLASTEMPLIBVERSION);
}

unittest_teardown() {
    fprintf(stderr, "\n");
}

// Test constants defined in the library
unittest(test_models) {
    assertEqual(0x10, DS18S20MODEL);
    assertEqual(0x28, DS18B20MODEL);
    assertEqual(0x22, DS1822MODEL);
    assertEqual(0x3B, DS1825MODEL);
    assertEqual(0x42, DS28EA00MODEL);
}

// Test error codes defined in the library
unittest(test_error_code) {
    assertEqual(DEVICE_DISCONNECTED_C, -127);
    assertEqual(DEVICE_DISCONNECTED_F, -196.6);
    assertEqual(DEVICE_DISCONNECTED_RAW, -7040);

    assertEqual(DEVICE_FAULT_OPEN_C, -254);
    assertEqualFloat(DEVICE_FAULT_OPEN_F, -425.2, 0.1);
    assertEqual(DEVICE_FAULT_OPEN_RAW, -32512);

    assertEqual(DEVICE_FAULT_SHORTGND_C, -253);
    assertEqualFloat(DEVICE_FAULT_SHORTGND_F, -423.4, 0.1);
    assertEqual(DEVICE_FAULT_SHORTGND_RAW, -32384);

    assertEqual(DEVICE_FAULT_SHORTVDD_C, -252);
    assertEqualFloat(DEVICE_FAULT_SHORTVDD_F, -421.6, 0.1);
    assertEqual(DEVICE_FAULT_SHORTVDD_RAW, -32256);
}

// Test basic initialization and functionality of the DallasTemperature library
unittest(test_initialization) {
    OneWire oneWire(ONE_WIRE_BUS);
    DallasTemperature sensors(&oneWire);

    sensors.begin();

    // Initially, there should be no devices detected
    assertEqual(0, sensors.getDeviceCount());
    assertFalse(sensors.isParasitePowerMode());
}

// Simulate a basic temperature read (mocked)
unittest(test_temperature_read) {
    OneWire oneWire(ONE_WIRE_BUS);
    DallasTemperature sensors(&oneWire);

    sensors.begin();

    // Mock reading temperature
    float tempC = sensors.getTempCByIndex(0);
    assertEqual(DEVICE_DISCONNECTED_C, tempC); // Simulated no device connected
}

unittest_main()