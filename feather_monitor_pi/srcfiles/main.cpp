/**
 * watching pin 8, 10 and using 12 on the raspberry pi to know whether the pi is running normally
 */
#include <Arduino.h>

#define PI_ON 14
#define PI_HERE 21
#define RES_PIN 32

unsigned int interval = 0;
bool healthCheckMode = false;

void setup() {
  Serial.begin(115200);
  pinMode(PI_ON, INPUT);
  pinMode(PI_HERE, INPUT);
  pinMode(RES_PIN, OUTPUT);
}

void loop() {
  int on = digitalRead(PI_ON);
  if (on == HIGH) {
    if (healthCheckMode) {
      // run a health check
      digitalWrite(RES_PIN, HIGH);
      interval++;
      if (interval > 5) {
        interval = 0;
        healthCheckMode = false;
        int piHere = digitalRead(PI_HERE);
        if (piHere == LOW) {
          Serial.println("appears the PI is not on!");
        } else {
          Serial.println("PI is on");
        }
      }
    } else {
      // it appears on good news
      interval++;
      if (interval > 5) {
        interval = 0;
        healthCheckMode = true;
        Serial.println("start health check");
      } else {
        digitalWrite(RES_PIN, LOW);
      }
    }
  } else {
    // seems to be off!
    interval = 0;
    Serial.println("pi appears to be off!");
  }
  delay(1000);
}
