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
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <printf.h>
#include <string.h>

#define NUMPIXELS 1
#define DATAPIN 7
#define CLOCKPIN 8
#define BUZZER 0 // instead of being able to transmit we'll to signal to our mp3 player
#define DELAY_BETWEEN_ALERTS 10000 // 10 seconds avoid repeat alerts


//address through which two modules communicate.
const byte address[6] = "00001";
short modeAlert = 0;
unsigned long cycles = 0;
unsigned long alertCycle = 0;
unsigned long cyclesSinceAlert = 0;

Adafruit_DotStar strip = Adafruit_DotStar(NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BGR);
RF24 radio(0, 1);  // CE, CSN

void setup() {
//  while (!Serial);
    Serial.begin(9600);
  
  // put your setup code here, to run once:
  strip.begin();
  strip.show(); // turns everything off


  radio.begin();
  //radio.setChannel(2);                     // Set the channel

  //radio.setPALevel(RF24_PA_LOW);           // Set PA LOW for this demonstration. We want the radio to be as lossy as possible for this example.
  radio.setPALevel(RF24_PA_MAX);
 // radio.setDataRate(RF24_1MBPS);           // Raise the data rate to reduce transmission distance and increase lossiness
 // radio.setAutoAck(1);                     // Ensure autoACK is enabled
 // radio.setRetries(15,15);                  // Optionally, increase the delay between retries. Want the number of auto-retries as high as possible (15)
 // radio.setCRCLength(RF24_CRC_16);         // Set CRC length to 16-bit to ensure quality of data

 //radio.setPALevel(RF24_PA_MIN);           // Set PA MIN for this demonstration. We want the radio to be as lossy as possible for this example.

  
  
  //set the address
  radio.openReadingPipe(0, address);
  
  //Set module as receiver
  radio.startListening();

  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, HIGH); // buzzer high is off

 // radio.powerUp();                        //Power up the radio

}

/* while we area in alerting mode
 * we know that each iteration of this function is about 1ms
 * so 100ms we want to keep the buzzer low
 * while in alert mode consume messages
 */
void alertMode() {

  if (radio.available()) {
    char text[32] = {0};
    radio.read(&text, sizeof(text));
    Serial.println(text);
  }
  
  if (cyclesSinceAlert < 100) {
    digitalWrite(BUZZER, LOW); // keep LOW to play the mp3
  } else {
    digitalWrite(BUZZER, HIGH); // bring the voltage high to avoid changing the volume
  }
  ++cyclesSinceAlert;
  if (cyclesSinceAlert > 12000) {
    modeAlert = 0;
  }
}

/*
 * while in monitorMode we'll consume data from the radio
 * if we get a postive alert e.g. matches our alert pattern Alert: (number)
 * then we'll switch into alert mode
 */
void monitorMode() {
  if (radio.available()) {
    char text[32] = {0};
    char bufstart[7] = {0};
    radio.read(&text, sizeof(text));
    Serial.println(text);
    
    Serial.println(text);
    if (strlen(text) > 7) { // greater than 'Alert: '
      strncpy(bufstart, text, 6);
      if (strcmp(bufstart, "Alert:") == 0) {
        if ( (cycles - alertCycle) > DELAY_BETWEEN_ALERTS) {
          // we have a confirmed alert
          alertCycle = cycles;
          cyclesSinceAlert = 0;
          modeAlert = 1;
        
          strip.setPixelColor(0, 0x000011); // blue when we receive a message
          strip.show();
          // pause for 12 seconds while it alerts to play the music
      //    digitalWrite(BUZZER, LOW); // enables the buzzer when low e.g. voltage flows through and makes that annoying buzz
      //    delay(100);
      //    digitalWrite(BUZZER, HIGH); // enables the buzzer when low e.g. voltage flows through and makes that annoying buzz
      //    delay(12000); // wait for the music to finish
        }
      } else {
        Serial.printf("buf start does not match: '%s' and '%s'", "Alert: ", bufstart);
      }
    }
  } else {
    strip.setPixelColor(0, 0x000000); // blank after sending
    strip.show();
    digitalWrite(BUZZER, HIGH); // turns off the buzzer when held high
  }
}

void loop() {
  if (modeAlert) {
    alertMode();
  } else {
    monitorMode();
  }
  delay(1); // 1 ms delay for each iteration so we we know that each loop is 1ms approximately
  ++cycles;
}
