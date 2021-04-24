/**
 * watching pin 8, 10 and using 12 on the raspberry pi to know whether the pi is running normally
 */
#include <TinyPICO.h>
#define LED 22
#define PHOTOCELL 14
char SERIAL_BUFFER[1024];
void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  pinMode(PHOTOCELL, INPUT); // mark as our input pin
}

void loop() {
  digitalWrite(LED, HIGH);
  int light  = analogRead(PHOTOCELL); // returns values between 0 and 1023
  snprintf(SERIAL_BUFFER, 1023, "light detected: %d", light);
  Serial.println(SERIAL_BUFFER);
  delay(1000);
//  digitalWrite(LED, LOW);
//  delay(1000);
}
