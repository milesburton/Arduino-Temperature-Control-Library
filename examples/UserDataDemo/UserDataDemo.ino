//
// FILE: UserDataDemo.ino
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

// Add 4 prepared sensors to the bus
// use the UserDataWriteBatch demo to prepare 4 different labeled sensors
struct
{
  int id;
  DeviceAddress addr;
} T[4];

float getTempByID(int id)
{
  for (uint8_t index = 0; index < deviceCount; index++)
  {
    if (T[index].id == id)
    {
      return sensors.getTempC(T[index].addr);
    }
  }
  return -999;
}

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
  Serial.println("Dallas Temperature Demo");

  sensors.begin();
  
  // count devices
  deviceCount = sensors.getDeviceCount();
  Serial.print("#devices: ");
  Serial.println(deviceCount);

  // Read ID's per sensor
  // and put them in T array
  for (uint8_t index = 0; index < deviceCount; index++)
  {
    // go through sensors
    sensors.getAddress(T[index].addr, index);
    T[index].id = sensors.getUserData(T[index].addr);
  }

  // Check all 4 sensors are set
  for (uint8_t index = 0; index < deviceCount; index++)
  {
    Serial.println();
    Serial.println(T[index].id);
    printAddress(T[index].addr);
    Serial.println();
  }
  Serial.println();

}


void loop(void)
{
  Serial.println();
  Serial.print(millis());
  Serial.println("\treq temp");
  sensors.requestTemperatures();

  Serial.print(millis());
  Serial.println("\tGet temp by address");
  for (int i = 0; i < 4; i++)
  {
    Serial.print(millis());
    Serial.print("\t temp:\t");
    Serial.println(sensors.getTempC(T[i].addr));
  }

  Serial.print(millis());
  Serial.println("\tGet temp by ID");  // assume ID = 0, 1, 2, 3
  for (int id = 0; id < 4; id++)
  {
    Serial.print(millis());
    Serial.print("\t temp:\t");
    Serial.println(getTempByID(id));
  }

  delay(1000);
}

// END OF FILE