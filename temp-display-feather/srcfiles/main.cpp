/* Basic demo for reading Humidity and Temperature
  display on oled display

	going to also have an epaper device now to display softer clock face we'll need to only update each minute to throttle 
	writes to the epaper since updates are expensive to the display

	the oled display will continue to be attached and we'll have a night option since clocks suck at night they're too bright
	we'll have the primary display be an epaper 5.65 " display with epaper, we'll dialy update the quote to inspire everyone
	*/
#define ENABLE_EPAPER 1
#include <time.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <EEPROM.h> /* to save the selected time offset button A adds 30 minutes, button B removes 30 minutes */
#define EEPROM_SIZE 3

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
//static int mode = 0; // Button A
static int degree = 1; // 0 is C 1 is F
static int timezoneIndex = 0;
// we need this table to allow buttons to switch between timezones also we can in this way perist the timezoneIndex in EEPROM
static const int ZONES[][2] = {{14,0}, // UTC +14	Samoa and Christmas Island/Kiribati	LINT	Kiritimati
											   {13,45}, // UTC +13:45	Chatham Islands/New Zealand	CHADT	Chatham Islands
												 {13,0}, // UTC +13	New Zealand with exceptions and 4 more	NZDT	Auckland
												 {12,0}, // UTC +12	Fiji, small region of Russia and 7 more	ANAT	Anadyr
												 {11,0}, // UTC +11	much of Australia and 7 more	AEDT	Melbourne
												 {10,45}, // UTC +10:30	small region of Australia	ACDT	Adelaide
												 {10,0}, // UTC +10	Queensland/Australia and 6 more	AEST	Brisbane
												 {9,30}, // UTC +9:30	Northern Territory/Australia	ACST	Darwin
										     {9,0}, // UTC +9	Japan, South Korea and 5 more	JST	Tokyo
												 {8,45}, // UTC +8:45	Western Australia/Australia	ACWST	Eucla
												 {8, 0}, // UTC +8	China, Philippines and 10 more	CST	Beijing
												 {7, 0}, // UTC +7	much of Indonesia, Thailand and 7 more	WIB	Jakarta
												 {6, 30}, // UTC +6:30	Myanmar and Cocos Islands	MMT	Yangon
												 {6,0}, // UTC +6	Bangladesh and 6 more	BST	Dhaka
												 {5,45}, // UTC +5:45	Nepal	NPT	Kathmandu
												 {5,30}, // UTC +5:30	India and Sri Lanka	IST	New Delhi
												 {5,0}, // UTC +5	Pakistan and 8 more	UZT	Tashkent
												 {4,30}, // UTC +4:30	Afghanistan	AFT	Kabul
												 {4,0}, // UTC +4	Azerbaijan and 8 more	GST	Dubai
												 {3,30}, // UTC +3:30	Iran	IRST	Tehran
												 {3,0}, // UTC +3	Greece and 36 more	MSK	Moscow
												 {2,0}, // UTC +2	Germany and 48 more	CEST	Brussels
												 {1,0}, // UTC +1	United Kingdom and 22 more	BST	London
												 {0,0}, // UTC +0	Iceland and 17 more	GMT	Accra
												 {-1,0}, // UTC -1	Cabo Verde	CVT	Praia
												 {-2,0}, //UTC -2	most of Greenland and 3 more	WGST	Nuuk
												 {-2,-30}, // UTC -2:30	Newfoundland and Labrador/Canada	NDT	St. John's
												 {-3,-0}, // UTC -3	most of Brazil, Argentina and 10 more	ART	Buenos Aires
												 {-4,-0}, // UTC -4	regions of USA and 31 more	EDT	New York
												 {-5,-0}, // UTC -5	regions of USA and 10 more	CDT	Mexico City
												 {-6,-0}, // UTC -6	small region of USA and 9 more	CST	Guatemala City
												 {-7,-0}, // UTC -7	regions of USA and 2 more	PDT	Los Angeles
												 {-8,-0}, // UTC -8	Alaska/USA and 2 more	AKDT	Anchorage
												 {-9,-0}, // UTC -9	Alaska/USA and regions of French Polynesia	HDT	Adak
												 {-9,-30}, // UTC -9:30	Marquesas Islands/French Polynesia	MART	Taiohae
												 {-10,-0}, // UTC -10	Hawaii/USA and 2 more	HST	Honolulu
												 {-11,-0}, // UTC -11	American Samoa and 2 more	NUT	Alofi
												 {-12,-0}  // UTC -12	much of US Minor Outlying Islands	AoE	Baker Island
};
//
// NOTE: maybe we can use curl "http://worldtimeapi.org/api/ip/:ipv4:.txt" to determine dst vs not?
// or at least on first boot as away to guess timezone
//
static const char *ZONE_NAMES[] = {"UTC +14	Samoa and Christmas Island/Kiribati	LINT	Kiritimati",
                                   "UTC +13:45	Chatham Islands/New Zealand	CHADT	Chatham Islands",
                                   "UTC +13	New Zealand with exceptions and 4 more	NZDT	Auckland",
                                   "UTC +12	Fiji, small region of Russia and 7 more	ANAT	Anadyr",
                                   "UTC +11	much of Australia and 7 more	AEDT	Melbourne",
                                   "UTC +10:30	small region of Australia	ACDT	Adelaide",
                                   "UTC +10	Queensland/Australia and 6 more	AEST	Brisbane",
                                   "UTC +9:30	Northern Territory/Australia	ACST	Darwin",
                                   "UTC +9	Japan, South Korea and 5 more	JST	Tokyo",
                                   "UTC +8:45	Western Australia/Australia	ACWST	Eucla",
                                   "UTC +8	China, Philippines and 10 more	CST	Beijing",
                                   "UTC +7	much of Indonesia, Thailand and 7 more	WIB	Jakarta",
                                   "UTC +6:30	Myanmar and Cocos Islands	MMT	Yangon",
                                   "UTC +6	Bangladesh and 6 more	BST	Dhaka",
                                   "UTC +5:45	Nepal	NPT	Kathmandu",
                                   "UTC +5:30	India and Sri Lanka	IST	New Delhi",
                                   "UTC +5	Pakistan and 8 more	UZT	Tashkent",
                                   "UTC +4:30	Afghanistan	AFT	Kabul",
                                   "UTC +4	Azerbaijan and 8 more	GST	Dubai",
                                   "UTC +3:30	Iran	IRST	Tehran",
                                   "UTC +3	Greece and 36 more	MSK	Moscow",
                                   "UTC +2	Germany and 48 more	CEST	Brussels",
                                   "UTC +1	United Kingdom and 22 more	BST	London",
                                   "UTC +0	Iceland and 17 more	GMT	Accra",
                                   "UTC -1	Cabo Verde	CVT	Praia",
                                   "UTC -2	most of Greenland and 3 more	WGST	Nuuk",
                                   "UTC -2:30	Newfoundland and Labrador/Canada	NDT	St. John's",
                                   "UTC -3	most of Brazil, Argentina and 10 more	ART	Buenos Aires",
                                   "UTC -4	regions of USA and 31 more	EDT	New York",
                                   "UTC -5	regions of USA and 10 more	CDT	Mexico City",
                                   "UTC -6	small region of USA and 9 more	CST	Guatemala City",
                                   "UTC -7	regions of USA and 2 more	PDT	Los Angeles",
                                   "UTC -8	Alaska/USA and 2 more	AKDT	Anchorage",
                                   "UTC -9	Alaska/USA and regions of French Polynesia	HDT	Adak",
                                   "UTC -9:30	Marquesas Islands/French Polynesia	MART	Taiohae",
                                   "UTC -10	Hawaii/USA and 2 more	HST	Honolulu",
                                   "UTC -11	American Samoa and 2 more	NUT	Alofi",
                                   "UTC -12	much of US Minor Outlying Islands	AoE	Baker Island"
};
int timezoneOffset() {
  int hours = ZONES[timezoneIndex][0];
  int minutes = ZONES[timezoneIndex][1];
  int totalMinutes = (hours * 60 + minutes);
  int totalSeconds = totalMinutes * 60;
  Serial.print("hours, minutes, totalMinutes, totalSeconds, timezoneIndex:");
  Serial.println(hours);
  Serial.println(minutes);
  Serial.println(totalMinutes);
  Serial.println(totalSeconds);
  Serial.println(timezoneIndex);
	return totalSeconds;
}

