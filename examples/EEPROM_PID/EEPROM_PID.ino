#include <EEPROM.h>
#include <PID_v1.h>

#define EEPROM_SIZE 64

double Setpoint, Input, Output;
PID myPID(&Input, &Output, &Setpoint, 2.0, 5.0, 1.0, DIRECT);

void setup() {
    Serial.begin(115200);
    EEPROM.begin(EEPROM_SIZE);
    if (myPID.loadParameters(0)) {
        Serial.println("PID params loaded from EEPROM:");
    } else {
        Serial.println("No valid params in EEPROM, using defaults.");
    }
    Serial.print("Kp="); Serial.println(myPID.GetKp());
    Serial.print("Ki="); Serial.println(myPID.GetKi());
    Serial.print("Kd="); Serial.println(myPID.GetKd());
    myPID.SetMode(AUTOMATIC);
}

void loop() {
    Input = analogRead(A0) * (5.0 / 1023.0);
    myPID.Compute();
    analogWrite(5, Output);
    if (Serial.available()) {
        char cmd = Serial.read();
        if (cmd == 's') {
            double newKp = myPID.GetKp() + 0.5;
            myPID.SetTunings(newKp, myPID.GetKi(), myPID.GetKd());
            myPID.saveParameters(0);
            Serial.print("Saved new Kp="); Serial.println(newKp);
        }
    }
    delay(1000);
}
