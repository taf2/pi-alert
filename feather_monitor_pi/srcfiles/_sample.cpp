#include <TinyPICO.h>
#include <WiFi.h>
#include <Wire.h>
#include <Arduino.h>

#include <SoftwareSerial.h>

#define VC0706_RESET 0x26
#define VC0706_GEN_VERSION 0x11
#define VC0706_SET_PORT 0x24
#define VC0706_READ_FBUF 0x32
#define VC0706_GET_FBUF_LEN 0x34
#define VC0706_FBUF_CTRL 0x36
#define VC0706_DOWNSIZE_CTRL 0x54
#define VC0706_DOWNSIZE_STATUS 0x55
#define VC0706_READ_DATA 0x30
#define VC0706_WRITE_DATA 0x31
#define VC0706_COMM_MOTION_CTRL 0x37
#define VC0706_COMM_MOTION_STATUS 0x38
#define VC0706_COMM_MOTION_DETECTED 0x39
#define VC0706_MOTION_CTRL 0x42
#define VC0706_MOTION_STATUS 0x43
#define VC0706_TVOUT_CTRL 0x44
#define VC0706_OSD_ADD_CHAR 0x45

#define VC0706_STOPCURRENTFRAME 0x0
#define VC0706_STOPNEXTFRAME 0x1
#define VC0706_RESUMEFRAME 0x3
#define VC0706_STEPFRAME 0x2

#define VC0706_640x480 0x00
#define VC0706_320x240 0x11
#define VC0706_160x120 0x22

#define VC0706_MOTIONCONTROL 0x0
#define VC0706_UARTMOTION 0x01
#define VC0706_ACTIVATEMOTION 0x01

#define VC0706_SET_ZOOM 0x52
#define VC0706_GET_ZOOM 0x53

#define CAMERABUFFSIZ 100
#define CAMERADELAY 10

/**************************************************************************/
/*!
    @brief Class for communicating with VC0706 cameras
*/
/**************************************************************************/
class Adafruit_VC0706 {
public:
#if defined(__AVR__) || defined(ESP8266)
  Adafruit_VC0706(SoftwareSerial *ser); // Constructor when using SoftwareSerial
#endif
  Adafruit_VC0706(HardwareSerial *ser); // Constructor when using HardwareSerial
  boolean begin(uint32_t baud = 38400);
  boolean reset(void);
  boolean TVon(void);
  boolean TVoff(void);
  boolean takePicture(void);
  uint8_t *readPicture(uint8_t n);
  boolean resumeVideo(void);
  uint32_t frameLength(void);
  char *getVersion(void);
  uint8_t available();
  uint8_t getDownsize(void);
  boolean setDownsize(uint8_t);
  uint8_t getImageSize();
  boolean setImageSize(uint8_t);
  boolean getMotionDetect();
  uint8_t getMotionStatus(uint8_t);
  boolean motionDetected();
  boolean setMotionDetect(boolean f);
  boolean setMotionStatus(uint8_t x, uint8_t d1, uint8_t d2);
  boolean cameraFrameBuffCtrl(uint8_t command);
  uint8_t getCompression();
  boolean setCompression(uint8_t c);

  boolean getPTZ(uint16_t &w, uint16_t &h, uint16_t &wz, uint16_t &hz,
                 uint16_t &pan, uint16_t &tilt);
  boolean setPTZ(uint16_t wz, uint16_t hz, uint16_t pan, uint16_t tilt);

  void OSD(uint8_t x, uint8_t y, char *s); // isnt supported by the chip :(

  char *setBaud9600();
  char *setBaud19200();
  char *setBaud38400();
  char *setBaud57600();
  char *setBaud115200();

private:
  uint8_t serialNum;
  uint8_t camerabuff[CAMERABUFFSIZ + 1];
  uint8_t bufferLen;
  uint16_t frameptr;

#if defined(__AVR__) || defined(ESP8266)
  SoftwareSerial *swSerial;
#endif
  HardwareSerial *hwSerial;

