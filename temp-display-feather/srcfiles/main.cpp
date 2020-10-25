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
//#include <SPIFFS.h> // cache quote of the day so we only re-fetch 1 time per day

#include <Adafruit_MS8607.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// so we can write to epaper
#ifdef ENABLE_EPAPER
#undef ENABLE_GxEPD2_GFX // ensure we use adafruit
#include <GxEPD2_BW.h>  // Include GxEPD2 library for black and white displays
#include <GxEPD2_3C.h>  // Include GxEPD2 library for 3 color displays
#endif


// Include fonts from Adafruit_GFX
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeMono18pt7b.h>
#include <Fonts/FreeMono24pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>
//#include "Roboto_Regular64pt7b.h"
#include "Roboto_Regular78pt7b.h"
//#include "Roboto_Regular92pt7b.h"

#include "eeprom_settings.h"

#define VBATPIN A13

// ePaper pins in addition to MOSI (DIN) and SCK (CLK)
// we can't use A3 or A4 since they have to be output capable and are not on the feather esp32
#define CS_PIN          A5 // must be output capable
#define DC_PIN          A1 // must be output capable
#define RST_PIN         A0 // must be output capable and is an INPUT_PULLUP
#define BUSY_PIN        A2 // must be input capable

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

static time_t lastSecond    = 0;
static float lastTemp    = 0;

static char buf[80];
static char buffer[1024];

static const char *ssid = "<%= @config[:ssid] %>"; // Put your SSID here
static const char *password = "<%= @config[:pass] %>"; // Put your PASSWORD here

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
Adafruit_MS8607 ms8607;
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);
#ifdef ENABLE_EPAPER
GxEPD2_3C<GxEPD2_565c, GxEPD2_565c::HEIGHT> ePaperDisplay(GxEPD2_565c(CS_PIN, DC_PIN, RST_PIN, BUSY_PIN));
#endif

EEPROMSettings settings;

void displayln(const char *text) {
  display.println(text);
  display.setCursor(0,0);
  display.display(); // actually display all of the above
}

void initWiFi() {
  // text display tests
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  IPAddress ip;

  WiFi.begin(ssid, password);
  snprintf(buffer, 1024, "Connecting to %s \\", ssid);
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
    snprintf(buffer, 1024, "Connecting to %s %c", ssid, c);
    display.clearDisplay();
    displayln(buffer);
    yield();
    ++count;
  }
  ip = WiFi.localIP();
  Serial.println(ip);
  snprintf(buffer, 1024, "Connected to %s with %s\n", ssid, ip.toString().c_str());
  display.clearDisplay();
  displayln(buffer);
  delay(2000);

}

void initButtons() {
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);
}

