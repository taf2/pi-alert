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
#define RELAY 3

#define LED 1 // pulse 'digital' pin 1 - AKA the built in red LED
#define PHOTOCELL A1 // CdS photocell connected to this ANALOG pin #
#define LOWBAT A2 // white wire from LB pin on powerboost 1000 to pin A0 on trinket
#define RPI A0 // single to the raspberry pi pin 37 that it should start the shutdown sequence
// the setup routine runs once when you press reset:
void setup() {
  digitalWrite(RELAY, HIGH);

  // initialize the digital pin as an output.
  pinMode(LED, OUTPUT);
  pinMode(RPI, OUTPUT);

  pinMode(RELAY, OUTPUT);
  pinMode(PHOTOCELL, INPUT); // mark as our input pin
  pinMode(LOWBAT, INPUT);

  digitalWrite(RPI, LOW);

}

int litcount = 0; // keep track of many seconds of good light we have before changing state.
int dimcount = 0; // keep track of how many seconds we are dim before changing state.

// the loop routine runs over and over again forever:
void loop() {
  int lowbat = digitalRead(LOWBAT); // get value of battery reading
  int light  = analogRead(PHOTOCELL); // returns values between 0 and 1023

  if (!lowbat && light > 100) { // plenty of sun light
    ++litcount;
    if (litcount > 10) {
      dimcount = 0;
      digitalWrite(RPI, LOW);

      digitalWrite(RELAY, HIGH);
    
      digitalWrite(LED, 0);  // PWM the LED from 255 (max) to 0
    }
  } else {
    ++dimcount;
    if (dimcount > 10) { // it appears we are low on sun light
      litcount = 0;
      digitalWrite(LED, 254);
      digitalWrite(RPI, HIGH);
      if (dimcount > 30) {
        digitalWrite(LED, 0);  // disable the led here to save power

        // send signal here to pi to shutdown proper
        // after 30 more seconds cut the power
        digitalWrite(RELAY, LOW); // it's been dark for 10 seconds power off

        dimcount = 0;
      }
    }
  }
  
  delay(1000);
  
}
