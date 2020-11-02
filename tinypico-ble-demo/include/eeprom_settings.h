#ifndef _EEPROM_SETTINGS_H
#define _EEPROM_SETTINGS_H

#include <EEPROM.h> // saves the users selected timezone and preference for fahrenheit F or C

struct EEPROMSettings {
  EEPROMSettings();

	// load returns true only if the crc32 matches the first time a device loads before ever saving this would return false
  bool load();
	// writes this datastructure to eeprom with a crc32 checksum
  bool save();
	// checks the data saved and confirms whether it looks reasonable and likely good for use e.g. the data is reasonable
	bool good();

  // IMPORTANT we can't reorder these since we're reading/writing this into EEPROM
  bool configured;
  char ssid[32]; // wifi ssid
  char pass[32]; // password ssid
  short  hour; // 24 hour clock max value 23 (alrh)
  short  minute; // 60 minutes per hour max value 59 (alrm)

private:
	uint32_t crc32();
	uint32_t checksum; // crc32 checksum of our persisted data if this isn't matched we know we didn't write the data
};

#define EEPROM_SIZE sizeof(EEPROMSettings) + 3 // 0: timezoneIndex, 1: fahrenheit or celsius, 2: have preferences been set, 3: last time we pulled a quote

#endif
