
// DISABLED AS NOT ALL STD LIBRARIES ARE MOCKED / INCLUDEABLE


//
//    FILE: unit_test_001.cpp
//  AUTHOR: Miles Burton / Rob Tillaart
//    DATE: 2021-01-10
// PURPOSE: unit tests for the Arduino-Temperature-Control-Library
//          https://github.com/MilesBurton/Arduino-Temperature-Control-Library
//          https://github.com/Arduino-CI/arduino_ci/blob/master/REFERENCE.md
//

// supported assertions
// ----------------------------
// assertEqual(expected, actual);               // a == b
// assertNotEqual(unwanted, actual);            // a != b
// assertComparativeEquivalent(expected, actual);    // abs(a - b) == 0 or (!(a > b) && !(a < b))
// assertComparativeNotEquivalent(unwanted, actual); // abs(a - b) > 0  or ((a > b) || (a < b))
// assertLess(upperBound, actual);              // a < b
// assertMore(lowerBound, actual);              // a > b
// assertLessOrEqual(upperBound, actual);       // a <= b
// assertMoreOrEqual(lowerBound, actual);       // a >= b
// assertTrue(actual);
// assertFalse(actual);
// assertNull(actual);

// // special cases for floats
// assertEqualFloat(expected, actual, epsilon);    // fabs(a - b) <= epsilon
// assertNotEqualFloat(unwanted, actual, epsilon); // fabs(a - b) >= epsilon
// assertInfinity(actual);                         // isinf(a)
// assertNotInfinity(actual);                      // !isinf(a)
// assertNAN(arg);                                 // isnan(a)
// assertNotNAN(arg);                              // !isnan(a)

#include <ArduinoUnitTests.h>


#include "Arduino.h"
#include "OneWire.h"
#include "DallasTemperature.h"

/*
NOTE 2022-06-03: why is unit test disabled.
There are problems with the including of util/crc16.h by Onewire.h
Without it test can't be run.
*/


unittest_setup()
{
  fprintf(stderr, "VERSION: %s\n", DALLASTEMPLIBVERSION);
}

unittest_teardown()
{
  fprintf(stderr, "\n");
}


unittest(test_models)
{
  assertEqual(0x10, DS18S20MODEL);
  assertEqual(0x28, DS18B20MODEL);
  assertEqual(0x22, DS1822MODEL);
  assertEqual(0x3B, DS1825MODEL);
  assertEqual(0x42, DS28EA00MODEL);
}


unittest(test_error_code)
{
  assertEqual(-255,   DEVICE_DISCONNECTED_C);
  assertEqual(-427,   DEVICE_DISCONNECTED_F);
  assertEqual(-32640, DEVICE_DISCONNECTED_RAW);

  assertEqual(-254, DEVICE_FAULT_OPEN_C);
  assertEqualFloat(-425.199982, DEVICE_FAULT_OPEN_F, 0.001);
  assertEqual(-32512, DEVICE_FAULT_OPEN_RAW);

  assertEqual(-253, DEVICE_FAULT_SHORTGND_C);
  assertEqualFloat(-423.399994, DEVICE_FAULT_SHORTGND_F, 0.001);
  assertEqual(-32384, DEVICE_FAULT_SHORTGND_RAW);

  assertEqual(-252, DEVICE_FAULT_SHORTVDD_C);
  assertEqualFloat(-421.599976, DEVICE_FAULT_SHORTVDD_F, 0.001);
  assertEqual( -32256, DEVICE_FAULT_SHORTVDD_RAW);
}


unittest(test_simple)
{
/*
  // BASED UPON SIMPLE (won't run, see above)
  // 
  // Data wire is plugged into port 2 on the Arduino
  #define ONE_WIRE_BUS 2

  // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
  OneWire oneWire(ONE_WIRE_BUS);

  // Pass our oneWire reference to Dallas Temperature. 
  DallasTemperature sensors(&oneWire);
  sensors.begin();
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);
  if(tempC != DEVICE_DISCONNECTED_C) 
  {
    fprintf(stderr, "Temperature for the device 1 (index 0) is: ");
    fprintf(stderr, "5f\n", tempC);
  } 
  else
  {
    fprintf(stderr, "Error: Could not read temperature data\n");
  }
*/

  assertEqual(1, 1);  // keep unit test happy
}

unittest_main()

// --------
