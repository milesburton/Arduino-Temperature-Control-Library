//
//    FILE: SaveRecallConfiguration.ino
//  AUTHOR: GitKomodo
// VERSION: 0.0.1
// PURPOSE: Show DallasTemperature lib functionality to
//          save/recall values to/from EEPROM
//
// HISTORY:
// 0.0.1 = 2020-02-16 initial version
//

#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 2

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensor(&oneWire);
DeviceAddress deviceAddress;

void setup()
{
  Serial.begin(9600);
  Serial.println("DallasTemperature Library Save / Recall Configuration DEMO");
  Serial.println("==========================================================");
  Serial.println("\nNote: DS1820 and DS18S20 sensors don't have a resolution configuration register\nand will always show resolution 12.");
  
  sensor.begin();
  sensor.getAddress(deviceAddress,0);
  
  Serial.println("\nUsing specific device address");
  Serial.println("-----------------------------");
  
  Serial.println("\nWhen powered up the scratchpad is initialized with EEPROM values automatically.");
  printValues();
  
  Serial.println("\nSetting resolution to 10 and user data to 17 and 19. Default setting of the lib\nis to save these values to EEPROM.");
  setValues(10,17,19);
  printValues();
  
  Serial.println("\nTurning automatic saving OFF!");
  sensor.setAutoSaveConfiguration(false);
  
  Serial.println("\nSetting resolution to 11 and user data to 23 and 29. The values are now only\nwritten to the scratchpad.");
  setValues(11,23,29);
  printValues();
  
  Serial.println("\nRecalling values from EEPROM. The scratchpad now contains resolution 10 and user\ndata 17 and 19 again.");
  sensor.recallConfiguration(deviceAddress);
  printValues();

  Serial.println("\nAddressing all devices");
  Serial.println("----------------------");
  
  Serial.println("\nSetting resolution to 11 and user data to 23 and 29 again. The values are only\nwritten to the scratchpad.");
  setValues(11,23,29);
  printValues();
  
  Serial.println("\nSaving values to EEPROM without a specific device address.");
  sensor.saveConfiguration();
  
  Serial.println("\nSetting resolution to 10 and user data to 17 and 19.  The values are only\nwritten to the scratchpad.");
  setValues(10,17,19);
  printValues();
  
  Serial.println("\nRecalling values from EEPROM without a specific device address. The scratchpad\nnow contains resolution 11 and user data 23 and 29 again.");
  sensor.recallConfiguration();
  printValues();
}

void setValues(uint8_t resolution, uint8_t data1, int8_t data2) {
  sensor.setResolution(deviceAddress,resolution);
  sensor.setUserData(deviceAddress,(data1 << 8) | data2);
}

void printValues() {
  Serial.println("\tCurrent values on the scratchpad:");
  Serial.print("\tResolution:\t");
  Serial.println(sensor.getResolution(deviceAddress));
  uint16_t userdata = sensor.getUserData(deviceAddress);
  Serial.print("\tUser data 1:\t");
  Serial.println(userdata >> 8);
  Serial.print("\tUser data 2:\t");
  Serial.println(userdata & 255);
}

void loop(){
  // Not looping because the number of times EEPROM can be written is limited.
}
