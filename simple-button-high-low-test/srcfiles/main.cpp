/*
 * mapping pins for nrf24l01 to trinket m0
 * SPI MOSI or blue 4 (A4)
 * SPI SCK or green 3 (A3)
 * SPI MISO or purple 2 (A1)
 * 0 is yellow
 * 1 is orange
 * 
 * because we are only a receiver and not transmitter we'll switch pin 0 (yellow) 
 * to be our buzzer pin e.g. play a sound whe n we have received a signal
 *
 * see: https://lastminuteengineers.com/nrf24l01-arduino-wireless-communication/
 * see: https://learn.adafruit.com/adafruit-trinket-m0-circuitpython-arduino/pinouts
 */
#include <Adafruit_DotStar.h>
#include <printf.h>
#include <string.h>

#define NUMPIXELS 1
#define DATAPIN 7
#define CLOCKPIN 8
#define BUTTON 2 // when pressed we are high
#define BUZZER 0 // instead of being able to transmit we'll to signal to our mp3 player
#define DELAY_BETWEEN_ALERTS 10000 // 10 seconds avoid repeat alerts


Adafruit_DotStar strip = Adafruit_DotStar(NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BGR);

void setup() {
  Serial.begin(9600);
  
  // put your setup code here, to run once:
  strip.begin();
  strip.show(); // turns everything off

  pinMode(BUZZER, OUTPUT);
  pinMode(BUTTON, INPUT);

  digitalWrite(BUZZER, HIGH); // buzzer high is off
}

void loop() {
  int pressed = digitalRead(BUTTON);
  if (pressed) {
    digitalWrite(BUZZER, LOW);
  } else {
    digitalWrite(BUZZER, HIGH);
  }
}
