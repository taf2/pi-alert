#include <Arduino.h>
// assuming m4 and m0 have similar wiring or m4 does not have same requirements as m0 and is maybe like esp32...
/*
#define RFM69_CS      10   // "B"
#define RFM69_RST     11   // "A"
#define RFM69_IRQ     6    // "D"
#define RFM69_IRQN    digitalPinToInterrupt(RFM69_IRQ )
*/
#define RFM95_CS  10   // "B"
#define RFM95_RST 11   // "A"
#define RFM95_INT  6   // "D"
#define LED 9
#define VOLTAGE_INPUT A5 // get a USB voltage signal over a resistor that lowers the input voltage to a safer 2.5/2.6V

int active = 1;

void setup() {
  Serial.begin(9600);
  pinMode(LED, INPUT);
  pinMode(VOLTAGE_INPUT, INPUT);
}

void loop() {
  int current = active;
  float reading = analogRead(VOLTAGE_INPUT);
  if (reading > 500.0) {
    active = 1;
    digitalWrite(LED, HIGH);
  } else {
    active = 0;
    digitalWrite(LED, LOW);
  }
  if (current != active) {
    Serial.println("voltage change");
    Serial.println(active);
    Serial.println(reading);
  }
}
