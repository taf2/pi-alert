// Basic demo for reading Humidity and Temperature
// display on oled display
#include <time.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include <Adafruit_MS8607.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define VBATPIN A13

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

static char buf[80];
static char buffer[1024];
//static int mode = 0; // Button A
static int degree = 1; // 0 is C 1 is F
static int timezone = -14400;
static const char *ssid = "<%= @config[:ssid] %>"; // Put your SSID here
static const char *password = "<%= @config[:pass] %>"; // Put your PASSWORD here

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
Adafruit_MS8607 ms8607;
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);


void displayln(const char *text) {
  display.println(text);
  display.setCursor(0,0);
  display.display(); // actually display all of the above
}

void setup(void) {

  Serial.begin(115200);
  while (!Serial) delay(10);     // will pause Zero, Leonardo, etc until serial console opens
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3C for 128x32

  display.display();
  delay(1000);
  // Clear the buffer.
  display.clearDisplay();
  display.display();

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


  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);

  // text display tests
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);


  displayln("Adafruit MS8607 test with oled display!");

  // Try to initialize!
  if (!ms8607.begin()) {
    Serial.println("Failed to find MS8607 chip");
    while (1) { delay(10); }
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
  Serial.println("");

  // initialize time
  timeClient.begin();

  // EST
  //timeClient.setTimeOffset(-18000);
  // EDT
  timeClient.setTimeOffset(timezone);
}

void displayTime() {
  timeClient.update();
  time_t rawtime = timeClient.getEpochTime();
  struct tm  ts;
  ts = *localtime(&rawtime);
  strftime(buf, sizeof(buf), "  %I:%M %p\n", &ts);
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.print(buf);
  display.setCursor(0,18);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  strftime(buf, sizeof(buf), "%a %b, %d\n", &ts);
  display.print(buf);
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

  if (degree) {
    // (26.15°C × 9/5) + 32 = 79.07°F
    temperature = (temperature * 1.8) + 32.0;
  }
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextColor(SSD1306_WHITE);
  displayTime();

  snprintf(buffer, 1024,
           "%.0f%s, %.0f %%rH, %.1fV\n",
           temperature, (degree ? "F" : "C"), humidity.relative_humidity, battery);
  display.setTextSize(1);

  display.print(buffer);

  yield();

  display.display();
}


void loop() {
  int button = 0;
  if (!digitalRead(BUTTON_A)) {
    Serial.println("Adjust timezone down by 30 minutes");
    timezone -= 1800;
    timeClient.setTimeOffset(timezone);
    button = 1;
  }

  if (!digitalRead(BUTTON_B)) {
    Serial.println("Adjust timezone up by 30 minutes");
    timezone += 1800;
    timeClient.setTimeOffset(timezone);
    button = 1;
  }

  if (!digitalRead(BUTTON_C)) {
    Serial.println("Modify celsius to fahrenheit");
    degree = (degree ? 0 : 1);
    button = 1;
  }

  displayClock();

  if (button) {
    delay(500);
  }

}