static int hasSetting = 0;
static const char *ssid = "<%= @config[:ssid] %>"; // Put your SSID here
static const char *password = "<%= @config[:pass] %>"; // Put your PASSWORD here

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
Adafruit_MS8607 ms8607;
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);
#ifdef ENABLE_EPAPER
GxEPD2_3C<GxEPD2_565c, GxEPD2_565c::HEIGHT> ePaperDisplay(GxEPD2_565c(CS_PIN, DC_PIN, RST_PIN, BUSY_PIN));
#endif

void displayln(const char *text) {
  display.println(text);
  display.setCursor(0,0);
  display.display(); // actually display all of the above
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
    Serial.print(".");
    char c = '/';
    if (count % 2 == 0) {
      c = '/';
    } else {
      c = '\\';
    }
    snprintf(buffer, 1024, "Connecting to %s %c", ssid, c);
    display.clearDisplay();
    displayln(buffer);
  }
  ip = WiFi.localIP();
  Serial.println(ip);
  snprintf(buffer, 1024, "Connected to %s with %s\n", ssid, ip.toString().c_str());
  display.clearDisplay();
  displayln(buffer);
  delay(2000);

  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);



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
  Serial.println("loading time");

  // initialize time
  timeClient.begin();

	hasSetting   = EEPROM.read(2);
	if (hasSetting) {
		Serial.println("hasSetting");
		timezoneIndex = EEPROM.read(0);
		Serial.print("saved timezone:");
		Serial.println(timezoneIndex);
		degree       = EEPROM.read(1);
		Serial.print("saved degree:");
		Serial.println(degree);
    Serial.print("size of ZONES");
    Serial.println((sizeof(ZONES) / sizeof(int)));
		if (timezoneIndex >= (sizeof(ZONES) / sizeof(int))) { // out of bounds
			timezoneIndex = 28; // EDT
		}
	} else {
		timezoneIndex = 28; // EDT
	}
	int offsetSeconds = timezoneOffset();
	Serial.print("set timezone offset:");
	Serial.println(offsetSeconds);
	timeClient.setTimeOffset(offsetSeconds);
 
