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
#define RPI A0 // single to the raspberry pi pin 37/GPIO 26 that it should start the shutdown sequence PIN #0
#define LOWBAT 0 // white wire from PG pin on verter 5v to trinket PIN #1
#define PHOTOCELL A1 // CDS photocell connected to this A1 pin on trinket PIN #2
#define VLED A3 // External LED that we can see through the camera face PIN #3 
#define RELAY 4 // EN pin on the verter 5v, PIN #4
#define LED 1 // pulse 'digital' pin 1 - AKA the built in red LED  - this just appears inside the box  
// the setup routine runs once when you press reset:
void setup() {
  // initialize the digital pin as an output.
  pinMode(LED, OUTPUT);
  pinMode(VLED, OUTPUT);

  pinMode(RPI, OUTPUT);

  pinMode(RELAY, OUTPUT);
  pinMode(PHOTOCELL, INPUT); // mark as our input pin
  pinMode(LOWBAT, INPUT);

  digitalWrite(RPI, LOW); // initially the pi should be off
  digitalWrite(RELAY, LOW); // initially the pi power should be off

  digitalWrite(LED, 0); // we have light but the battery appears low start charging
  digitalWrite(VLED, LOW); // we have light but the battery appears low start charging

  // delay bootup to avoid blips
  for (short i = 0; i < 5; ++i) {
    
    delay(1000);
    blinkPhase(i);
    
  }
  
}

short litcount = 0; // keep track of many seconds of good light we have before changing state.
short dimcount = 31; // keep track of how many seconds we are dim before changing state.

void softOn() {
  digitalWrite(LED, HIGH);
  digitalWrite(RPI, HIGH); // ensure the pi is signaled for wake up
}

void piOn() {
  digitalWrite(RPI, HIGH); // ensure the pi is signaled for wake up

  digitalWrite(RELAY, HIGH); // ensure ourpower to the pi is ON
    
  digitalWrite(LED, LOW);  // PWM the LED from 255 (max) to 0
  digitalWrite(VLED, HIGH);  // PWM the LED from 255 (max) to 0
}

void softOff() {
  digitalWrite(LED, HIGH);
  digitalWrite(RPI, LOW);
}

void piOff() {
  digitalWrite(LED, LOW);  // disable the led here to save power
  digitalWrite(VLED, LOW);  // disable the led here to save power

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
    softOff();
    digitalWrite(LED, HIGH);
    digitalWrite(VLED, LOW);

    delay(1000);
    return;
  }
  
  if (light > 100) { // light
    ++litcount;
  
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
