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
  void fetchWeather(NTPClient &timeClient);

  void nextZone(Adafruit_SSD1306 &display, NTPClient &timeClient);
  void prevZone(Adafruit_SSD1306 &display, NTPClient &timeClient);
  void adjustTimezone(Adafruit_SSD1306 &display, NTPClient &timeClient);

  bool load();	// load returns true only if the crc32 matches the first time a device loads before ever saving this would return false
  bool save();	// writes this datastructure to eeprom with a crc32 checksum
  bool good();	// checks the data saved and confirms whether it looks reasonable and likely good for use e.g. the data is reasonable

	uint32_t crc32();

  // IMPORTANT we can't reorder these since we're reading/writing this into EEPROM
	int _timezoneOffset;
  short timezoneIndex;
  bool fahrenheit;
  bool hasSetting;
  int lastQuoteFetchDay;
  /*
   *  Time.now.to_i / 60 / 60 / 24
   * => 18560 -> day since 1970 which is what we can store this in short and get 32,767 which is around 2057 or we can use an int and not worry about it
   */
  bool configured;
  char ssid[32]; // wifi ssid
  char pass[32]; // password ssid
  char quote[256]; // space for a quote
  char author[32]; // space for an authors name
  char zipcode[8]; // zipcode for getting the weather
  char weatherAPI[34]; // api key for the weather
  float temp;
  float feels_like;
  short  hour; // 24 hour clock max value 23 (alrh)
  short  minute; // 60 minutes per hour max value 59 (alrm)
	uint32_t checksum; // crc32 checksum of our persisted data if this isn't matched we know we didn't write the data

}; // we should have about 512 bytes to use in EEPROM 128 + 32 + 2 + 1 + 1 + 4 == 168 leaving plenty of room to spare...

#define EEPROM_SIZE sizeof(EEPROMSettings) + 3 // 0: timezoneIndex, 1: fahrenheit or celsius, 2: have preferences been set, 3: last time we pulled a quote

#endif
