/*
   File: Dallas_Tester.ino modified
   By: (Forrest) Lee Erickson
   Date: 20230921
   Use the serial plotter to view results
   This program forked from Dallas_Tester.ino on 20230921
   Set D9 PWM for about 50% to test PWM on MOM (Mock up of Maryville hardware)
   20231128 Minimize program size
*/
#define COMPANY_NAME "pubinv.org "
#define PROG_NAME "Arduino-Temperature-Control-Library"
#define VERSION ";_Rev_0.4_LIB"
#define DEVICE_UNDER_TEST "Hardware: Mockup Of Maryville"  //A model number
#define LICENSE "GNU Affero General Public License, version 3 "

//Thermocouple amplifier setup
#include <OneWire.h>
#include "DallasTemperature.h"
#define ONE_WIRE_BUS 5  // But Due port 5 on Controller v1
#define TEMPERATURE_PRECISION 9 // Lower resolution
OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature.
int numberOfDevices; // Number of temperature devices found
DeviceAddress tempDeviceAddress; // We'll use this variable to store a found device address

//Other hardware
#define nFAN1_PWM 9 // The pin D9 for driving the Blower.
//Interference Tests
int D8 = 8;  //Going to set to GND
int D12 = 12; // Going to set to VCC

void setup(void)
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);  //Signal start of setup.
  //Interference Tests
  pinMode(D8, OUTPUT);
  digitalWrite(D8, LOW);
  pinMode(D12, OUTPUT);
  digitalWrite(D12, HIGH);
  Serial.begin(115200);   // start serial port
  delay(500);
  Serial.flush();
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println();

  sensors.begin();   // Start up the DallasTemprature library
  numberOfDevices = sensors.getDeviceCount();   // Grab a count of devices on the wire
  for (int i = 0; i <= numberOfDevices; i++) {
    Serial.print("TC");
    Serial.print(i);
    Serial.print(", ");
  }
  Serial.print(PROG_NAME);
  Serial.print(VERSION);
  Serial.println(DALLASTEMPLIBVERSION);

  //Print some status
  //  // locate devices on the bus
  //  Serial.print("Locating devices...");
  //  Serial.print("Found ");
  //  Serial.print(numberOfDevices, DEC);
  //  Serial.println(" devices.");
  //
  //  // report parasite power requirements
  //  Serial.print("Parasite power is: ");
  //  if (sensors.isParasitePowerMode()) Serial.println("ON");
  //  else Serial.println("OFF");

  // Loop through each device, print out address
  for (int i = 0; i < numberOfDevices; i++)
  {
    // Search the wire for address
    //    if (sensors.getAddress(tempDeviceAddress, i))
    if (false)
    {
      Serial.print("Found device ");
      Serial.print(i, DEC);
      Serial.print(" with address: ");
      printAddress(tempDeviceAddress);
      Serial.println();
      Serial.print("Setting resolution to ");
      Serial.println(TEMPERATURE_PRECISION, DEC);

      // set the resolution to TEMPERATURE_PRECISION bit (Each Dallas/Maxim device is capable of several different resolutions)
      sensors.setResolution(tempDeviceAddress, TEMPERATURE_PRECISION);
      Serial.print("Resolution actually set to: ");
      Serial.print(sensors.getResolution(tempDeviceAddress), DEC);
      Serial.println();
    } else {
      //      Serial.print("Found ghost device at ");
      //      Serial.print(i, DEC);
      //      Serial.print(" but could not detect address. Check power and cabling");
    }
  }

  analogWrite(nFAN1_PWM, 200);  // Set for low RPM. Remember PWM is inverted by Controller V1.1
  //Make some reads to clear initilized the system
  sensors.requestTemperatures(); // Send the command to get temperatures
  delay(10);
  sensors.requestTemperatures(); // Send the command to get temperatures
  delay(10);
  sensors.requestTemperatures(); // Send the command to get temperatures
  delay(10);

  digitalWrite(LED_BUILTIN, LOW);   //Signal end of setup.
}// end setup()

/* Functions */
// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}//end printAddress()

// function to print the temperature for a device
void printTemperature(DeviceAddress deviceAddress)
{
  // method 2 - faster
  float tempC = sensors.getTempC(deviceAddress);

  // method 3 - returning a structure
  //  struct sensors.celsius_result_t tempC = sensors.getTempC(deviceAddress);
  //  struct sensors.celsius_result_t tempC;
  //  sensors.my_Celsius_result_t.celsius_unit_t tempC ={};
  //  sensors.my_Celsius_result_t tempC ={};
  //  sensors.celsius_result_t tempC = sensors.getTempC(deviceAddress);
  //  sensors.my_Celsius_result_t tempC;
  //  sensors.my_Celsius_result_t = sensors.getTempC(deviceAddress);
  //  celsius_unit_t tempC = sensors.my_Celsius_result_t.value;

  //error_code
  if (tempC == DEVICE_DISCONNECTED_C)
  {
    Serial.print("Error:_Could_not_read_temperature_data_#:");
//    Serial.println(int(deviceAddress));
    return;
  }
  //  Serial.print("Temp C: ");
  Serial.print(tempC);
}// end printTemperature

void loop(void)
{
  //  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get global temperatures
  digitalWrite(LED_BUILTIN, HIGH);   //Signal end of temp request.
  for (int i = 0; i < numberOfDevices; i++)    // Loop through each device, print out temperature data
  {
    if (sensors.getAddress(tempDeviceAddress, i)) // Search the wire for address
    {
      // Output the device ID
      //		Serial.print("Temperature for device: ");
      //		Serial.print(i,DEC);
      // It responds almost immediately. Let's print out the data
      printTemperature(tempDeviceAddress); // Use a simple function to print out the data
      Serial.print(", ");
    }
    //else ghost device! Check your power requirements and cabling
  }
  Serial.println(); //end of line
  digitalWrite(LED_BUILTIN, LOW);   //Signal end of print temp.
}//end loop()
