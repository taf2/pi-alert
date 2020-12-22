/* 
	Going to also have an epaper device now to display softer clock face we'll need to only update each minute to throttle 
	writes to the epaper since updates are expensive to the display

	the oled display will continue to be attached and we'll have a night option since clocks suck at night they're too bright
	we'll have the primary display be an epaper 5.65 " display with epaper, we'll dialy update the quote to inspire everyone
	*/
//#define ENABLE_EPAPER 1
#include <time.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

// add ble bluetooth for easy configuration
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include "eeprom_settings.h"

#define VBATPIN A13
#define EPAPER_POWER_ON 12 // control power to the display epaper control ItsyBitsy board 
#define LED_BLUE_CONFIG 13
#define USER2_BUTTON_PIN 14 // external user interface button to toggle on / off alarm snooze function e.g. trigger alarm again in 5 minutes if no user input on alarm cancel taken
#define ALARM_BUTTON_LED 15
#define USER_BUTTON_PIN 21 // external user interface button to toggle on / off alarm
#define MP3_PWR 27 // connected to the enable pin on the buck converter 3.3v to mp3 df mini player
#define SNOOZE_BUTTON_LED 32
#define BUZZER 33

#define SNOOZE_SECONDS 30 // seconds that snooze will prevent alarm from re-firing

// ePaper pins in addition to MOSI (DIN) and SCK (CLK)
// we can't use A3 or A4 since they have to be output capable and are not on the feather esp32
#define CS_PIN          A5 // must be output capable
#define DC_PIN          A1 // must be output capable
#define RST_PIN         A0 // must be output capable and is an INPUT_PULLUP
#define BUSY_PIN        A2 // must be input capable

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
#define SERVICE_UUID           "e2f52832-2c23-4318-85cb-be11f7421999" //"6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "e5de2fab-bf9b-439d-81d1-24651d8201a7" //"6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "e6f61157-5e3a-4656-a412-de8610a63d76" //"6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// OLED FeatherWing buttons map to different pins depending on board:
#if defined(ESP8266)
  #define BUTTON_A  0
  #define BUTTON_B 16
  #define BUTTON_C  2
#elif defined(ESP32)
  #define BUTTON_A 15
  #define BUTTON_B 32
  #define BUTTON_C 14
#elif defined(ARDUINO_STM32_FEATHER)
  #define BUTTON_A PA15
  #define BUTTON_B PC7
  #define BUTTON_C PC5
#elif defined(TEENSYDUINO)
  #define BUTTON_A  4
  #define BUTTON_B  3
  #define BUTTON_C  8
#elif defined(ARDUINO_FEATHER52832)
  #define BUTTON_A 31
  #define BUTTON_B 30
  #define BUTTON_C 27
#else // 32u4, M0, M4, nrf52840 and 328p
  #define BUTTON_A  9
  #define BUTTON_B  6
  #define BUTTON_C  5
#endif

static short displayTime(short needUpdate, time_t &lastSecond, bool normalDisplay=true);
static bool initWiFi(const char *ssid, const char *password);
static bool initClockAndWifi();

static volatile time_t currentSecond = 0;
static time_t displayUpdateStartTime = 0; // when did we start a display update
static time_t prevSecond = 0;
static time_t lastSecond    = 0;
static bool snoozeActivated = false;
static bool didSnoozeStart = false;
static time_t snoozedAt = 0;

static const short OUT_BUFFER_SIZE = 512;
static char buffer[OUT_BUFFER_SIZE];

static int last_sent_key = 0;
static const int BLE_KEY_CONFIG_SIZE = 7;
static const char *BLE_SET_CONFIG_KEYS[] =  {"ssid", "pass", "alrh", "alrm", "dtim", "zip", "api"}; // no keys longer than 4
static bool deviceConnected = false;
static bool ConfigMode = false; //  when pressed we'll enter configuration mode
static bool DidInitWifi = false;
static bool SongActive = false;
static time_t StopSongTime = 0; // the time that it was stopped

static BLEService *pService;
static BLEServer *pServer;
static BLECharacteristic *pCharacteristic;

