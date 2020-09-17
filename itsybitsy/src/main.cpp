#include <string.h>
#include <stdlib.h>

#include <SPI.h>
#define MAX_COUNT 4294000000
#define WINDOW_SIZE 100

const int LED_PIN = 7;
const int DIS_PIN = A0;

long samples = 0;
double average = 0;
long accum = 0;
long totalSamples = 0;
double totalAverage = 0;
long totalAccum = 0;
char buffer[4096];

void setup() {
  Serial.begin(9600);

  pinMode(LED_PIN, OUTPUT);
  pinMode(DIS_PIN, INPUT);
}

void loop() {
/*  Serial.println("high");
  digitalWrite(LED_PIN, HIGH);
  delay(1000);
  Serial.println("low");
  digitalWrite(LED_PIN, LOW);
  delay(1000);
  */

  /*
  Scale factor is (Vcc/512) per inch. A 5V supply yields ~9.8mV/in
  Arduino analog pin goes from 0 to 1024, so the value has to be divided by 2 to get the actual inches
  */
  long dist = analogRead(DIS_PIN)/4;
  samples++;
  totalSamples++;
  accum += dist;
  totalAccum  += dist;
  if (totalSamples > MAX_COUNT || totalAccum > MAX_COUNT) {
    totalSamples = 0;
    totalAccum = 0;
  }
  if (samples % 2 == 0) {
    average = accum / samples;
  }
  if (samples > WINDOW_SIZE) {
    samples = 0;
    accum = 0;
  }
  if (totalSamples % 2 == 0) {
    totalAverage = totalAccum / totalSamples;
  }
  snprintf(buffer, 4096, "dist: %ld, avg: %d, last %ld samples, total avg: %d, total samples: %ld", dist, (int)average, samples, (int)totalAverage, totalSamples);
  Serial.println(buffer);

  if (totalSamples > WINDOW_SIZE) {
    // now we can start asking the question is the current distance measure 
    // less than our average such that we believe we should sound the alarm
    if (dist < average-10) { // TODO: maybe we use std as our factor to compute 10 intead of a constant?
      // alarm
      Serial.println("alarm");
      digitalWrite(LED_PIN, HIGH);
      delay(10000);
    } else {
      digitalWrite(LED_PIN, LOW);
    }

    delay(1);
  } else {

    // we're in the initial window warm up slow start
    delay(10);
  }
}