void initWeather() {
  // Try to initialize!
  if (!ms8607.begin()) {
    while (1) {Serial.println("Failed to find MS8607 chip"); delay(10); }
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

void setup(void) {

  Serial.begin(115200);
  while (!Serial) delay(10);     // will pause Zero, Leonardo, etc until serial console opens

	EEPROM.begin(EEPROM_SIZE);


	// now init the ePaper
#ifdef ENABLE_EPAPER
	ePaperDisplay.init(115200);  // Initiate the display
  ePaperDisplay.setRotation(0);  // Set orientation. Goes from 0, 1, 2 or 3
  ePaperDisplay.setCursor(0,0);
	Serial.println("Initialized epaper");
#endif

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3C for 128x32

  display.display();
  delay(1000);
  // Clear the buffer.
  display.clearDisplay();
  display.display();


//  if (!SPIFFS.begin(true)) {
//    Serial.println("An Error has occurred while mounting SPIFFS");
//  }
//
  initWiFi();

  initButtons();
  initWeather();

  Serial.println("loading time");

  // initialize time
  timeClient.begin();


  settings.load();
	int offsetSeconds = settings.timezoneOffset();
	Serial.print("set timezone offset:");
	Serial.println(offsetSeconds);
	timeClient.setTimeOffset(offsetSeconds);
 
#ifdef ENABLE_EPAPER
  ePaperDisplay.setTextWrap(true);  // By default, long lines of text are set to automatically “wrap” back to the leftmost column.
  ePaperDisplay.setFullWindow();  // Set full window mode, meaning is going to update the entire screen
  ePaperDisplay.setTextColor(GxEPD_BLACK);  // Set color for text

/*  ePaperDisplay.fillScreen(GxEPD_WHITE);  // Clear previous graphics to start over to print new things.
  
  // Print text - "Hello World!":
  ePaperDisplay.setTextColor(GxEPD_BLACK);  // Set color for text
  ePaperDisplay.setFont(&FreeMono18pt7b);  // Set font
  Serial.print("calling display\r\n ");
  ePaperDisplay.display();
  Serial.print("display\r\n ");
  delay(2000);
  */
#endif

}

// returns 0 no updates, returns 1 only lcd updates, returns 2 both lcd and epaper updates
int displayTime(short needUpdate) {
  
  time_t rawtime = timeClient.getEpochTime();
  int rawMinute = (int)(rawtime/60);
  int lastMinute = (int)(lastSecond/60);
  if (needUpdate ||  rawMinute > lastMinute) {
    bool epaper_update = (rawMinute > lastMinute);
    snprintf(buffer, 1024,
             "rawtime: %ld, lastSecond: %ld, rm: %d lm: %d\n", rawtime, lastSecond, rawMinute, lastMinute);
    Serial.print(buffer);
    timeClient.update(); // only update client every 60 seconds avoids extra network ?
    struct tm  ts;
    ts = *localtime(&rawtime);
    lastSecond = rawtime;
    strftime(buf, sizeof(buf), "%l:%M %p\n", &ts);
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.print(buf);
    if (epaper_update) {
#ifdef ENABLE_EPAPER
      yield();
      //int16_t  x1, y1;
      //uint16_t w, h;

      //ePaperDisplay.getTextBounds(buf, 0, 32, &x1, &y1, &w, &h);
      //ePaperDisplay.fillRect(0, 32, w, h, GxEPD_WHITE);
      ePaperDisplay.fillScreen(GxEPD_WHITE);  // Clear previous graphics to start over to print new things.
      // Print text - "Hello World!":
      //ePaperDisplay.setTextSize(12);
      ePaperDisplay.setFont(&Roboto_Regular78pt7b);  // Set font
      ePaperDisplay.setCursor(0, 180);  // Set the position to start printing text (x,y)
      strftime(buf, sizeof(buf), "%l:%M", &ts);
      ePaperDisplay.println(buf);  // Print some text
#endif
    }
    display.setCursor(0,18);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    strftime(buf, sizeof(buf), "%a %b, %d\n", &ts);
    display.print(buf);
    if (epaper_update) {
#ifdef ENABLE_EPAPER
      /*int16_t  x1, y1;
      uint16_t w, h;
      ePaperDisplay.setFont(&FreeMono18pt7b);  // Set font
      ePaperDisplay.getTextBounds(buf, 0, 32, &x1, &y1, &w, &h);
      ePaperDisplay.fillRect(0, 32, w, h, GxEPD_WHITE);
      */
      ePaperDisplay.setCursor(65, 260);  // Set the position to start printing text (x,y)
      ePaperDisplay.setFont(&FreeMono24pt7b);  // Set font
      ePaperDisplay.println(buf);  // print the date
      yield();
#endif
      return 2;
    }
    return 1;
  } else {
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
  sensors_event_t temp, pressure, humidity;
  ms8607.getEvent(&pressure, &temp, &humidity);
  float temperature = temp.temperature;
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
  needUpdate = displayTime(needUpdate);
  if (needUpdate) {
    snprintf(buffer, 1024,
             "%.0f%s, %.0f %%rH, %.1fV\n",
             temperature, (settings.fahrenheit ? "F" : "C"), humidity.relative_humidity, battery);
    display.setTextSize(1);

    display.print(buffer);

    yield();

    display.display();
    if (needUpdate == 2) {
      // this means at least 1 minute has past since the last update so we'll do things here like also checking alarms and quote updates
      // as well as updating our epaper if it's enabled
      settings.loadQuote(timeClient);
#ifdef ENABLE_EPAPER
      yield();
      ePaperDisplay.display();
      delay(2000);
#endif
    }
  }
}

void loop() {
  short button_delay = 0;
  short button_a_pressed = 0;
  short button_b_pressed = 0;
  short button_c_pressed = 0;

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
    Serial.println("Select prev timezone");
    settings.prevZone(display, timeClient);
    button_delay = 1;
  }

  if (button_b_pressed) {
    Serial.println("Select next timezone");
    settings.nextZone(display, timeClient);
    button_delay = 1;
  }

  if (button_c_pressed) {
    if (settings.fahrenheit) {
      Serial.println("Modify to fahrenheit celsius");
      settings.fahrenheit = false;
    } else {
      Serial.println("Modify celsius to fahrenheit");
      settings.fahrenheit = true;
    }
    settings.save();
    button_delay = 1;
  }

  if (button_a_pressed && button_b_pressed) {
    display.clearDisplay();
    displayln("Factory Reset...");
    settings.timezoneIndex = 28; // EDT
    settings.fahrenheit = true;
    settings.save();

    delay(5000);
  } else {
    displayClock();
  }

  if (button_delay) {
    delay(500);
  }

}
