#ifndef __EEPROM_WRITE_ANYTHING_H
#define __EEPROM_WRITE_ANYTHING_H
// see: https://forum.arduino.cc/index.php?topic=52159.0

template <class T> int EEPROM_writeAnything(int ee, const T& value) {
  const byte* p = (const byte*)(const void*)&value;
  int i;
  for (i = 0; i < sizeof(value); i++)
  EEPROM.write(ee++, *p++);
  return i;
}

template <class T> int EEPROM_readAnything(int ee, T& value) {
  byte* p = (byte*)(void*)&value;
  int i;
  for (i = 0; i < sizeof(value); i++)
  *p++ = EEPROM.read(ee++);
  return i;
}

#endif
