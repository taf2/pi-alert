#ifndef _EEPROM_SETTINGS_H
#define _EEPROM_SETTINGS_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <NTPClient.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <EEPROM.h> // saves the users selected timezone and preference for fahrenheit F or C
#include <ArduinoJson.h>
#include "writeAnything.h"

struct EEPROMSettings {
  EEPROMSettings();

  int timezoneOffset();
  void loadQuote(NTPClient &timeClient);
  void fetchQuote(NTPClient &timeClient);

  void nextZone(Adafruit_SSD1306 &display, NTPClient &timeClient);
  void prevZone(Adafruit_SSD1306 &display, NTPClient &timeClient);
  void adjustTimezone(Adafruit_SSD1306 &display, NTPClient &timeClient);

  int load();
  int save();

  // IMPORTANT we can't reorder these since we're reading/writing this into EEPROM
  short timezoneIndex;
  bool fahrenheit;
  bool hasSetting;
  int lastQuoteFetchDay;
  /*
   *  Time.now.to_i / 60 / 60 / 24
   * => 18560 -> day since 1970 which is what we can store this in short and get 32,767 which is around 2057 or we can use an int and not worry about it
   */
  char quote[128]; // space for a quote
  char author[32]; // space for an authors name
}; // we should have about 512 bytes to use in EEPROM 128 + 32 + 2 + 1 + 1 + 4 == 168 leaving plenty of room to spare...

#define EEPROM_SIZE sizeof(EEPROMSettings) + 3 // 0: timezoneIndex, 1: fahrenheit or celsius, 2: have preferences been set, 3: last time we pulled a quote

#endif
