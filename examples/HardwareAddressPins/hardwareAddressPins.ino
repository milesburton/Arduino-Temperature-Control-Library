
// This simple sketch demonstrates how to map and use the hardware pins to a device index
// then reference the map to pull the devices in order
//
// The hardware assignable pins/address is helpful to place devices on the same bus in a speicfic
// order and identify them based on hardware specification.
//
// One other note, nowhere in the spec (at least for the MAX31850) does it say these pins can change
// over time.  If that is allowed, one could envision using these pins instead to detect when events
// or faults might occur in an applcation.

#include "DallasTemperature.h"

#include <OneWire.h>

DallasTemperature dallas(new OneWire(D3));

uint8_t DeviceIndexes[15];

void setup() {
    Serial.begin(9600);
    dallas.begin();
    dallas.setResolution(9);
    Serial.println("Resolution: "+String(dallas.getResolution()));

    for (int x=0; x < dallas.getDeviceCount(); x++) {
        DeviceIndexes[dallas.getAddressPinsByIndex(x)] = x;
    }
}

float c, f;
int32_t pins;

void loop() {

    // this is a hack, but set the resolution to 10bits for the faster conversion
    dallas.setResolution(10);

    // get temps
    digitalWrite(D7, HIGH);
    dallas.requestTemperatures();
    digitalWrite(D7, LOW);

    // walk though the devices by the pin assignment mapped in the setup()
    for (int x=0; x < dallas.getDeviceCount(); x++) {
        Serial.print("  Probe "+String(x)+" index "+String(DeviceIndexes[x])+" Temperature = ");
        c = dallas.getTempCByIndex(DeviceIndexes[x]);
        f = dallas.getTempFByIndex(DeviceIndexes[x]);
        Serial.println(String(c)+" C, "+String(f)+" F\n");
    }
    delay(3000);
}
