#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into port 2 on the Arduino, while external pullup P-MOSFET gate into port 3
#define ONE_WIRE_BUS    2
#define ONE_WIRE_PULLUP 3

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire, ONE_WIRE_PULLUP);

void setup(void)
{
  // start serial port
  Serial.begin(9600);
  Serial.println("Dallas Temperature IC Control Library Demo");

  // Start up the library
  sensors.begin();
}

void loop(void)
{ 
  // call sensors.requestTemperatures() to issue a global temperature 
  // request to all devices on the bus
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");
  
 for(int i=0;i<sensors.getDeviceCount();i++) {
   Serial.println("Temperature for Device "+String(i)+" is: " + String(sensors.getTempCByIndex(i)));
 } 
}