#ifdef ENABLE_EPAPER
  ePaperDisplay.setTextWrap(false);  // By default, long lines of text are set to automatically “wrap” back to the leftmost column.
  ePaperDisplay.setFullWindow();  // Set full window mode, meaning is going to update the entire screen

  ePaperDisplay.fillScreen(GxEPD_WHITE);  // Clear previous graphics to start over to print new things.
  Serial.print("blank out the screen\r\n ");
  ePaperDisplay.display();
  Serial.print("display finished\r\n ");
  delay(2000);
  Serial.print("done init\r\n ");
#endif
  
  // Print text - "Hello World!":
//  ePaperDisplay.setTextColor(GxEPD_BLACK);  // Set color for text
//  ePaperDisplay.setFont(&FreeMono12pt7b);  // Set font
  //ePaperDisplay.setTextSize(12);
//  ePaperDisplay.setCursor(0, 12);  // Set the position to start printing text (x,y)
//  ePaperDisplay.println("Booting up...");  // Print some text
  Serial.print("Booting up...\r\n ");

  // Draw triangle:
/*  ePaperDisplay.drawTriangle(0,85,   20,60,   40,85,   GxEPD_BLACK);  // Draw triangle. X, Y coordinates for three corner points defining the triangle, followed by a color
  
  // Draw filled triangle:
  ePaperDisplay.fillTriangle(110,85,   130,60,   150,85,   GxEPD_BLACK);  // Draw filled triangle. X, Y coordinates for three corner points defining the triangle, followed by a color
  Serial.print("triangle\r\n ");
  
  // Draw line:
  ePaperDisplay.drawLine(74,20,   74,80,   GxEPD_BLACK);  // Draw line (x0,y0,x1,y1,color)
  Serial.print("line\r\n ");
  
  // Draw rounded rectangle and fill:
  ePaperDisplay.fillRoundRect(48,60,   20,25,   5, GxEPD_BLACK);  // Draw filled rounded rectangle (x,y,width,height,color)
                                                 // It draws from the location to down-right
  Serial.print("rect\r\n ");
  
  // Draw circle:
  ePaperDisplay.drawCircle(95,70,   15, GxEPD_BLACK);  //  Draw circle (x,y,radius,color). X and Y are the coordinates for the center point
  
  // Draw a filled circle:
  ePaperDisplay.fillCircle(100,75,   7, GxEPD_BLACK);  // Draw filled circle (x,y,radius,color). X and Y are the coordinates for the center point
  Serial.print("circle\r\n ");
  
  // Draw rectangle:
  ePaperDisplay.drawRect(8,25,   49,27, GxEPD_BLACK);  // Draw rectangle (x,y,width,height,color)
  Serial.print("rect\r\n ");

  delay(1000);
	*/
  Serial.print("calling display\r\n ");
 // ePaperDisplay.display();
