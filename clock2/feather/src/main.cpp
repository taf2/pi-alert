/*********************************************************************
This is an example sketch for our Monochrome SHARP Memory Displays

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/products/1393

These displays use SPI to communicate, 3 pins are required to
interface

Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.
BSD license, check license.txt for more information
All text above, must be included in any redistribution
*********************************************************************/
/*
 * Clock v2, using a Monochrome SHARP memory display 400x240 for fast refresh rates and partial updates
 *
 * Features
 *  - WiFi clock for always accurate time
 *  - ip based location to zip code for always local time
 *  - 1 alarm  with a snooze
 */
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SharpMem.h>

// any pins can be used
#define SHARP_SCK  5
#define SHARP_MOSI 18
#define SHARP_SS   25
#define BTN1       15
#define BTN2       27
#define LED_ON     33
#define LED_ALARM  21

Adafruit_SharpMem display(SHARP_SCK, SHARP_MOSI, SHARP_SS, 400, 240);

#define BLACK 0
#define WHITE 1

int minorHalfSize; // 1/2 of lesser of display width or height

void setup(void) {
  Serial.begin(115200);
  pinMode(BTN1, INPUT);
  pinMode(BTN2, INPUT);
  pinMode(LED_ON, OUTPUT);
  pinMode(LED_ALARM, OUTPUT);
  Serial.println("Hello!");
  digitalWrite(LED_ON, HIGH);

  // start & clear the display
  display.begin();
  display.clearDisplay();
}

void loop(void) {
  bool btn1 = false, btn2 = false;
  if (digitalRead(BTN1)) {
    btn1 = true;
  }
  if (digitalRead(BTN2)) {
    btn2 = true;
  }

  if (btn1) {
    Serial.println("button 1 pressed");
    digitalWrite(LED_ALARM, HIGH);
  }

  if (btn2) {
    Serial.println("button 2 pressed");
    digitalWrite(LED_ALARM, LOW);
  }
}
