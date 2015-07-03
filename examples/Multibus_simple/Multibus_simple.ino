#include <OneWire.h>
#include <DallasTemperature.h>

int oneWirePins[]={3,7};//OneWire DS18x20 temperature sensors on these wires
const int oneWirePinsCount=sizeof(oneWirePins)/sizeof(int);

OneWire ds18x20[oneWirePinsCount];
DallasTemperature sensor[oneWirePinsCount];


void setup(void) {
  // start serial port
  Serial.begin(9600);
  Serial.println("Dallas Temperature Multiple Bus Control Library Simple Demo");
  Serial.print("============Ready with ");
  Serial.print(oneWirePinsCount);
  Serial.println(" Sensors================");
  
  // Start up the library on all defined bus-wires
  DeviceAddress deviceAddress;
  for (int i=0; i<oneWirePinsCount; i++) {;
    ds18x20[i].setPin(oneWirePins[i]);
    sensor[i].setOneWire(&ds18x20[i]);
    sensor[i].begin();
    if (sensor[i].getAddress(deviceAddress, 0)) sensor[i].setResolution(deviceAddress, 12);
  }
  
}

void loop(void) {
  // call sensors.requestTemperatures() to issue a global temperature 
  // request to all devices on the bus
  Serial.print("Requesting temperatures...");
  for (int i=0; i<oneWirePinsCount; i++) {
    sensor[i].requestTemperatures();
  }
  Serial.println("DONE");
  
  delay(1000);
  for (int i=0; i<oneWirePinsCount; i++) {
    float temperature=sensor[i].getTempCByIndex(0);
    Serial.print("Temperature for the sensor ");
    Serial.print(i);
    Serial.print(" is ");
    Serial.println(temperature);
  }
  Serial.println();
}