//  Serial.print("display\r\n ");
  //delay(1000);
}

int displayTime(short needUpdate) {
  
  time_t rawtime = timeClient.getEpochTime();
  int rawMinute = (int)(rawtime/60);
  int lastMinute = (int)(lastSecond/60);
  if (needUpdate ||  rawMinute > lastMinute) {
    snprintf(buffer, 1024,
             "rawtime: %ld, lastSecond: %ld, rm: %d lm: %d\n", rawtime, lastSecond, rawMinute, lastMinute);
    Serial.print(buffer);
    timeClient.update(); // only update client every 60 seconds avoids extra network ?
    struct tm  ts;
    ts = *localtime(&rawtime);
    lastSecond = rawtime;
    strftime(buf, sizeof(buf), "  %I:%M %p\n", &ts);
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.print(buf);
    display.setCursor(0,18);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    strftime(buf, sizeof(buf), "%a %b, %d\n", &ts);
    display.print(buf);
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

  if (degree) {
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
             temperature, (degree ? "F" : "C"), humidity.relative_humidity, battery);
    display.setTextSize(1);

    display.print(buffer);

    yield();

    display.display();
  }
}

void adjustTimezone() {
  // 24 times zones
  // smallest timezone of 24 is -12 e.g. 43200
  int offsetSeconds = timezoneOffset();
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextColor(SSD1306_WHITE);
  display.print(ZONE_NAMES[timezoneIndex]);

  yield();

  display.display();

  Serial.print("set timezone offset:");
  Serial.println(offsetSeconds);
  timeClient.setTimeOffset(offsetSeconds);
  EEPROM.write(0, timezoneIndex);
  EEPROM.write(2, 1);
  EEPROM.commit();

  yield();
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
		--timezoneIndex;
		if (timezoneIndex < 0) {
			timezoneIndex = (sizeof(ZONES)/sizeof(int)) - 1;
		}
    adjustTimezone();
    button_delay = 1;
  }

  if (button_b_pressed) {
    Serial.println("Select next timezone");
		++timezoneIndex;
		if (timezoneIndex >= (sizeof(ZONES)/sizeof(int))) {
			timezoneIndex = 0;
		}
    adjustTimezone();
    button_delay = 1;
  }


  if (button_c_pressed) {
    Serial.println("Modify celsius to fahrenheit");
    degree = (degree ? 0 : 1);
		EEPROM.write(1, degree);
		EEPROM.write(2, 1);
		EEPROM.commit();
    button_delay = 1;
  }

  if (button_a_pressed && button_b_pressed) {
    display.clearDisplay();
    displayln("Factory Reset...");
    timezoneIndex = 28; // EDT
    degree = 1;
		EEPROM.write(0, timezoneIndex);
		EEPROM.write(1, degree);
		EEPROM.write(2, 0);
    delay(5000);
  } else {
    displayClock();
  }

  if (button_delay) {
    delay(500);
  }

}
