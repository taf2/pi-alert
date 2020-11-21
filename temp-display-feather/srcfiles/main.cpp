/* Basic demo for reading Humidity and Temperature
  display on oled display

	going to also have an epaper device now to display softer clock face we'll need to only update each minute to throttle 
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

//#include <SPIFFS.h> // cache quote of the day so we only re-fetch 1 time per day

#include <Adafruit_MS8607.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


// so we can write to epaper
#ifdef ENABLE_EPAPER
#undef ENABLE_GxEPD2_GFX // ensure we use adafruit
//#include <GxEPD2_BW.h>  // Include GxEPD2 library for black and white displays
#include <GxEPD2_3C.h>  // Include GxEPD2 library for 3 color displays
#endif


// Include fonts from Adafruit_GFX
//#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeMono18pt7b.h>
//#include <Fonts/FreeMono24pt7b.h>
//#include <Fonts/FreeMonoBold9pt7b.h>
//#include <Fonts/FreeMonoBold12pt7b.h>
//#include <Fonts/FreeMonoBold18pt7b.h>
//#include <Fonts/FreeMonoBold24pt7b.h>
//#include "Roboto_Regular64pt7b.h"
#include "Roboto_Regular78pt7b.h"
//#include "Roboto_Regular92pt7b.h"

#include "eeprom_settings.h"

#define LED_CONFIGURED 12
#define LED_BLUE_CONFIG 13
#define BUZZER 33
#define MP3_PWR 27

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
#define SERVICE_UUID           "e2f52832-2c23-4318-85cb-be11f7421999" //"6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "e5de2fab-bf9b-439d-81d1-24651d8201a7" //"6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "e6f61157-5e3a-4656-a412-de8610a63d76" //"6E400003-B5A3-F393-E0A9-E50E24DCCA9E"


#define VBATPIN A13

// ePaper pins in addition to MOSI (DIN) and SCK (CLK)
// we can't use A3 or A4 since they have to be output capable and are not on the feather esp32
#define CS_PIN          A5 // must be output capable
#define DC_PIN          A1 // must be output capable
#define RST_PIN         A0 // must be output capable and is an INPUT_PULLUP
#define BUSY_PIN        A2 // must be input capable
#define USER_BUTTON_PIN 21 // external user interface button to toggle on / off alarm

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

#ifdef ENABLE_EPAPER
static short displayTime(short needUpdate, time_t &lastSecond, bool normalDisplay=true, GxEPD2_3C<GxEPD2_565c, GxEPD2_565c::HEIGHT> *ePaperDisplay=NULL);
#else
static short displayTime(short needUpdate, time_t &lastSecond, bool normalDisplay=true);
#endif
static void disableBLE();
static bool initWiFi(const char *ssid, const char *password);
static bool initClockAndWifi();

static volatile time_t currentSecond = 0;
static time_t prevSecond = 0;
static time_t lastSecond    = 0;
static float lastTemp    = 0;

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
static Adafruit_MS8607 ms8607;
static Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);

#ifdef ENABLE_EPAPER
static SemaphoreHandle_t settingsLock;
#endif
static EEPROMSettings settings;

#ifdef ENABLE_EPAPER
static TaskHandle_t UpdateEPaperTask;
// moving all ePaperDisplay operations into their own loop on a different core to keep the clock responsive
GxEPD2_3C<GxEPD2_565c, GxEPD2_565c::HEIGHT> *ePaperDisplay = new GxEPD2_3C<GxEPD2_565c, GxEPD2_565c::HEIGHT>(GxEPD2_565c(CS_PIN, DC_PIN, RST_PIN, BUSY_PIN));

static void UpdateEPaper(void * parameter) {
  short needUpdate = 0;
  time_t lastSecond = 0;
	Serial.println("Initialize epaper");

  try {
  // put it on the heap to avoid blowing up the stack

  if (!ePaperDisplay) {
    Serial.println("INvalid ePaperDisplay unable to allocate crash");
    return;
  }

  //uxTaskGetStackHighWaterMark(NULL);
  //yield();

	ePaperDisplay->init(115200);  // Initiate the display
  ePaperDisplay->setRotation(0);  // Set orientation. Goes from 0, 1, 2 or 3
  ePaperDisplay->setCursor(0,0);

  ePaperDisplay->setTextWrap(true);  // By default, long lines of text are set to automatically “wrap” back to the leftmost column.
  ePaperDisplay->setFullWindow();  // Set full window mode, meaning is going to update the entire screen
  ePaperDisplay->setTextColor(GxEPD_BLACK);  // Set color for text
	//Serial.println("Initialized epaper");

  //Serial.println("Start ePaper Loop");

  while(true) {
    
    if (DidInitWifi && !ConfigMode) {
#ifdef ENABLE_EPAPER
      if (xSemaphoreTake(settingsLock, (TickType_t) 10 ) == pdTRUE) {
        needUpdate = displayTime(needUpdate, lastSecond, false, ePaperDisplay);
        xSemaphoreGive(settingsLock);
      } else {
        needUpdate = 0;
      }
#else
      needUpdate = displayTime(needUpdate, lastSecond, false, ePaperDisplay);
#endif
      if (needUpdate == 2) {
        Serial.println("refresh epaper");
        //ePaperDisplay.refresh(80, 220, 400, 96);
        ePaperDisplay->display();
      } else {
        delay(500);
      }
    }
    yield();

   // yield();
  }

  } catch(...) {
    while(true) {
      Serial.println("failed");
      delay(1000);
    }
  }
}
#endif

void displayln(const char *text) {
  display.println(text);
  display.setCursor(0,0);
  display.display(); // actually display all of the above
}

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
        settings.save();
        Serial.print("hour set:");
        Serial.println(settings.hour);
      } else if (key.length() > 0 && key == "alrm" && value.length() > 0) {
        Serial.println( ( key + " = " + value).c_str() );
        settings.minute = atoi(value.c_str());
        StopSongTime = 0; // reset alarm stop
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
    snprintf(buffer, OUT_BUFFER_SIZE, "Failed to connect to %s \\", ssid);
    displayln(buffer);
    return false;
  }
  snprintf(buffer, OUT_BUFFER_SIZE, "Connecting to %s \\", ssid);
  displayln(buffer);
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
    snprintf(buffer, OUT_BUFFER_SIZE, "Connecting to %s %c", ssid, c);
    display.clearDisplay();
    displayln(buffer);
    yield();
    ++count;
    if (!digitalRead(BUTTON_A)) {
      // user pressed A button while trying to connect this should abort all wifi
      return false;
    }
  }
  ip = WiFi.localIP();
  Serial.println(ip);
  snprintf(buffer, OUT_BUFFER_SIZE, "Connected to %s with %s\n", ssid, ip.toString().c_str());
  display.clearDisplay();
  displayln(buffer);
  delay(2000);

  // initialize time
  DidInitWifi = true;
  return true;
}

void startSong() {
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
  SongActive   = false;
  if (report) {
    Serial.println("stop alarm");
    StopSongTime = currentSecond;
  }
}

void initButtons() {
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);
  digitalWrite(BUTTON_C, HIGH);
  digitalWrite(BUTTON_B, HIGH);
  digitalWrite(BUTTON_A, HIGH);

  pinMode(USER_BUTTON_PIN, INPUT);
  digitalWrite(USER_BUTTON_PIN, LOW);
}

void initWeather() {
  // Try to initialize!
  if (!ms8607.begin()) {
    while (1) {Serial.println("Failed to find MS8607 chip"); delay(1000); }
  }
  Serial.println("MS8607 Found!");

  ms8607.setHumidityResolution(MS8607_HUMIDITY_RESOLUTION_OSR_8b);
  Serial.print("Humidity resolution set to ");
  switch (ms8607.getHumidityResolution()){
    case MS8607_HUMIDITY_RESOLUTION_OSR_12b: Serial.println("12-bit"); break;
    case MS8607_HUMIDITY_RESOLUTION_OSR_11b: Serial.println("11-bit"); break;
    case MS8607_HUMIDITY_RESOLUTION_OSR_10b: Serial.println("10-bit"); break;
    case MS8607_HUMIDITY_RESOLUTION_OSR_8b: Serial.println("8-bit"); break;
  }
  // ms8607.setPressureResolution(MS8607_PRESSURE_RESOLUTION_OSR_4096);
  Serial.print("Pressure and Temperature resolution set to ");
  switch (ms8607.getPressureResolution()){
    case MS8607_PRESSURE_RESOLUTION_OSR_256: Serial.println("256"); break;
    case MS8607_PRESSURE_RESOLUTION_OSR_512: Serial.println("512"); break;
    case MS8607_PRESSURE_RESOLUTION_OSR_1024: Serial.println("1024"); break;
    case MS8607_PRESSURE_RESOLUTION_OSR_2048: Serial.println("2048"); break;
    case MS8607_PRESSURE_RESOLUTION_OSR_4096: Serial.println("4096"); break;
    case MS8607_PRESSURE_RESOLUTION_OSR_8192: Serial.println("8192"); break;
  }
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

// NOTE: once ble is disabled without restarting the device i have not figured out how to get enable to work after a disable...
void disableBLE() {
  ConfigMode = false;
  if (pServer) {
    Serial.println("stop ble server");
    pServer->getAdvertising()->stop();
    digitalWrite(LED_BLUE_CONFIG, LOW);
    if (digitalRead(LED_BLUE_CONFIG)) {
      Serial.println("failed to set LED low");
    } else {
      Serial.println("LED should be low now");
    }
  }
}
bool initClockAndWifi() {
  Serial.println("wifi is configuredi start connecting");
  Serial.println(settings.ssid);
  if (!initWiFi(settings.ssid, settings.pass)) {
    return false;
  }
  initWeather(); // local device weather conditions
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

#ifdef ENABLE_EPAPER
  Serial.println("create the SemaphoreHandle_t");
  settingsLock = xSemaphoreCreateMutex();
  if (settingsLock) {
    xTaskCreatePinnedToCore(UpdateEPaper, // Function to implement the task
      "UpdateEPaper", // Name of the task
      4096, //8192,  // Stack size in words
      NULL,  // Task input parameter
      -1,  // Priority of the task
      &UpdateEPaperTask,  // Task handle.
      0); // Core where the task should run
    Serial.println("created the paper task");
  }
#endif
  return true;
}

int buttonPressedCount = 0;
void setup() {

  Serial.begin(115200);
  while (!Serial) delay(10);     // will pause Zero, Leonardo, etc until serial console opens

	EEPROM.begin(EEPROM_SIZE);

  pinMode(LED_CONFIGURED, OUTPUT);
  pinMode(LED_BLUE_CONFIG, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(MP3_PWR, OUTPUT);
  digitalWrite(BUZZER, HIGH); // ensure 
  digitalWrite(MP3_PWR, LOW);
  digitalWrite(LED_BLUE_CONFIG, LOW);


  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3C for 128x32
  // text display tests
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);

  display.display();
  delay(1000);
  // Clear the buffer.
  display.clearDisplay();
  display.display();


//  if (!SPIFFS.begin(true)) {
//    Serial.println("An Error has occurred while mounting SPIFFS");
//  }
//
  initButtons();

  Serial1.begin(9600);

    //enableBLE(); // initialize the library early
/*  if (settings.load() && settings.good()) {
    if (!initWiFi(settings.ssid, settings.pass)) {
      return;
    }
    DidInitWifi = false;
    enableBLE();
  }
  */
    
  if (settings.load() && settings.good()) {
    //enableBLE(); // initialize the library early
    //delay(2000);
    //disableBLE(); // turn it off unless needed?
    //delay(2000);
    Serial.println("config seems good so we're gonna try to configure wifi");
    initClockAndWifi();
  } else {
    Serial.println("settings not good yet turn on bluetooth so we can get user settings");
    display.clearDisplay();
    displayln("Enable BLE");
    enableBLE();
    display.clearDisplay();
    displayln("Connect to Clock via Bluetooh to configure WiFI");
  }

}

