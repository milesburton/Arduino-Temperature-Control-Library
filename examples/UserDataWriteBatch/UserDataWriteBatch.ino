//
// FILE: UserDataWriteBatch.ino
// AUTHOR: Rob Tillaart
// VERSION: 0.1.0
// PURPOSE: use of alarm field as user identification demo
// DATE: 2019-12-23
// URL:
//
// Released to the public domain
//

#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS      2

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

uint8_t deviceCount = 0;

void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}



void setup(void)
{
  Serial.begin(115200);
  Serial.println(__FILE__);
  Serial.println("Write user ID to DS18B20\n");

  sensors.begin();

  // count devices
  deviceCount = sensors.getDeviceCount();
  Serial.print("#devices: ");
  Serial.println(deviceCount);

  Serial.println();
  Serial.println("current ID's");
  for (uint8_t index = 0; index < deviceCount; index++)
  {
    DeviceAddress t;
    sensors.getAddress(t, index);
    printAddress(t);
    Serial.print("\t\tID: ");
    int id = sensors.getUserData(t);
    Serial.println(id);
  }

  Serial.println();
  Serial.print("Enter ID for batch: ");
  int c = 0;
  int id = 0;
  while (c != '\n' && c != '\r')
  {
    c = Serial.read();
    switch (c)
    {
    case '0'...'9':
      id *= 10;
      id += (c - '0');
      break;
    default:
      break;
    }
  }
  Serial.println();
  Serial.println(id);
  Serial.println();

  Serial.println("Start labeling ...");
  for (uint8_t index = 0; index < deviceCount; index++)
  {
    Serial.print(".");
    DeviceAddress t;
    sensors.getAddress(t, index);
    sensors.setUserData(t, id);
  }
  Serial.println();

  Serial.println();
  Serial.println("Show results ...");
  for (uint8_t index = 0; index < deviceCount; index++)
  {
    DeviceAddress t;
    sensors.getAddress(t, index);
    printAddress(t);
    Serial.print("\t\tID: ");
    int id = sensors.getUserData(t);
    Serial.println(id);
  }
  Serial.println("Done ...");

}

void loop(void) {}

// END OF FILE