static WiFiUDP ntpUDP;
static NTPClient timeClient(ntpUDP);

static EEPROMSettings settings;

class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
  };

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
  }
};

class MyCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string rxValue = pCharacteristic->getValue();

    if (rxValue.length() > 0) {
      Serial.println("*********");
      Serial.print("Received Value: ");
      // expecting key:value pairs
      String key, value;
      bool isKey = true;

      for (int i = 0; i < rxValue.length(); i++) {
        Serial.print(rxValue[i]);
        if (rxValue[i] == ':') {
          isKey = false;
        } else {
          if (isKey) {
            key +=  rxValue[i];
          } else {
            value += rxValue[i];
          }
        }
      }

      Serial.println();
      if (key.length() > 0 && key == "ssid" && value.length() > 0) {
        Serial.println( (key + " = " + value).c_str() );
        memcpy(settings.ssid, value.c_str(), 32);
        settings.configured = true; 
        settings.save();
        if (strlen(settings.pass) > 0 && !DidInitWifi) {
          initWiFi(settings.ssid, settings.pass);
          // try to init wifi
        }
      } else if (key.length() > 0 && key == "pass" && value.length() > 0) {
        Serial.println( ( key + " = " + value).c_str() );
        memcpy(settings.pass, value.c_str(), 32);
        settings.configured = true; 
        settings.save();
        if (strlen(settings.pass) > 0 && !DidInitWifi) {
          initWiFi(settings.ssid, settings.pass);
          // try to init wifi
        }
      } else if (key.length() > 0 && key == "alrh" && value.length() > 0) {
        Serial.println( ( key + " = " + value).c_str() );
        settings.hour = atoi(value.c_str());
        StopSongTime = 0; // reset alarm stop
        snoozedAt = 0;
        snoozeActivated = false;
        didSnoozeStart = false;
        settings.save();
        Serial.print("hour set:");
        Serial.println(settings.hour);
      } else if (key.length() > 0 && key == "alrm" && value.length() > 0) {
        Serial.println( ( key + " = " + value).c_str() );
        settings.minute = atoi(value.c_str());
        StopSongTime = 0; // reset alarm stop
        snoozedAt = 0;
        snoozeActivated = false;
        didSnoozeStart = false;
        settings.save();
        Serial.print("minute set:");
        Serial.println(settings.minute);
      } else if (key.length() > 0 && key == "zip" && value.length() > 0 && value.length() < 6) {
        Serial.println( ( key + " = " + value).c_str() );
        memcpy(settings.zipcode, value.c_str(), 8);
        settings.configured = true; 
        settings.save();
      } else if (key.length() > 0 && key == "api" && value.length() > 0) {
        Serial.println( ( key + " = " + value).c_str() );
        memcpy(settings.weatherAPI, value.c_str(), 34);
        settings.configured = true; 
        settings.save();
      }

      Serial.println();
      Serial.println("*********");
    }
  }
};

static MyServerCallbacks *callbacks = NULL;

bool initWiFi(const char *ssid, const char *password) {
  IPAddress ip;

  wl_status_t status = WiFi.begin(ssid, password);
  if (status == WL_CONNECT_FAILED) {
    Serial.println("Connection Error!!!!!!!!!");
    snprintf(buffer, OUT_BUFFER_SIZE, "Failed to connect to %s \\", ssid);
    Serial.println(buffer);
    return false;
  }
  snprintf(buffer, OUT_BUFFER_SIZE, "Connecting to %s \\", ssid);
  Serial.println(buffer);
  int count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    char c = '/';
    if (count % 2 == 0) {
      c = '/';
    } else {
      c = '\\';
    }
    Serial.print(c);
    yield();
    ++count;
  }
  ip = WiFi.localIP();
  Serial.println(ip);
  snprintf(buffer, OUT_BUFFER_SIZE, "Connected to %s with %s\n", ssid, ip.toString().c_str());
  Serial.print(buffer);
  delay(2000);

  // initialize time
  DidInitWifi = true;
  return true;
}

