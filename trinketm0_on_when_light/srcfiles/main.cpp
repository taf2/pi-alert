#include <Adafruit_DotStar.h>
#include <SPI.h>

#define NUMPIXELS 1
#define DATAPIN 7
#define CLOCKPIN 8

Adafruit_DotStar strip = Adafruit_DotStar(NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BGR);

#define LED 13 // pulse 'digital' pin 13 - AKA the built in red LED  - this just appears inside the box we use this for diagnostics internally 

#define RPI 0 // single to the raspberry pi pin 29/GPIO 5 that it should start the shutdown sequence PIN #0
#define LOWBAT 1 // from PG pin on verter 5v to trinket PIN #1
#define PHOTOCELL A1 // CDS photocell connected to this A1 pin on trinket PIN #2
#define VLED A3 // External LED that we can see through the camera face PIN #3 
#define RELAY 4 // EN pin on the verter 5v, PIN #4
// because led 1 is also lowbat 0 we need to avoid messing with this pin it'll be an input pin only so well see led light up when low bat?
//#define LED 1 // pulse 'digital' pin 1 - AKA the built in red LED  - this just appears inside the box  
// the setup routine runs once when you press reset:
/*
adafruit verter: via https://forums.adafruit.com/viewtopic.php?f=19&t=78545

EN is the enable line. Pulling it low shuts off the converter.

G is GND.

PG is 'Power Good'. It goes low when the chip decides the output level is stable.
                    It's more powerful than a low-battery warning, but does the same basic job.
                    When the battery discharges far enough that the chip can't produce a
                    well-regulated output voltage for the given load,
                    the PG pin will go high (technically it's an open-drain pin with a weak pull-up resistor,
                    but that's enough to make a digital input work).

PS is 'Power Save'. If you pull it low, the chip will drop to a lower frequency for light loads. It runs more efficiently that way, but you'll get some lag if the current load suddenly becomes larger.

The Verter will work happily with the 2200mAh cylindrical LiPo.

#### ERRRR conflicting information about this board #####

see: https://forums.adafruit.com/viewtopic.php?f=8&t=145149
The PG signal is Power Good.
That pin goes low if the boost-buck converter thinks there's a problem with the upstream power supply.

*/

char SERIAL_BUFFER[1024];
void blinkPhase(short value);

void setup() {
  Serial.begin(9600);

  strip.begin();
  strip.show(); // turns everything off

  // initialize the digital pin as an output.
//  pinMode(LED, OUTPUT);
  pinMode(VLED, OUTPUT);

  pinMode(RPI, OUTPUT);

  pinMode(RELAY, OUTPUT);
  pinMode(PHOTOCELL, INPUT); // mark as our input pin
  pinMode(LOWBAT, INPUT);

  digitalWrite(RPI, LOW); // initially the pi should be off
  digitalWrite(RELAY, LOW); // initially the pi power should be off

//  digitalWrite(LED, 0); // we have light but the battery appears low start charging
  digitalWrite(VLED, LOW); // we have light but the battery appears low start charging

  // delay bootup to avoid blips
  for (short i = 0; i < 5; ++i) {
    
    delay(1000);
    blinkPhase(i);
    
  }
  
}

short litcount = 0; // keep track of many seconds of good light we have before changing state.
short dimcount = 31; // keep track of how many seconds we are dim before changing state.
short lowcount = 0; // keep track of how many seconds we are low battery and possibly go full off

void softOn() {
//  digitalWrite(LED, HIGH);

  digitalWrite(RPI, HIGH); // ensure the pi is signaled for wake up
}

void piOn() {
  digitalWrite(RPI, HIGH); // ensure the pi is signaled for wake up

  digitalWrite(RELAY, HIGH); // ensure ourpower to the pi is ON
    
//  digitalWrite(LED, LOW);  // PWM the LED from 255 (max) to 0
  digitalWrite(VLED, HIGH);  // PWM the LED from 255 (max) to 0
}

void softOff() {
//  digitalWrite(LED, HIGH);
  digitalWrite(RPI, LOW);
  Serial.println("softOff");
  strip.setPixelColor(0, 64, 0, 0);
  strip.show();
}

void piOff() {
//  digitalWrite(LED, LOW);  // disable the led here to save power
  strip.setPixelColor(0, 0, 0, 0);
  strip.clear();
  strip.show();

  digitalWrite(VLED, LOW);  // disable the led here to save power
  digitalWrite(RPI, LOW);
  Serial.println("piOff");

  // send signal here to pi to shutdown proper
  // after 30 more seconds cut the power
  digitalWrite(RELAY, LOW); // it's been dark for 10 seconds power off
}

void blinkPhase(short value) {
  if (value % 2 == 0) {
    digitalWrite(VLED, LOW);
  } else {
    digitalWrite(VLED, HIGH);
  }
}

// the loop routine runs over and over again forever:
void loop() {
  int lowbat = digitalRead(LOWBAT); // get value of battery reading  HIGH indicates low battery LOW indicates plenty of battery
  int light  = analogRead(PHOTOCELL); // returns values between 0 and 1023

  if (lowbat == HIGH) {
    Serial.println("lowbat");
    ++lowcount;
    softOff();

    if (lowcount > 10) { // 10 seconds of low power turn off more things
      lowcount = 11;
      // everything off we're nearly out of battery/juice
//      digitalWrite(LED, LOW);
      digitalWrite(VLED, LOW);
      piOff();
    } else {
      blinkPhase(lowcount);
    }

    delay(1000);
    return;
  } else {
    strip.setPixelColor(0, 0, 0, 0);
    strip.clear();
    strip.show();
    lowcount = 0;
    softOn();
    piOn();
  }

  snprintf(SERIAL_BUFFER, 1023, "light detected: %d - %d", light, lowbat);
  Serial.println(SERIAL_BUFFER);
  
  if (light > 80) { // light
    digitalWrite(VLED, HIGH);
    /*++litcount;
  
    if (litcount > 10) { 
      dimcount = 0;
      
      if (litcount < 20) {
        blinkPhase(litcount);
        softOn();
      } else if (litcount > 20) {
        piOn();
        litcount = 21;
      }
    } else {
      blinkPhase(litcount);
    }*/
  } else {
    digitalWrite(VLED, LOW);
    /*++dimcount;
    if (dimcount > 10) { // it appears we are low on sun light
      litcount = 0; // it appears dark 
      if (dimcount < 30) {
        blinkPhase(dimcount);
        softOff();
      } else if (dimcount > 30) {
        piOff();
        dimcount = 31;
      }
    } else {
       blinkPhase(dimcount);
    }*/
  }

  delay(1000);
  
}