// returns 0 no updates, returns 1 only lcd updates, returns 2 both lcd and epaper updates
// normalDisplay is so we can run this in the main loop for an lcd but also in an epaper loop
//int displayTime(short needUpdate, NTPClient &tc, time_t &lastSecond, bool normalDisplay) {
#ifdef ENABLE_EPAPER
short displayTime(short needUpdate, time_t &lastSecond, bool normalDisplay, GxEPD2_3C<GxEPD2_565c, GxEPD2_565c::HEIGHT> *ePaperDisplay) {
#else
short displayTime(short needUpdate, time_t &lastSecond, bool normalDisplay) {
#endif
  char buf[80];
  time_t rawtime = currentSecond;//tc.getEpochTime();
  int rawMinute = (int)(rawtime/60);
  int lastMinute = (int)(lastSecond/60);
  if (needUpdate ||  rawMinute > lastMinute) {
    bool epaper_update = (rawMinute > lastMinute);
    //snprintf(buffer, 1024,
    //         "rawtime: %ld, lastSecond: %ld, rm: %d lm: %d\n", rawtime, lastSecond, rawMinute, lastMinute);
    //Serial.print(buffer);
    struct tm  ts;
    ts = *localtime(&rawtime);
    lastSecond = rawtime;
    strftime(buf, sizeof(buf), "%l:%M %p\n", &ts);

    //  the epaper display is too big for us to also include on this board so we'll write to serial here instead
    Serial.println("write to Serial1");
    StaticJsonDocument<1024> doc;
    doc["time"] = currentSecond;
    doc["temp"] = settings.temp;
    doc["quote"] = settings.quote;
    doc["author"] = settings.author;
    serializeJson(doc, Serial1);

#ifdef ENABLE_EPAPER
    if (normalDisplay && !ePaperDisplay) {
#else
    if (normalDisplay) {
#endif
      display.setTextSize(2);
      display.setTextColor(SSD1306_WHITE);
      display.print(buf);
    }
#ifdef ENABLE_EPAPER
    if (epaper_update && ePaperDisplay) {
      //ePaperDisplay.getTextBounds(buf, 0, 32, &x1, &y1, &w, &h);
      //ePaperDisplay.fillRect(0, 32, w, h, GxEPD_WHITE);
      ePaperDisplay->fillScreen(GxEPD_WHITE);  // Clear previous graphics to start over to print new things.
      //ePaperDisplay.setPartialWindow(0, 0, ePaperDisplay.width(), ePaperDisplay.height());
      ePaperDisplay->setFont(&Roboto_Regular78pt7b);  // Set font
      ePaperDisplay->setCursor(80, 220);  // Set the position to start printing text (x,y)
      strftime(buf, sizeof(buf), "%l:%M", &ts);
      ePaperDisplay->println(buf);  // Print some text
    }
#endif
    strftime(buf, sizeof(buf), "%a %b, %d\n", &ts);
#ifdef ENABLE_EPAPER
    if (normalDisplay && !ePaperDisplay) {
#else
    if (normalDisplay) {
#endif
      display.setCursor(0,18);
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.print(buf);
    }
#ifdef ENABLE_EPAPER
    if (epaper_update && ePaperDisplay) {
#else
    if (epaper_update) {
#endif
#ifdef ENABLE_EPAPER
      /*int16_t  x1, y1;
      uint16_t w, h;
      ePaperDisplay.setFont(&FreeMono18pt7b);  // Set font
      ePaperDisplay.getTextBounds(buf, 0, 32, &x1, &y1, &w, &h);
      ePaperDisplay.fillRect(0, 32, w, h, GxEPD_WHITE);
      */
      ePaperDisplay->setCursor(25, 50);  // Set the position to start printing text (x,y)
      ePaperDisplay->setFont(&FreeMono18pt7b);  // Set font
      ePaperDisplay->println(buf);  // print the date
      //yield();
      // print a quote if we have one
      if (strlen(settings.quote) > 0) {
        ePaperDisplay->setCursor(0, 290);  // Set the position to start printing text (x,y)
        ePaperDisplay->setFont(&FreeMono12pt7b);  // Set font
        ePaperDisplay->println(settings.quote);  // print the quote
        ePaperDisplay->print("by: ");  // print the author
        ePaperDisplay->println(settings.author);  // print the author
      }
      // display temperature
      ePaperDisplay->setCursor(480, 50);
      ePaperDisplay->setFont(&FreeMono18pt7b);
      ePaperDisplay->print((int)roundf(settings.temp));
      ePaperDisplay->println("F");
#else
#endif
      return 2;
    }
    return 1;
#ifdef ENABLE_EPAPER
  } else if (normalDisplay && !ePaperDisplay) {
#else
  } else if (normalDisplay) {
#endif
    struct tm  ts;
    ts = *localtime(&rawtime);
    lastSecond = rawtime;
  }
  return 0;
}

float readBattery() {
  float measuredvbat = analogRead(VBATPIN);
  //Serial.print("battery volt reading: "); Serial.println(measuredvbat);
  // measuredvbat = (measuredvbat / 4095)*2*3.3*1.1;
  measuredvbat /= 4095; // convert to voltage
  measuredvbat *= 2;    // we get half voltage, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat *= 1.1;  // not sure... but this gives better on esp32 then perhaps other processor?
  return measuredvbat;
}


void displayClock() {
  sensors_event_t temp, temp_pressure, temp_humidity;
  ms8607.getEvent(&temp_pressure, &temp, &temp_humidity);
  float temperature = temp.temperature;
  float humidity = temp_humidity.relative_humidity;
  float battery = readBattery();
  short needUpdate = roundf(lastTemp) != roundf(temperature);

  lastTemp = temperature; // so we can track changes in the battey signal

  if (settings.fahrenheit) {
    // (26.15°C × 9/5) + 32 = 79.07°F
    temperature = (temperature * 1.8) + 32.0;
  }
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextColor(SSD1306_WHITE);
#ifdef ENABLE_EPAPER
  if (xSemaphoreTake(settingsLock, (TickType_t) 10 ) == pdTRUE) {
    needUpdate = displayTime(needUpdate, lastSecond);
  } else {
    needUpdate  = 0;
  }
#else
  needUpdate = displayTime(needUpdate, lastSecond);
#endif
  if (needUpdate) {
    snprintf(buffer, OUT_BUFFER_SIZE,
             "%.0f%s, %.0f %%rH, %.1fV\n",
             temperature, (settings.fahrenheit ? "F" : "C"), humidity, battery);
    display.setTextSize(1);

    display.print(buffer);

    display.display();
    if (needUpdate == 2) {
      // this means at least 1 minute has past since the last update so we'll do things here like also checking alarms and quote updates
      // as well as updating our epaper if it's enabled
      Serial.println("check for updated quote");
      settings.loadQuote(timeClient);
#ifdef ENABLE_EPAPER
      //doDisplay = true;
      //ePaperDisplay.display();
#endif
    }
  }
}

void checkAlarm() {
  //const int    tzOffset = -5 * 60 * 60;
  currentSecond = timeClient.getEpochTime(); // set globally
  const int offsetSeconds = settings.timezoneOffset();
  const time_t epoch = currentSecond;
  const time_t offsetTime = epoch;// + tzOffset;
  const int    currentDay = offsetTime / 24 / 60 / 60; // convert seconds to the day
  const short hour = settings.hour;
  const short minute = settings.minute;

  const int startOfDayInSeconds = (currentDay * 24 * 60 * 60); // this gets us the starting second of the current day
  const int startTimeToAlarm = startOfDayInSeconds + (hour*60*60) + (minute*60);
  const int endTimeToAlarm = startOfDayInSeconds + (hour*60*60) + ((minute+4)*60); // 4 minutes of padding

  //snprintf(buffer, OUT_BUFFER_SIZE, "epoch: %ld (%ld), %d, startTimeToAlarm: %d, endTimeToAlarm: %d\n", epoch, offsetTime, offsetSeconds, startTimeToAlarm, endTimeToAlarm);
//  Serial.println(buffer);

  if (offsetTime > startTimeToAlarm && offsetTime < endTimeToAlarm) {
    if (!SongActive) {
      Serial.println("sound the alarm");
      if (StopSongTime < startTimeToAlarm) {
        startSong();
      } else {
        Serial.println("you stopped it");
        Serial.println(StopSongTime);
        Serial.println(startTimeToAlarm);
      }
    }
  } else {
    stopSong(false);
  }
}

int last_button_pressed = 0;

void loop() {
  short button_delay = 0;
  short button_a_pressed = 0;
  short button_b_pressed = 0;
  short button_c_pressed = 0;

  if (digitalRead(USER_BUTTON_PIN)) {
    button_b_pressed = 1;
    Serial.println("user button pressed");
    last_button_pressed++;
  } else {
    last_button_pressed = 0;
  }

  if (last_button_pressed > 10) { // 500 * 10 == 5 seconds
    Serial.println("user pressed a long 10 seconds");
    button_a_pressed = 1; // as if configure toggled on
  }
  if (last_button_pressed > 40) { // 500 * 40 == 20 seconds (force fetch new quote)
    Serial.println("force fetch");
    button_a_pressed = 0; // toggle off
    button_c_pressed = 1; // fetch 
  }

  if (last_button_pressed > 80) { // 500 * 80 == 40 seconds (factory reset)
    Serial.println("user factory reset");
    button_a_pressed = 1;
    button_b_pressed = 1;
    button_c_pressed = 0;
  }


  if (!digitalRead(BUTTON_A)) {
    button_a_pressed  = 1;
  }

  if (!digitalRead(BUTTON_B)) {
    button_b_pressed = 1;
  }

  if (!digitalRead(BUTTON_C)) {
    button_c_pressed = 1;
  }

  if (button_a_pressed) {
    Serial.println("Button A pressed enable ble");
    button_delay = 1;
    if (ConfigMode) {
      ConfigMode  = false;
    } else {
      ConfigMode  = true;
    }

    if (ConfigMode) {
      digitalWrite(LED_BLUE_CONFIG, HIGH);
      digitalWrite(LED_CONFIGURED, LOW);
      enableBLE();
    } else {
      digitalWrite(LED_BLUE_CONFIG, LOW);
      disableBLE();
    }

  }
  if (button_b_pressed) {
    Serial.println("pressed B");
    //if (SongActive) {
    stopSong();
//    } else {
//      startSong();
//    }
    button_delay = 1;
  }

  if (button_c_pressed) {
    Serial.println("pressed C");
    settings.fetchQuote(timeClient);
    settings.fetchWeather(timeClient);
    button_delay = 1;
  }

  if (DidInitWifi) {
    int offsetSeconds = settings.timezoneOffset();
//    Serial.print("set timezone offset:");
//    Serial.println(offsetSeconds);
    timeClient.setTimeOffset(offsetSeconds);
    //Serial.println("offset now update");
    timeClient.update();
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
        Serial.println("it's been over an hour fetching updated data");
        settings.fetchQuote(timeClient);
        settings.fetchWeather(timeClient);
      }
    }
    prevSecond = currentSecond;
    checkAlarm();
  }

  if (button_a_pressed && button_b_pressed) {
    displayln("Factory Reset...");
    display.clearDisplay();
    memset(settings.ssid, 0, 32);
    memset(settings.pass, 0, 32);
    memset(settings.quote, 0, 256);
    memset(settings.author, 0, 32);
    memset(settings.zipcode, 0, 8);
    memset(settings.weatherAPI, 0, 34);
    settings.temp = 0;
    settings.feels_like = 0;
    settings.hour = 0;
    settings.minute = 0;
    settings.save();

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
      snprintf(txBuffer, sizeof(txBuffer), "%s:%lu", key, currentSecond);
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