void startSong() {
  gpio_hold_dis((gpio_num_t)EPAPER_POWER_ON);
  gpio_hold_dis((gpio_num_t)MP3_PWR);
  digitalWrite(EPAPER_POWER_ON, HIGH);
  digitalWrite(MP3_PWR, HIGH); // send power to the device
  // it takes sometime for the relay to warm up to give power to the device... we need to delay here to ensure it had time 
  delay(1000);
  digitalWrite(BUZZER, LOW); // play music
  delay(100);
  digitalWrite(BUZZER, HIGH); // play music
  SongActive  = true;
  StopSongTime = 0;
}
void stopSong(bool report=true) {
  digitalWrite(MP3_PWR, LOW); // CUT the power
  Serial.println("stopSong");
  if (report) {
    Serial.println("user stop alarm");
    StopSongTime = currentSecond;
  }
  digitalWrite(ALARM_BUTTON_LED, LOW);
  digitalWrite(SNOOZE_BUTTON_LED, LOW);
  if (didSnoozeStart) {
    Serial.println("reset snooze");
    // reset all the snooze
    didSnoozeStart = false;
    snoozeActivated = false;
    snoozedAt = 0;
  }
  SongActive   = false;
}

void initButtons() {
  pinMode(USER_BUTTON_PIN, INPUT_PULLDOWN);
  pinMode(USER2_BUTTON_PIN, INPUT_PULLDOWN);
  pinMode(ALARM_BUTTON_LED, OUTPUT);
  pinMode(SNOOZE_BUTTON_LED, OUTPUT);
  digitalWrite(ALARM_BUTTON_LED, LOW);
  digitalWrite(SNOOZE_BUTTON_LED, LOW);
}

void enableBLE() {
  // Create the BLE Device
  //  
  //WiFi.mode(WIFI_OFF);
  //btStop(); //??

  Serial.println("enableBLE");
  if (!BLEDevice::getInitialized()) {
    Serial.println("init start");
    BLEDevice::init("Clock"); // Give it a name
    Serial.println("init complete");

    // Create the BLE Server
    pServer = BLEDevice::createServer();
    Serial.println("server created");
    callbacks = new MyServerCallbacks();
    pServer->setCallbacks(callbacks);
    Serial.println("created pServer");

    // Create the BLE Service
    pService = pServer->createService(SERVICE_UUID);

    Serial.println("created pService");
    BLEDescriptor client_characteristic_configuration(BLEUUID((uint16_t)0x2902));

    // Create a BLE Characteristic
    pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX,BLECharacteristic::PROPERTY_NOTIFY);

    pCharacteristic->addDescriptor(new BLE2902());
    //pCharacteristic->addDescriptor(&client_characteristic_configuration);

    BLECharacteristic *pRxC = pService->createCharacteristic(CHARACTERISTIC_UUID_RX,BLECharacteristic::PROPERTY_WRITE);

    // gatt.client_characteristic_configuration
    //pRxC->addDescriptor(new BLE2902());
    pRxC->addDescriptor(&client_characteristic_configuration);

    pRxC->setCallbacks(new MyCallbacks());

    Serial.println("start server");

    // Start the service
    pService->start();
  }

  Serial.println("start advertising");

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
  digitalWrite(LED_BLUE_CONFIG, HIGH);

  ConfigMode = true;
}

bool initClockAndWifi() {
  Serial.println("wifi is configured start connecting...");
  Serial.println(settings.ssid);
  if (!initWiFi(settings.ssid, settings.pass)) {
    Serial.println("!!!!!!!!Error Connecting with WIFI!!!!!!!!");
    return false;
  }
  Serial.println("Wifi connected. Loading time");

  // initialize time
  timeClient.begin();

//  strncpy(settings.zipcode, "<%= @config[:zipcode] %>", 6);
//  strncpy(settings.weatherAPI, "<%= @config[:weatherAPI] %>", 33);
	int offsetSeconds = settings.timezoneOffset();
	Serial.print("set timezone offset:");
	Serial.println(offsetSeconds);
	timeClient.setTimeOffset(offsetSeconds);
  Serial.println("offset now update");
  timeClient.update();
  Serial.println("called update");
  currentSecond = timeClient.getEpochTime(); // set globally
  Serial.println("received current second");

  return true;
}

