/**
 * watching pin 8, 10 and using 12 on the raspberry pi to know whether the pi is running normally
 */
#include <TinyPICO.h>
#define FAN_PIN 15 // pull high to open transistor to turn fan power on via the 3.3v power rail
#define LED 26
#define PI_EN_POWER 18 // pull high to open transistor to turn pi on 
void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(PI_EN_POWER, OUTPUT);
  digitalWrite(FAN_PIN, LOW);
  digitalWrite(PI_EN_POWER, LOW);
}

void loop() {
  digitalWrite(LED, LOW);
  digitalWrite(FAN_PIN, LOW);
  digitalWrite(PI_EN_POWER, LOW);
  Serial.println("HIGH");
  delay(2000);
  digitalWrite(LED, LOW);
  digitalWrite(FAN_PIN, LOW);
  digitalWrite(PI_EN_POWER, LOW);
  Serial.println("LOW");
  delay(2000);
}
