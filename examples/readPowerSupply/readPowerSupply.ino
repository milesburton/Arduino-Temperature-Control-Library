#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// Arrays to hold device addresses
DeviceAddress insideThermometer, outsideThermometer;

void setup() {
    Serial.begin(115200);
    Serial.println("Arduino Temperature Control Library Demo - readPowerSupply");

    sensors.begin();

    // Count devices
    int deviceCount = sensors.getDeviceCount();
    Serial.print("Device count: ");
    Serial.println(deviceCount);

    // Check parasite power
    Serial.print("Parasite power is: ");
    if (sensors.isParasitePowerMode()) Serial.println("ON");
    else Serial.println("OFF");

    // Get device addresses
    if (!sensors.getAddress(insideThermometer, 0)) {
        Serial.println("Unable to find address for Device 0");
    }
    if (!sensors.getAddress(outsideThermometer, 1)) {
        Serial.println("Unable to find address for Device 1");
    }

    // Print addresses
    Serial.print("Device 0 Address: ");
    printAddress(insideThermometer);
    Serial.println();
    Serial.print("Power = parasite: ");
    Serial.println(sensors.readPowerSupply(insideThermometer));

    Serial.print("Device 1 Address: ");
    printAddress(outsideThermometer);
    Serial.println();
    Serial.print("Power = parasite: ");
    Serial.println(sensors.readPowerSupply(outsideThermometer));
}

void loop() {
    // Empty
}

void printAddress(DeviceAddress deviceAddress) {
    for (uint8_t i = 0; i < 8; i++) {
        if (deviceAddress[i] < 0x10) Serial.print("0");
        Serial.print(deviceAddress[i], HEX);
    }
}