int buttonPressedCount = 0;
void setup() {

  Serial.begin(115200);
  while (!Serial) delay(10);     // will pause Zero, Leonardo, etc until serial console opens

	EEPROM.begin(EEPROM_SIZE);

  pinMode(EPAPER_POWER_ON, OUTPUT);
  pinMode(LED_BLUE_CONFIG, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(MP3_PWR, OUTPUT);
  digitalWrite(BUZZER, HIGH); // ensure 
  digitalWrite(MP3_PWR, LOW);
  digitalWrite(LED_BLUE_CONFIG, LOW);

  initButtons();

  Serial1.begin(9600); // communications to the epaper ItsyBitsy
  digitalWrite(EPAPER_POWER_ON, HIGH); // power up the device

  delay(3000);
  if (settings.load() && settings.good()) {
    Serial.println("config seems good so we're gonna try to configure wifi");
    initClockAndWifi();
    enableBLE();
  } else {
    Serial.println("settings not good yet turn on bluetooth so we can get user settings");
    enableBLE();
  }

  if (digitalRead(MP3_PWR)) {
    Serial.println("we see mp3 is running maybe we fell asleep and started again?");
    SongActive = true; // light sleep restore SongActive state
  }

}

// returns 0 no updates, returns 1 only lcd updates, returns 2 both lcd and epaper updates
// normalDisplay is so we can run this in the main loop for an lcd but also in an epaper loop
//int displayTime(short needUpdate, NTPClient &tc, time_t &lastSecond, bool normalDisplay) {
short displayTime(short needUpdate, time_t &lastSecond, bool normalDisplay) {
  time_t rawtime = currentSecond;//tc.getEpochTime();
  int rawMinute = (int)(rawtime / 60);
  int lastMinute = (int)(lastSecond / 60);
  int secondsInMinute = (int)(rawtime % 60);

  // the number of seconds past since we asked the epaper display to start updating
  int drawingSeconds = (int)(currentSecond - displayUpdateStartTime);

  /* to optimize power consumption we'll compute needPower as a function of time
   * e.g. if it's 20 seconds before the top of the minute or 20 seconds after the
   * top of the minute we'll want to keep power ON otherwise we can keep power off 
   */
  bool needPower = (needUpdate > 0) || (secondsInMinute > 49 || secondsInMinute < 21) || (drawingSeconds < 20);
  // when it needs power
  if (needPower || SongActive || deviceConnected) { // power on within the update window or if we're playing the music for the alarm
    gpio_hold_dis((gpio_num_t)EPAPER_POWER_ON);
    digitalWrite(EPAPER_POWER_ON, HIGH);
  } else {
  // when it's safe to turn it off
    digitalWrite(EPAPER_POWER_ON, LOW);
    gpio_hold_en((gpio_num_t)EPAPER_POWER_ON);
    gpio_hold_en((gpio_num_t)MP3_PWR);
    // light sleep until 10 seconds before the top of the minute
    int sleepFor = 50 - secondsInMinute;
    if (sleepFor > 10) { //  don't bother if it's less then 10 seconds
      Serial.println("light sleep");
      // the problem with doing this sleep is then we can't power off easily on the epaper board and it makes a crackling sound with the df mini player gets current
      // esp_sleep_enable_ext0_wakeup // use this to allow disabling this power saving feature by pressing a button to keep ble / wifi on for configuration
      esp_sleep_enable_timer_wakeup( sleepFor * 1000000ULL);
      esp_light_sleep_start();
    }
  }

  if (rawMinute > lastMinute) {
    needUpdate = 1;
  }

  if (needUpdate) {
    bool epaper_update = (rawMinute > lastMinute);
    //snprintf(buffer, 1024,
    //         "rawtime: %ld, lastSecond: %ld, rm: %d lm: %d\n", rawtime, lastSecond, rawMinute, lastMinute);
    //Serial.print(buffer);
    struct tm  ts;
    ts = *localtime(&rawtime);
    lastSecond = rawtime;

    //  the epaper display is too big for us to also include on this board so we'll write to serial here instead
    Serial.println("write to Serial1");
    StaticJsonDocument<1024> doc;
    doc["time"] = currentSecond;
    doc["temp"] = settings.temp;
    doc["quote"] = settings.quote;
    doc["author"] = settings.author;
    size_t bytesWritten = serializeJson(doc, Serial1);
    Serial.println(bytesWritten);
    displayUpdateStartTime = currentSecond;

    if (epaper_update) {
      needUpdate = 2;
      return needUpdate;
    }
    return needUpdate;
  } else if (normalDisplay) {
    struct tm  ts;
    ts = *localtime(&rawtime);
    lastSecond = rawtime;
  }
  return needUpdate;
}

void displayClock() {
  short needUpdate = 0;

  needUpdate = displayTime(needUpdate, lastSecond);
  if (needUpdate == 2) {
    // this means at least 1 minute has past since the last update so we'll do things here like also checking alarms and quote updates
    // as well as updating our epaper if it's enabled
    Serial.println("check for updated quote");
    settings.loadQuote(timeClient);
  }
}

void checkAlarm() {
  currentSecond = timeClient.getEpochTime(); // set globally
 
  const int   currentDay        = currentSecond / 24 / 60 / 60; // convert seconds to the day
  const short hour              = settings.hour;
  const short minute            = settings.minute;

  // this gets us the starting second of the current day
  const int startOfDayInSeconds = currentDay * 24 * 60 * 60;

  // startTimeToAlarm either relative to the snoozedAt + SNOOZE_SECONDS or user set time to alarm
  const int startTimeToAlarm    = snoozeActivated ? (snoozedAt + SNOOZE_SECONDS) : (startOfDayInSeconds + (hour*60*60) + (minute*60));
  const int endTimeToAlarm      = startOfDayInSeconds + (hour*60*60) + ((minute+4)*60);
                                  // 4 minutes of padding for song to play

  //snprintf(buffer, OUT_BUFFER_SIZE, "epoch: %ld (%ld), %d, startTimeToAlarm: %d, endTimeToAlarm: %d\n", currentSecond, currentSecond, offsetSeconds, startTimeToAlarm, endTimeToAlarm);
//  Serial.println(buffer);

  if (currentSecond > startTimeToAlarm && currentSecond < endTimeToAlarm) {
    if (SongActive) {
      digitalWrite(SNOOZE_BUTTON_LED, HIGH); // light up the snooze button
      // blink BLUE button LED for alarm 1 second blinkage
      if (currentSecond % 2 == 0) {
        digitalWrite(ALARM_BUTTON_LED, HIGH);
      } else {
        digitalWrite(ALARM_BUTTON_LED, LOW);
      }
    } else {
      if (StopSongTime < startTimeToAlarm) {
        if (snoozeActivated) {
          didSnoozeStart = true; // ensure the snooze feature is turned off when stop is called
        }
        Serial.println("sound the alarm");
        startSong();
      } else if (snoozeActivated) {
        // while snoozed keep the alarm button blinking so you can cancel the snooze or know you can cancel the snooze
        if (currentSecond % 2 == 0) {
          digitalWrite(ALARM_BUTTON_LED, HIGH);
        } else {
          digitalWrite(ALARM_BUTTON_LED, LOW);
        }
      }
    }
  } else {
    if (snoozeActivated) {
      if (currentSecond % 2 == 0) {
        digitalWrite(ALARM_BUTTON_LED, HIGH);
      } else {
        digitalWrite(ALARM_BUTTON_LED, LOW);
      }
    }// else {
      // only stop alarm 30 seconds after the alarm time otherwise skip
      if (currentSecond > endTimeToAlarm && SongActive) {
        Serial.println("stop alarm");
        stopSong(false);
      }
    //}
  }
}

void loop() {
  bool button_delay = false;
  bool alarm_button_pressed = false;
  bool snooze_button_pressed = false;

  if (digitalRead(USER_BUTTON_PIN)) { alarm_button_pressed = true; }
  if (digitalRead(USER2_BUTTON_PIN)) { snooze_button_pressed = true; }

  if (alarm_button_pressed) {
    Serial.println("pressed B");
    stopSong();
    snoozeActivated = false;
    button_delay = true;
  }

  if (snooze_button_pressed) {
    Serial.println("pressed snooze");
    stopSong();
    if (SongActive) {
      didSnoozeStart  = false;
      snoozeActivated = true;
      snoozedAt       = currentSecond;
    }
    button_delay = true;
  }

  if (DidInitWifi) {
    int offsetSeconds = settings.timezoneOffset();
    timeClient.setTimeOffset(offsetSeconds);
    //Serial.println("offset now update");
    //Serial.println("called update");
    currentSecond = timeClient.getEpochTime(); // set globally

    //Serial.println("try to get time");
    int rawMinute = (int)(currentSecond/60);
    int lastMinute = (int)(prevSecond/60);
    if (rawMinute > lastMinute) {
      int rawHour = rawMinute/60;
      int lastHour = lastMinute/60;
      //snprintf(buffer, OUT_BUFFER_SIZE, "rawTime: %lu, prevSecond: %lu, rawMinute: %d, lastMinute: %d, rawHour: %d, lastHour: %d\n", rawTime, prevSecond, rawMinute, lastMinute, rawHour, lastHour);
      //Serial.print(buffer);

      if (lastHour < rawHour) {
        // keep clock in sync
        Serial.println("it's been over an hour fetch updated time, quote and weather as necessary");
        timeClient.update();
        settings.fetchQuote(timeClient);
        settings.fetchWeather(timeClient);
      }
    }
    prevSecond = currentSecond;
    checkAlarm();
  }

  if (snooze_button_pressed && alarm_button_pressed) {
    // clear the display
    StaticJsonDocument<64> doc;
    doc["clear"] = true;
    size_t bytesWritten = serializeJson(doc, Serial1);
    delay(5000);
  } else if (DidInitWifi) {
    displayClock();
  }

  if (button_delay) {
    delay(500);
  }

  if (ConfigMode && deviceConnected) {
    // send key:value pairs
    // for each key and value we support for writing to our ble device
    const char *key = BLE_SET_CONFIG_KEYS[last_sent_key];
    char txBuffer[4+1+32+1]; // 4 bytes for key name, 1 byte for :, and 32 bytes for the value and 1 byte for a null
    switch (last_sent_key) {
    case 0: // "ssid"
      snprintf(txBuffer, sizeof(txBuffer), "%s:%s", key, settings.ssid);
      break;
    case 1: // "pass"
      snprintf(txBuffer, sizeof(txBuffer), "%s:%s", key, settings.pass);
      break;
    case 2: // "alrh" -> a 24 hour when the alarm should sound
      snprintf(txBuffer, sizeof(txBuffer), "%s:%d", key, settings.hour);
      break;
    case 3: // "alrm" -> the minute that the alarm should sound within an hour
      snprintf(txBuffer, sizeof(txBuffer), "%s:%d", key, settings.minute);
      break;
    case 4: // "dtim" -> device time a read only option so we can see what time the device has stored
      snprintf(txBuffer, sizeof(txBuffer), "%s:%lu", key, currentSecond+settings.timezoneOffset());
      break;
    case 5: // zip
      snprintf(txBuffer, sizeof(txBuffer), "%s:%s", key, settings.zipcode);
      break;
    case 6: // api
      snprintf(txBuffer, sizeof(txBuffer), "%s:%s", key, settings.weatherAPI);
      break;
    default:
      Serial.println("unknown key index");
      last_sent_key = 0;
      // overflow
      return;
    }

    last_sent_key++;
    if (last_sent_key >= BLE_KEY_CONFIG_SIZE) {
      last_sent_key = 0; // loop back around
    }

    // Let's convert the value to a char array:
    //dtostrf(txValue, 1, 2, txString); // float_val, min_width, digits_after_decimal, char_buffer

//    pCharacteristic->setValue(&txValue, 1); // To send the integer value
//    pCharacteristic->setValue("Hello!"); // Sending a test message
    pCharacteristic->setValue(txBuffer);

    pCharacteristic->notify(); // Send the value to the app!
    /*Serial.print("*** Sent Value: ");
    Serial.print(txBuffer);
    Serial.println(" ***");
    */

  }

  delay(10);
}
