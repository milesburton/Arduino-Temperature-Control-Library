//
//    FILE: SaveRecallScratchPad.ino
//  AUTHOR: GitKomodo
// VERSION: 0.0.1
// PURPOSE: Show DallasTemperature lib functionality to
//          save/recall ScratchPad values to/from EEPROM
//
// HISTORY:
// 0.0.1 = 2020-02-18 initial version
//

#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 2

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress deviceAddress;

void setup()
{
  Serial.begin(9600);
  Serial.println(__FILE__);
  Serial.println("Dallas Temperature Demo");
  
  sensors.begin();
  
  // Get ID of first sensor (at index 0)
  sensors.getAddress(deviceAddress,0);

  // By default configuration and alarm/userdata registers are also saved to EEPROM
  // when they're changed. Sensors recall these values automatically when powered up.
  
  // Turn OFF automatic saving of configuration and alarm/userdata registers to EEPROM
  sensors.setAutoSaveScratchPad(false);
  
  // Change configuration and alarm/userdata registers on the scratchpad
  int8_t resolution = 12;
  sensors.setResolution(deviceAddress,resolution);
  int16_t userdata = 24680;
  sensors.setUserData(deviceAddress,userdata);

  // Save configuration and alarm/userdata registers to EEPROM
  sensors.saveScratchPad(deviceAddress);

  // saveScratchPad can also be used without a parameter to save the configuration
  // and alarm/userdata registers of ALL connected sensors to EEPROM:
  //
  //   sensors.saveScratchPad();
  //
  // Or the configuration and alarm/userdata registers of a sensor can be saved to
  // EEPROM by index:
  //
  //   sensors.saveScratchPadByIndex(0);
  
  // Print current values on the scratchpad (resolution = 12, userdata = 24680)
  printValues();
  
}

void loop(){
  
  // Change configuration and alarm/userdata registers on the scratchpad
  int8_t resolution = 10;
  sensors.setResolution(deviceAddress,resolution);
  int16_t userdata = 12345;
  sensors.setUserData(deviceAddress,userdata);
  
  // Print current values on the scratchpad (resolution = 10, userdata = 12345)
  printValues();
  
  delay(2000);
  
  // Recall configuration and alarm/userdata registers from EEPROM
  sensors.recallScratchPad(deviceAddress);
  
  // recallScratchPad can also be used without a parameter to recall the configuration
  // and alarm/userdata registers of ALL connected sensors from EEPROM:
  //
  //   sensors.recallScratchPad();
  //
  // Or the configuration and alarm/userdata registers of a sensor can be recalled
  // from EEPROM by index:
  //
  //   sensors.recallScratchPadByIndex(0);
  
  // Print current values on the scratchpad (resolution = 12, userdata = 24680)
  printValues();
  
  delay(2000);
  
}

void printValues() {
  
  Serial.println();
  Serial.println("Current values on the scratchpad:");
  
  Serial.print("Resolution:\t");
  Serial.println(sensors.getResolution(deviceAddress));
  
  Serial.print("User data:\t");
  Serial.println(sensors.getUserData(deviceAddress));
  
}
