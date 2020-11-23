#include <Arduino.h>

void setup() {
  pinMode(9, INPUT);
}

void loop() {
  digitalWrite(9, HIGH);
  delay(1000);
  digitalWrite(9, LOW);
  delay(1000);
}
