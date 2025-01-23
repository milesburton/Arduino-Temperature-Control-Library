#include <DallasTemperature.h>
#include <ArduinoUnitTests.h>

unittest(test_initialization) {
    OneWire oneWire(2); // Simulate OneWire on pin 2
    DallasTemperature sensors(&oneWire);

    sensors.begin();
    assertEqual(0, sensors.getDeviceCount());
}

unittest(test_parasite_power_mode) {
    OneWire oneWire(2);
    DallasTemperature sensors(&oneWire);

    sensors.begin();
    assertFalse(sensors.isParasitePowerMode());
}

unittest_main()  