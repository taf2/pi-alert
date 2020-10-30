
#include <SPI.h>
#include <Adafruit_VS1053.h>
#include <SD.h>
#include "pins.h"

static void printDirectory(File dir, int numTabs);

Adafruit_VS1053_FilePlayer *musicPlayer = NULL;

void setup() {
  Serial.begin(9600);
  delay(5000);
  Serial.println("Adafruit VS1053 Simple Test");
  musicPlayer = new Adafruit_VS1053_FilePlayer(-1, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);
  if (!musicPlayer->begin()) { // initialise the music player
     while (1) {
       Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
       delay(1000);
     }
  }
  Serial.println(F("VS1053 found"));

  if (!SD.begin(CARDCS)) {
    while (1) {
      Serial.println(F("SD failed, or not present"));
      delay(1000);
    }
  }

  // list files
  printDirectory(SD.open("/"), 0);

  // Set volume for left, right channels. lower numbers == louder volume!
  musicPlayer->setVolume(10,10);

  // If DREQ is on an interrupt pin (on uno, #2 or #3) we can do background
  // audio playing
  musicPlayer->useInterrupt(VS1053_FILEPLAYER_PIN_INT);  // DREQ int

  // Play one file, don't return until complete
  Serial.println(F("Playing track 001"));
  musicPlayer->playFullFile("/COME-T~1.MP3");
  // Play another file in the background, REQUIRES interrupts!
  //Serial.println(F("Playing track 002"));
//  musicPlayer->startPlayingFile("/track002.mp3");
}

void loop() {
  // File is playing in the background
  if (musicPlayer->stopped()) {
    Serial.println("Done playing music");
    while (1) {
      delay(10);  // we're done! do nothing...
    }
  }
  if (Serial.available()) {
    char c = Serial.read();

    // if we get an 's' on the serial console, stop!
    if (c == 's') {
      musicPlayer->stopPlaying();
    }

    // if we get an 'p' on the serial console, pause/unpause!
    if (c == 'p') {
      if (! musicPlayer->paused()) {
        Serial.println("Paused");
        musicPlayer->pausePlaying(true);
      } else {
        Serial.println("Resumed");
        musicPlayer->pausePlaying(false);
      }
    }
  }

  delay(100);
}


/// File listing helper
void printDirectory(File dir, int numTabs) {
  while(true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      //Serial.println("**nomorefiles**");
      break;
    }
    for (uint8_t i=0; i<numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs+1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}
