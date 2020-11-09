/*
  Pulse
  Pulses the internal LED to demonstrate the analogWrite function

  This example code is in the public domain.  

  To upload to your Gemma or Trinket:
  1) Select the proper board from the Tools->Board Menu


  3) Plug in the Gemma/Trinket, make sure you see the green LED lit
  4) For windows, install the USBtiny drivers
  5) Press the button on the Gemma/Trinket - verify you see
     the red LED pulse. This means it is ready to receive data
  6) Click the upload button above within 10 seconds
*/
#include <Adafruit_DotStar.h>
#include <SPI.h>

#define NUMPIXELS 1
#define DATAPIN    8
#define CLOCKPIN   6

Adafruit_DotStar strip(NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BRG);


#define RELAY 4 // controls power to the raspberry pi
#define VLED 3 // external LED to indicate the raspberry pi is being given power

#define LED 13 // pulse 'digital' pin 13 - AKA the built in red LED  - this just appears inside the box we use this for diagnostics internally 
#define PHOTOCELL A0 // CdS photocell connected to this ANALOG 1 pin # which is PIN 1~
#define LOWBAT 0 // power good signal from the VERTER 5v will be LOW when power is good and HIGH when power is low
#define POWERSAVE 2 // use this to toggle power save mode on the vert to converve power

void blinkPhase(short value);

short lowbatcycles = 0; // keep track of how many times the battery has been low... we'll signal to the pi in the case of low power and eventually we should just disable the relay
short litcount = 0; // keep track of many seconds of good light we have before changing state.
short dimcount = 31; // keep track of how many seconds we are dim before changing state.
char SERIAL_BUFFER[1024]; // output buffer for easy debuggin

// the setup routine runs once when you press reset:
void setup() {
  Serial.begin(9600);

  strip.begin();
  strip.show(); // turns everything off
  
  // initialize the digital pin as an output.
  pinMode(LED, OUTPUT);
  pinMode(VLED, OUTPUT);

  pinMode(POWERSAVE, OUTPUT);

  pinMode(RELAY, OUTPUT);
  pinMode(PHOTOCELL, INPUT); // mark as our input pin
  
  //pinMode(LOWBAT, INPUT_PULLUP);

  digitalWrite(POWERSAVE, HIGH); // initial turn power save mode off until we determine the light levels
  digitalWrite(RELAY, LOW); // initially the pi power should be off

  digitalWrite(LED, 0); // we have light but the battery appears low start charging
  digitalWrite(VLED, LOW); // we have light but the battery appears low start charging

  

  // delay bootup to avoid blips
  for (short i = 0; i < 5; ++i) {
    
    delay(1000);
    blinkPhase(i);
    
  }
  
}

void softOn() {
  digitalWrite(LED, HIGH);
  digitalWrite(POWERSAVE, HIGH); // pull HIGH to turn off power save mode
  Serial.println("softOn");

}

void piOn() {
  digitalWrite(POWERSAVE, HIGH); // pull high to ensure power save it turned off

  digitalWrite(RELAY, HIGH); // ensure ourpower to the pi is ON
    
  digitalWrite(LED, LOW);  // PWM the LED from 255 (max) to 0
  digitalWrite(VLED, HIGH);  // PWM the LED from 255 (max) to 0
  Serial.println("piOn");
}

void softOff() {
  digitalWrite(LED, LOW);
  digitalWrite(POWERSAVE, LOW); // enable power save mode
  Serial.println("softOff");
}

void piOff() {
  digitalWrite(LED, LOW);  // disable the led here to save power
  digitalWrite(VLED, LOW);  // disable the led here to save power

  // send signal here to pi to shutdown proper
  // after 30 more seconds cut the power
  digitalWrite(RELAY, LOW); // it's been dark for 10 seconds power off
  Serial.println("piOff");
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

// NOTE: we can't figure out why when we draw more current through the VERTER power good goes HIGH but measuring
// the voltage on the verter shows it is over 4.7 volts into the VIN/GND pins... 

  if (lowbat == HIGH) {
    Serial.println("low battery detected");

    softOff();
    digitalWrite(LED, HIGH);
    digitalWrite(VLED, LOW);
    lowbatcycles++;
    if (lowbatcycles > 60) {
      piOff();
      lowbatcycles = 61; 
      // keep power off until batteries recharge
      return;
    }
  }
  lowbatcycles = 0;

  if (light > 490) { // light
    if (lowbatcycles != 0) { 
       delay(1000);
       return;
    }
    ++litcount;
    snprintf(SERIAL_BUFFER, 1023, "good light detected: %d %d - %d", litcount, light, lowbat);
    Serial.println(SERIAL_BUFFER);
  
    if (litcount > 10) {
      dimcount = 0;
      
      if (litcount < 30) {
        blinkPhase(litcount);
        softOn();
      } else if (litcount > 30) {
        piOn();
        litcount = 31;
      }
    } else {
      blinkPhase(litcount);
    }
  } else {
    ++dimcount;
    snprintf(SERIAL_BUFFER, 1023, "low light (%d) detected: %d %d - %d", dimcount, litcount, light, lowbat);
    Serial.println(SERIAL_BUFFER);


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
    }
  }

  delay(1000);
  
}
