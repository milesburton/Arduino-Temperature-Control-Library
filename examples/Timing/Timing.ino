//
//    FILE: Timing.ino
//  AUTHOR: Rob Tillaart
// VERSION: 0.0.3
// PURPOSE: show performance of DallasTemperature lib 
//          compared to datasheet times per resolution
//
// HISTORY:
// 0.0.1    2017-07-25 initial version
// 0.0.2    2020-02-13 updates to work with current lib version
// 0.0.3    2020-02-20 added timing measurement of setResolution

#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 2

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensor(&oneWire);

uint32_t start, stop;


void setup()
{
  Serial.begin(9600);
  Serial.println(__FILE__);
  Serial.print("DallasTemperature Library version: ");
  Serial.println(DALLASTEMPLIBVERSION);

  sensor.begin();
}

void loop()
{
  float ti[4] = { 94, 188, 375, 750 };

  Serial.println();
  Serial.println("Test takes about 30 seconds for 4 resolutions");
  Serial.println("RES\tTIME\tACTUAL\tGAIN");
  for (int r = 9; r < 13; r++)
  {
    start = micros();
    sensor.setResolution(r);
    Serial.println(micros() - start);

    start = micros();
    sensor.setResolution(r);
    Serial.println(micros() - start);

    uint32_t duration = run(20);
    float avgDuration = duration / 20.0;

    Serial.print(r);
    Serial.print("\t");
    Serial.print(ti[r - 9]);
    Serial.print("\t");
    Serial.print(avgDuration, 2);
    Serial.print("\t");
    Serial.print(avgDuration * 100 / ti[r - 9], 1);
    Serial.println("%");
  }
  delay(1000);
}

uint32_t run(int runs)
{
  float t;
  start = millis();
  for (int i = 0; i < runs; i++)
  {
    sensor.requestTemperatures();
    t = sensor.getTempCByIndex(0);
  }
  stop = millis();
  return stop - start;
}
