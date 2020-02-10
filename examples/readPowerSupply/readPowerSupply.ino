//
// FILE: readPowerSupply.ino
// AUTHOR: Rob Tillaart
// VERSION: 0.1.0
// PURPOSE: demo
// DATE: 2020-02-10
//
// Released to the public domain
//

// Include the libraries we need
#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// arrays to hold device addresses
DeviceAddress insideThermometer, outsideThermometer;
// Assign address manually. The addresses below will beed to be changed
// to valid device addresses on your bus. Device address can be retrieved
// by using either oneWire.search(deviceAddress) or individually via
// sensors.getAddress(deviceAddress, index)
// DeviceAddress insideThermometer = { 0x28, 0x1D, 0x39, 0x31, 0x2, 0x0, 0x0, 0xF0 };
// DeviceAddress outsideThermometer   = { 0x28, 0x3F, 0x1C, 0x31, 0x2, 0x0, 0x0, 0x2 };

int devCount = 0;

/*
 * The setup function. We only start the sensors here
 */
void setup(void)
{
  Serial.begin(115200);
  Serial.println("Arduino Temperature Control Library Demo - readPowerSupply");

  sensors.begin();

  devCount = sensors.getDeviceCount();
  Serial.print("#devices: ");
  Serial.println(devCount);

  // report parasite power requirements
  Serial.print("Parasite power is: ");
  if (sensors.readPowerSupply()) Serial.println("ON");  // no address means "scan all devices for parasite mode"
  else Serial.println("OFF");

  // Search for devices on the bus and assign based on an index.
  if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0");
  if (!sensors.getAddress(outsideThermometer, 1)) Serial.println("Unable to find address for Device 1");

  // show the addresses we found on the bus
  Serial.print("Device 0 Address: ");
  printAddress(insideThermometer);
  Serial.println();
  Serial.print("Power = parasite: ");
  Serial.println(sensors.readPowerSupply(insideThermometer));
  Serial.println();
  Serial.println();

  Serial.print("Device 1 Address: ");
  printAddress(outsideThermometer);
  Serial.println();
  Serial.print("Power = parasite: ");
  Serial.println(sensors.readPowerSupply(outsideThermometer));
  Serial.println();
  Serial.println();
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 0x10) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

// empty on purpose
void loop(void)
{
}

// END OF FILE