  void common_init(void);
  boolean runCommand(uint8_t cmd, uint8_t args[], uint8_t argn, uint8_t resp,
                     boolean flushflag = true);
  void sendCommand(uint8_t cmd, uint8_t args[], uint8_t argn);
  uint8_t readResponse(uint8_t numbytes, uint8_t timeout);
  boolean verifyResponse(uint8_t command);
  void printBuff(void);
};
#include <SPI.h>

const int TX_PIN = 25;
const int RX_PIN = 33;
const int LED_PIN = 26;


const char *ssid =  "<%= @config[:ssid] %>"; // Put your SSID here
const char *password =  "<%= @config[:pass] %>"; // Put your PASSWORD here

void connectToWiFi(const char * ssid, const char * pwd)
{
  int ledState = 0;

  Serial.println("Connecting to WiFi network: " + String(ssid));

  WiFi.begin(ssid, pwd);

  while (WiFi.status() != WL_CONNECTED) {
    // Blink LED while we're connecting:
    digitalWrite(LED_PIN, ledState);
    ledState = (ledState + 1) % 2; // Flip ledState
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}


void setup() {

  Wire.begin();

  Serial.begin(115200);
  Serial.println(F("Booting"));

  // initialize SPI:
  SPI.begin();
  SPI.setFrequency(4000000); //4MHz
  //
  //
  SoftwareSerial cameraconnection(TX_PIN, RX_PIN);

  connectToWiFi(ssid, password);

}

void loop() {
}


/*
 * There are more than one way to use a library... but for some reason platform io when including this library decides we also 
 * need sd card reader libraries compiled into our source code... we don't need an sd card reader... so we'll just copy
 * the code below directly into our file from https://raw.githubusercontent.com/adafruit/Adafruit-VC0706-Serial-Camera-Library/master/Adafruit_VC0706.cpp
 */


/***************************************************
  This is a library for the Adafruit TTL JPEG Camera (VC0706 chipset)

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/products/397

  These displays use Serial to communicate, 2 pins are required to interface

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ****************************************************/

// Initialization code used by all constructor types
void Adafruit_VC0706::common_init(void) {
#if defined(__AVR__) || defined(ESP8266)
  swSerial = NULL;
#endif
  hwSerial = NULL;
  frameptr = 0;
  bufferLen = 0;
  serialNum = 0;
}

#if defined(__AVR__) || defined(ESP8266)
/**************************************************************************/
/*!
    @brief Constructor when using SoftwareSerial
    @param ser Software serial connection
*/
/**************************************************************************/
Adafruit_VC0706::Adafruit_VC0706(SoftwareSerial *ser) {
  common_init();  // Set everything to common state, then...
  swSerial = ser; // ...override swSerial with value passed.
}
#endif

/**************************************************************************/
/*!
    @brief Constructor when using HardwareSerial
    @param ser Hardware serial connection
*/
/**************************************************************************/
Adafruit_VC0706::Adafruit_VC0706(HardwareSerial *ser) {
  common_init();  // Set everything to common state, then...
  hwSerial = ser; // ...override hwSerial with value passed.
}

/**************************************************************************/
/*!
    @brief Connect to and reset the camera
    @param baud Camera interface baud rate
    @return True on reset success
*/
/**************************************************************************/
boolean Adafruit_VC0706::begin(uint32_t baud) {
#if defined(__AVR__) || defined(ESP8266)
  if (swSerial)
    swSerial->begin(baud);
  else
#endif
    hwSerial->begin(baud);
  return reset();
}

/**************************************************************************/
/*!
    @brief  Soft reset the camera
    @return True on success
*/
/**************************************************************************/
boolean Adafruit_VC0706::reset() {
  uint8_t args[] = {0x0};

  return runCommand(VC0706_RESET, args, 1, 5);
}

/**************************************************************************/
/*!
    @brief  Check if motion is detected
    @return True on motion detected
*/
/**************************************************************************/
boolean Adafruit_VC0706::motionDetected() {
  if (readResponse(4, 200) != 4) {
    return false;
  }
  if (!verifyResponse(VC0706_COMM_MOTION_DETECTED))
    return false;

  return true;
}

/**************************************************************************/
/*!
    @brief  Set motion detection
    @param x See datasheet for VC0706_MOTION_CTRL command details
    @param d1 See datasheet for VC0706_MOTION_CTRL command details
    @param d2 See datasheet for VC0706_MOTION_CTRL command details
    @return True on command success
*/
/**************************************************************************/
boolean Adafruit_VC0706::setMotionStatus(uint8_t x, uint8_t d1, uint8_t d2) {
  uint8_t args[] = {0x03, x, d1, d2};

  return runCommand(VC0706_MOTION_CTRL, args, sizeof(args), 5);
}

/**************************************************************************/
/*!
    @brief  Get motion status
    @param x See datasheet for VC0706_MOTION_STATUS command details
    @return True on command success
*/
/**************************************************************************/
uint8_t Adafruit_VC0706::getMotionStatus(uint8_t x) {
  uint8_t args[] = {0x01, x};

  return runCommand(VC0706_MOTION_STATUS, args, sizeof(args), 5);
}

/**************************************************************************/
/*!
    @brief Set motion detection
    @param flag Whether to activate motion detection
    @return True on command success
*/
/**************************************************************************/
boolean Adafruit_VC0706::setMotionDetect(boolean flag) {
  if (!setMotionStatus(VC0706_MOTIONCONTROL, VC0706_UARTMOTION,
                       VC0706_ACTIVATEMOTION))
    return false;

  uint8_t args[] = {0x01, flag};

  runCommand(VC0706_COMM_MOTION_CTRL, args, sizeof(args), 5);
}

/**************************************************************************/
/*!
    @brief  Get motion detection status
    @return True if motion detection is on!
*/
/**************************************************************************/
boolean Adafruit_VC0706::getMotionDetect(void) {
  uint8_t args[] = {0x0};

  if (!runCommand(VC0706_COMM_MOTION_STATUS, args, 1, 6))
    return false;

  return camerabuff[5];
}

/**************************************************************************/
/*!
    @brief  Get image size with VC0706_READ_DATA
    @return VC0706_640x480, VC0706_320x240 or VC0706_160x120
*/
/**************************************************************************/
uint8_t Adafruit_VC0706::getImageSize() {
  uint8_t args[] = {0x4, 0x4, 0x1, 0x00, 0x19};
  if (!runCommand(VC0706_READ_DATA, args, sizeof(args), 6))
    return -1;

  return camerabuff[5];
}

/**************************************************************************/
/*!
    @brief  Set image size with VC0706_WRITE_DATA
    @param x VC0706_640x480, VC0706_320x240 or VC0706_160x120
    @return True on command success
*/
/**************************************************************************/
boolean Adafruit_VC0706::setImageSize(uint8_t x) {
  uint8_t args[] = {0x05, 0x04, 0x01, 0x00, 0x19, x};

  return runCommand(VC0706_WRITE_DATA, args, sizeof(args), 5);
}

/****************** downsize image control */

/**************************************************************************/
/*!
    @brief  Get downsize Status
    @return Camera reply byte
*/
/**************************************************************************/
uint8_t Adafruit_VC0706::getDownsize(void) {
  uint8_t args[] = {0x0};
  if (!runCommand(VC0706_DOWNSIZE_STATUS, args, 1, 6))
    return -1;

  return camerabuff[5];
}

/**************************************************************************/
/*!
    @brief  Set downsize size
    @param newsize See datasheet for VC0706_DOWNSIZE_CTRL parameters
    @return True on command success
*/
/**************************************************************************/
boolean Adafruit_VC0706::setDownsize(uint8_t newsize) {
  uint8_t args[] = {0x01, newsize};

  return runCommand(VC0706_DOWNSIZE_CTRL, args, 2, 5);
}

/***************** other high level commands */

/**************************************************************************/
/*!
    @brief Get firmware version
    @returns Pointer to buffer containing camera response
*/
/**************************************************************************/
char *Adafruit_VC0706::getVersion(void) {
  uint8_t args[] = {0x01};

  sendCommand(VC0706_GEN_VERSION, args, 1);
  // get reply
  if (!readResponse(CAMERABUFFSIZ, 200))
    return 0;
  camerabuff[bufferLen] = 0; // end it!
  return (char *)camerabuff; // return it!
}

/**************************************************************************/
/*!
    @brief  Set the baud rate to 9600
    @return The response as a character buffer
*/
/**************************************************************************/
char *Adafruit_VC0706::setBaud9600() {
  uint8_t args[] = {0x03, 0x01, 0xAE, 0xC8};

  sendCommand(VC0706_SET_PORT, args, sizeof(args));
  // get reply
  if (!readResponse(CAMERABUFFSIZ, 200))
    return 0;
  camerabuff[bufferLen] = 0; // end it!
  return (char *)camerabuff; // return it!
}

/**************************************************************************/
/*!
    @brief  Set the baud rate to 19200
    @return The response as a character buffer
*/
/**************************************************************************/
char *Adafruit_VC0706::setBaud19200() {
  uint8_t args[] = {0x03, 0x01, 0x56, 0xE4};

  sendCommand(VC0706_SET_PORT, args, sizeof(args));
  // get reply
  if (!readResponse(CAMERABUFFSIZ, 200))
    return 0;
  camerabuff[bufferLen] = 0; // end it!
  return (char *)camerabuff; // return it!
}

/**************************************************************************/
/*!
    @brief  Set the baud rate to 38400
    @return The response as a character buffer
*/
/**************************************************************************/
char *Adafruit_VC0706::setBaud38400() {
  uint8_t args[] = {0x03, 0x01, 0x2A, 0xF2};

  sendCommand(VC0706_SET_PORT, args, sizeof(args));
  // get reply
  if (!readResponse(CAMERABUFFSIZ, 200))
    return 0;
  camerabuff[bufferLen] = 0; // end it!
  return (char *)camerabuff; // return it!
}

/**************************************************************************/
/*!
    @brief  Set the baud rate to 57600
    @return The response as a character buffer
*/
/**************************************************************************/
char *Adafruit_VC0706::setBaud57600() {
  uint8_t args[] = {0x03, 0x01, 0x1C, 0x1C};

  sendCommand(VC0706_SET_PORT, args, sizeof(args));
  // get reply
  if (!readResponse(CAMERABUFFSIZ, 200))
    return 0;
  camerabuff[bufferLen] = 0; // end it!
  return (char *)camerabuff; // return it!
}

/**************************************************************************/
/*!
    @brief  Set the baud rate to 115200
    @return The response as a character buffer
*/
/**************************************************************************/
char *Adafruit_VC0706::setBaud115200() {
  uint8_t args[] = {0x03, 0x01, 0x0D, 0xA6};

  sendCommand(VC0706_SET_PORT, args, sizeof(args));
  // get reply
  if (!readResponse(CAMERABUFFSIZ, 200))
    return 0;
  camerabuff[bufferLen] = 0; // end it!
  return (char *)camerabuff; // return it!
}

/**************************************************************************/
/*!
    @brief  Add a character to on screen display
    @param x X offset
    @param y Y offset
    @param str The string to display
*/
/**************************************************************************/
void Adafruit_VC0706::OSD(uint8_t x, uint8_t y, char *str) {
  if (strlen(str) > 14) {
    str[13] = 0;
  }

  uint8_t args[17] = {strlen(str), strlen(str) - 1,
                      (y & 0xF) | ((x & 0x3) << 4)};

  for (uint8_t i = 0; i < strlen(str); i++) {
    char c = str[i];
    if ((c >= '0') && (c <= '9')) {
      str[i] -= '0';
    } else if ((c >= 'A') && (c <= 'Z')) {
      str[i] -= 'A';
      str[i] += 10;
    } else if ((c >= 'a') && (c <= 'z')) {
      str[i] -= 'a';
      str[i] += 36;
    }

    args[3 + i] = str[i];
  }

  runCommand(VC0706_OSD_ADD_CHAR, args, strlen(str) + 3, 5);
  printBuff();
}

/**************************************************************************/
/*!
    @brief Set compression rate
    @param c See datasheet for compression settings
    @returns True on command success
*/
/**************************************************************************/
boolean Adafruit_VC0706::setCompression(uint8_t c) {
  uint8_t args[] = {0x5, 0x1, 0x1, 0x12, 0x04, c};
  return runCommand(VC0706_WRITE_DATA, args, sizeof(args), 5);
}

/**************************************************************************/
/*!
    @brief Get compression rate
    @returns The character reply
*/
/**************************************************************************/
uint8_t Adafruit_VC0706::getCompression(void) {
  uint8_t args[] = {0x4, 0x1, 0x1, 0x12, 0x04};
  runCommand(VC0706_READ_DATA, args, sizeof(args), 6);
  printBuff();
  return camerabuff[5];
}

/**************************************************************************/
/*!
    @brief Set PTZ (orientation), see datasheet for SET_ZOOM command
    @param wz Rotation
    @param hz Horizontal
    @param pan Pan
    @param tilt Tilt

    @returns True on command success
*/
/**************************************************************************/
boolean Adafruit_VC0706::setPTZ(uint16_t wz, uint16_t hz, uint16_t pan,
                                uint16_t tilt) {
  uint8_t args[] = {0x08,     wz >> 8, wz,        hz >> 8, wz,
                    pan >> 8, pan,     tilt >> 8, tilt};

  return (!runCommand(VC0706_SET_ZOOM, args, sizeof(args), 5));
}

/**************************************************************************/
/*!
    @brief Get PTZ (orientation), see datasheet for GET_ZOOM command
    @param w Pointer to variable to store width
    @param h Pointer to variable to store height
    @param wz Pointer to variable to store rotation
    @param hz Pointer to variable to store horizontal
    @param pan Pointer to variable to store pan
    @param tilt Pointer to variable to store tilt
    @returns True on command success
*/
/**************************************************************************/
boolean Adafruit_VC0706::getPTZ(uint16_t &w, uint16_t &h, uint16_t &wz,
                                uint16_t &hz, uint16_t &pan, uint16_t &tilt) {
  uint8_t args[] = {0x0};

  if (!runCommand(VC0706_GET_ZOOM, args, sizeof(args), 16))
    return false;
  printBuff();

  w = camerabuff[5];
  w <<= 8;
  w |= camerabuff[6];

  h = camerabuff[7];
  h <<= 8;
  h |= camerabuff[8];

  wz = camerabuff[9];
  wz <<= 8;
  wz |= camerabuff[10];

  hz = camerabuff[11];
  hz <<= 8;
  hz |= camerabuff[12];

  pan = camerabuff[13];
  pan <<= 8;
  pan |= camerabuff[14];

  tilt = camerabuff[15];
  tilt <<= 8;
  tilt |= camerabuff[16];

  return true;
}

/**************************************************************************/
/*!
    @brief Send STOPCURRENTFRAME command
    @returns True on command success
*/
/**************************************************************************/
boolean Adafruit_VC0706::takePicture() {
  frameptr = 0;
  return cameraFrameBuffCtrl(VC0706_STOPCURRENTFRAME);
}

/**************************************************************************/
/*!
    @brief Send RESUMEFRAME command
    @returns True on command success
*/
/**************************************************************************/
boolean Adafruit_VC0706::resumeVideo() {
  return cameraFrameBuffCtrl(VC0706_RESUMEFRAME);
}

/**************************************************************************/
/*!
    @brief TV output ON
    @returns True on command success
*/
/**************************************************************************/
boolean Adafruit_VC0706::TVon() {
  uint8_t args[] = {0x1, 0x1};
  return runCommand(VC0706_TVOUT_CTRL, args, sizeof(args), 5);
}

/**************************************************************************/
/*!
    @brief TV output OFF
    @returns True on command success
*/
/**************************************************************************/
boolean Adafruit_VC0706::TVoff() {
  uint8_t args[] = {0x1, 0x0};
  return runCommand(VC0706_TVOUT_CTRL, args, sizeof(args), 5);
}

/**************************************************************************/
/*!
    @brief Send FBUF_CTRL with command
    @param command The FBUF control command (see datasheet)
    @returns True on command success
*/
/**************************************************************************/
boolean Adafruit_VC0706::cameraFrameBuffCtrl(uint8_t command) {
  uint8_t args[] = {0x1, command};
  return runCommand(VC0706_FBUF_CTRL, args, sizeof(args), 5);
}

/**************************************************************************/
/*!
    @brief Get frame buffer length
    @returns Length reply
*/
/**************************************************************************/
uint32_t Adafruit_VC0706::frameLength(void) {
  uint8_t args[] = {0x01, 0x00};
  if (!runCommand(VC0706_GET_FBUF_LEN, args, sizeof(args), 9))
    return 0;

  uint32_t len;
  len = camerabuff[5];
  len <<= 8;
  len |= camerabuff[6];
  len <<= 8;
  len |= camerabuff[7];
  len <<= 8;
  len |= camerabuff[8];

  return len;
}

/**************************************************************************/
/*!
    @brief Get available bytes to read
    @returns Internal buffer length
*/
/**************************************************************************/
uint8_t Adafruit_VC0706::available(void) { return bufferLen; }

/**************************************************************************/
/*!
    @brief Read in picture data
    @param n Number of bytes
    @returns Pointer to buffer containing n bytes of picture data
*/
/**************************************************************************/
uint8_t *Adafruit_VC0706::readPicture(uint8_t n) {
  uint8_t args[] = {0x0C,
                    0x0,
                    0x0A,
                    0,
                    0,
                    frameptr >> 8,
                    frameptr & 0xFF,
                    0,
                    0,
                    0,
                    n,
                    CAMERADELAY >> 8,
                    CAMERADELAY & 0xFF};

  if (!runCommand(VC0706_READ_FBUF, args, sizeof(args), 5, false))
    return 0;

  // read into the buffer PACKETLEN!
  if (readResponse(n + 5, CAMERADELAY) == 0)
    return 0;

  frameptr += n;

  return camerabuff;
}

/**************** low level commands */

boolean Adafruit_VC0706::runCommand(uint8_t cmd, uint8_t *args, uint8_t argn,
                                    uint8_t resplen, boolean flushflag) {
  // flush out anything in the buffer?
  if (flushflag) {
    readResponse(100, 10);
  }

  sendCommand(cmd, args, argn);
  if (readResponse(resplen, 200) != resplen)
    return false;
  if (!verifyResponse(cmd))
    return false;
  return true;
}

void Adafruit_VC0706::sendCommand(uint8_t cmd, uint8_t args[] = 0,
                                  uint8_t argn = 0) {
#if defined(__AVR__) || defined(ESP8266)
  if (swSerial) {

    swSerial->write((byte)0x56);
    swSerial->write((byte)serialNum);
    swSerial->write((byte)cmd);

    for (uint8_t i = 0; i < argn; i++) {
      swSerial->write((byte)args[i]);
      // Serial.print(" 0x");
      // Serial.print(args[i], HEX);
    }
  } else
#endif
  {
    hwSerial->write((byte)0x56);
    hwSerial->write((byte)serialNum);
    hwSerial->write((byte)cmd);

    for (uint8_t i = 0; i < argn; i++) {
      hwSerial->write((byte)args[i]);
      // Serial.print(" 0x");
      // Serial.print(args[i], HEX);
    }
  }
  // Serial.println();
}

uint8_t Adafruit_VC0706::readResponse(uint8_t numbytes, uint8_t timeout) {
  uint8_t counter = 0;
  bufferLen = 0;
  int avail;

  while ((timeout != counter) && (bufferLen != numbytes)) {
#if defined(__AVR__) || defined(ESP8266)
    avail = swSerial ? swSerial->available() : hwSerial->available();
#else
    avail = hwSerial->available();
#endif
    if (avail <= 0) {
      delay(1);
      counter++;
      continue;
    }
    counter = 0;
    // there's a byte!
#if defined(__AVR__) || defined(ESP8266)
    camerabuff[bufferLen++] = swSerial ? swSerial->read() : hwSerial->read();
#else
    camerabuff[bufferLen++] = hwSerial->read();
#endif
  }
  // printBuff();
  // camerabuff[bufferLen] = 0;
  // Serial.println((char*)camerabuff);
  return bufferLen;
}

boolean Adafruit_VC0706::verifyResponse(uint8_t command) {
  if ((camerabuff[0] != 0x76) || (camerabuff[1] != serialNum) ||
      (camerabuff[2] != command) || (camerabuff[3] != 0x0))
    return false;
  return true;
}

void Adafruit_VC0706::printBuff() {
  for (uint8_t i = 0; i < bufferLen; i++) {
    Serial.print(" 0x");
    Serial.print(camerabuff[i], HEX);
  }
  Serial.println();
}
