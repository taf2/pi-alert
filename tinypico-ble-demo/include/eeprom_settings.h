#ifndef _EEPROM_SETTINGS_H
#define _EEPROM_SETTINGS_H

#include <EEPROM.h> // saves the users selected timezone and preference for fahrenheit F or C

struct EEPROMSettings {
  EEPROMSettings();

  int load();
  int save();

  // IMPORTANT we can't reorder these since we're reading/writing this into EEPROM

  bool configured;
  char ssid[32]; // wifi ssid
  char pass[32]; // password ssid
};

#define EEPROM_SIZE sizeof(EEPROMSettings) + 3 // 0: timezoneIndex, 1: fahrenheit or celsius, 2: have preferences been set, 3: last time we pulled a quote

#endif
