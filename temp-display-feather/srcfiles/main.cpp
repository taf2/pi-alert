// Basic demo for reading Humidity and Temperature
// display on oled display
#include <SPI.h>
#include <Wire.h>
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

Adafruit_MS8607 ms8607;
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);
static char buffer[1024];
static int mode = 0; // Button A
static int degree = 0; // 0 is C 1 is F


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
}

// button A
void displayWeather() {
  sensors_event_t temp, pressure, humidity;
  ms8607.getEvent(&pressure, &temp, &humidity);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  float temperature = temp.temperature;

  if (degree) {
    // (26.15°C × 9/5) + 32 = 79.07°F
    temperature = temperature * (9/5) + 32;
  }

  snprintf(buffer, 1024,
           "Temperature: %.1f %s \nPressure: %.1f hPa\nHumidity: %.1f %%rH\n",
           temperature, (degree ? "F" : "C"), pressure.pressure, humidity.relative_humidity);
  display.clearDisplay();

  display.print(buffer);

  delay(10);
  yield();
  display.display();
}

// button B
void displayBattery() {

  float measuredvbat = analogRead(VBATPIN);
  Serial.print("battery volt reading: "); Serial.println(measuredvbat);
  measuredvbat /= 2;    // divid 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage
  // / 2 *  3.3 / 1024
  Serial.print("VBat: " ); Serial.println(measuredvbat);

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  snprintf(buffer, 1024, "VBat: %.1f", measuredvbat);
  display.clearDisplay();

  display.print(buffer);

  delay(10);
  yield();
  display.display();
}

void loop() {
  if (!digitalRead(BUTTON_A)) { Serial.println("A"); mode = 0; }
  if (!digitalRead(BUTTON_B)) { Serial.println("B"); mode = 1; }
  if (!digitalRead(BUTTON_C)) { Serial.println("C"); degree = (degree ? 0 : 1); }

  switch (mode) {
  case 0:
    displayWeather();
    break;
  case 1:
    displayBattery();
    break;
  case 2:
    break;
  }
  //delay(500);

}
