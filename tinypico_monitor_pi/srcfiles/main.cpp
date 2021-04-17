/**
 * watching pin 8, 10 and using 12 on the raspberry pi to know whether the pi is running normally
 */
#include <TinyPICO.h>

#include <SPI.h>
#include <WiFi.h>
#include <Wire.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <AsyncUDP.h>

// status led for the tinypico to tell us if the pi is on or off
#define LED 5
#define LED_GPIO GPIO_NUM_5

// these pins handle daylight detection for the tinypico, we can use this to maybe adjust power consumption based on solar being available
#define PHOTOCELL 14
// we connect this to the pi to give it easier read on whether it's daylight or NOT pull HIGH when light low when dark
#define PI_DAY_LIGHT_ON 22
#define PI_DAY_LIGHT_ON_GPIO GPIO_NUM_22

// these pins handle signaling between pi and tinypico so the tinypico can know that the pi is on
#define PI_ON 21 // maps to physical pin 8 on the pi
#define PI_HERE 32 // maps to physical pin 10 on the pi
#define RES_PIN 33 // maps to physical pin 12 on the pi

#define FAN_PIN 15 // pull low to open transistor to turn fan power on via the 3.3v power rail

// from the verter e.g. our power source we'll pull power_save high if our power is NOT good
#define POWER_GOOD 22
#define POWER_SAVE 5
#define POWER_SAVE_GPIO GPIO_NUM_5

// control power to the pi if we pull this low the pi will turn off normally we hold this high to keep the pi on unless we're low bat
#define PI_EN_POWER 18
#define PI_EN_POWER_GPIO GPIO_NUM_18

// to figure out the tempature we use a thermistor to measure how warm/cold it is in the camera case
// if it gets too hot we'll turn on the fan
#define NUMSAMPLES 5
#define SERIES_RESISTOR 10000
#define THERMISTOR_PIN 4
// resistance at 25 degrees C
#define THERMISTORNOMINAL 10000
// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25
// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 3950

#define uS_TO_S_FACTOR 1000000  //Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP  5       //Time ESP32 will go to sleep (in seconds)

unsigned int interval = 0;
bool healthCheckMode = false;
int samples[NUMSAMPLES];

static void ConnectToWiFi(const char * ssid, const char * pwd);
static void notify(String message);
void checkHealth(int light);
int checkLight();


static const char *ssid = "<%= @config[:ssid] %>";
static const char *pass = "<%= @config[:pass] %>";
AsyncUDP udp;
IPAddress localIP;
bool IsConnected = false;

void setup() {
  Serial.begin(115200);
  pinMode(PI_ON, INPUT);
  pinMode(PI_HERE, INPUT);
  pinMode(RES_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(THERMISTOR_PIN, INPUT);
  pinMode(LED, OUTPUT);
  pinMode(PI_EN_POWER, OUTPUT);
  pinMode(POWER_GOOD, INPUT);
  pinMode(POWER_SAVE, OUTPUT);
  pinMode(PHOTOCELL, INPUT);

  digitalWrite(FAN_PIN, HIGH);
  digitalWrite(LED, HIGH);

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
}

float averageReading() {
  int i;
  float average = 0;
  // take N samples in a row, with a slight delay
  for (i=0; i< NUMSAMPLES; i++) {
   samples[i] = analogRead(THERMISTOR_PIN);
   delay(10);
  }

  // average all the samples out
  average = 0;
  for (i=0; i< NUMSAMPLES; i++) {
     average += samples[i];
  }
  average /= NUMSAMPLES;
  return average;
}

float getTemp() {
  // see: https://learn.adafruit.com/thermistor/using-a-thermistor
  float reading = averageReading() / 3.8;
  //Serial.print("Analog reading ");
//  Serial.println(reading); // 1574

  // convert the value to resistance
  reading = (1023 / reading)  - 1;     // (1023/ADC - 1)
  reading = SERIES_RESISTOR / reading;  // 10K / (1023/ADC - 1)
//  Serial.print("Thermistor resistance ");
//  Serial.println(reading);
  float steinhart;
  steinhart = reading / THERMISTORNOMINAL;     // (R/Ro)
  steinhart = log(steinhart);                  // ln(R/Ro)
  steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;                 // Invert
  steinhart -= 273.15;                         // convert absolute temp to C

  Serial.print("Temperature ");
  Serial.print(steinhart);
  Serial.println(" *C");
  return steinhart;
}

int checkLight() {
  int light = analogRead(PHOTOCELL);
  if (light > 80) { // light
    digitalWrite(PI_DAY_LIGHT_ON, HIGH);
    gpio_hold_en(PI_DAY_LIGHT_ON_GPIO);
  } else {
    digitalWrite(PI_DAY_LIGHT_ON, LOW);
    gpio_hold_en(PI_DAY_LIGHT_ON_GPIO);
  }
  return light;
}

void checkHealth(int light) {
  float c = getTemp();

  if (c > 36) {
    digitalWrite(FAN_PIN, LOW); // turn fan on
    notify(String("temp:high:") + c);
  } else {
    digitalWrite(FAN_PIN, HIGH); // turn fan off
  }

  int on = digitalRead(PI_ON);
  if (on == HIGH) {
    if (healthCheckMode) {
      // run a health check
      digitalWrite(RES_PIN, HIGH);
      interval++;
      if (interval > 5) {
        interval = 0;
        healthCheckMode = false;
        int piHere = digitalRead(PI_HERE);
        if (piHere == LOW) {
          Serial.println("appears the PI is not on!");
          notify("pi:ack:fail");
          digitalWrite(LED, LOW);
          // for now keep looping while pi is not this is not what we'd want to actually do!
          //return;
        } else {
          Serial.println("PI is on");
          digitalWrite(LED, HIGH);
          gpio_hold_en(LED_GPIO);
          notify((String("pi:on:") + c) + ":" + light); // send pi on with the temp
        }
        gpio_deep_sleep_hold_en();
        esp_deep_sleep_start();
      }
    } else {
      // it appears on good news
      interval++;
      if (interval > 5) {
        interval = 0;
        healthCheckMode = true;
        Serial.println("start health check");
      } else {
        digitalWrite(RES_PIN, LOW);
      }
    }
  } else {
    // seems to be off!
    interval = 0;
    Serial.println("pi appears to be off!");
    notify("pi:dead");
    digitalWrite(LED, LOW);
    // for now keep looping while pi is not this is not what we'd want to actually do!
    //return;
    gpio_deep_sleep_hold_en();
    esp_deep_sleep_start();
  }
}

void checkBattery() {
  int lowbat = digitalRead(POWER_GOOD); // get value of battery reading  HIGH indicates low battery LOW indicates plenty of battery
  if (lowbat == HIGH) {
    Serial.println("power not good");
    // turning down systems - raspberry pi off
    digitalWrite(PI_EN_POWER, LOW);
    gpio_hold_en(PI_EN_POWER_GPIO);

    digitalWrite(POWER_SAVE, LOW);
    gpio_hold_en(POWER_SAVE_GPIO);

    // tell everyone we're low battery and then we go bye bye
    notify("bat:low");

    // go to sleep until we have more battery
    gpio_deep_sleep_hold_en();
    esp_deep_sleep_start();
  } else {
    Serial.println("power good");
    digitalWrite(PI_EN_POWER, HIGH);
    gpio_hold_en(PI_EN_POWER_GPIO);

    digitalWrite(POWER_SAVE, HIGH);
    gpio_hold_en(POWER_SAVE_GPIO);
  }
}

void loop() {
  checkBattery();
  checkHealth(checkLight());
  delay(1000);
}

void ConnectToWiFi(const char * ssid, const char * pwd) {
  if (!IsConnected) {
    Serial.println("Connecting to WiFi network: " + String(ssid));

    WiFi.begin(ssid, pwd);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.println("Connection Failed! Rebooting...");
      delay(5000);
      ESP.restart();
    }
    // this has a dramatic effect on packet RTT
    WiFi.setSleep(WIFI_PS_NONE);
    localIP = WiFi.localIP();
    IsConnected = true;
    Serial.println(WiFi.localIP());
  }
}
static void notify(String message) {
  ConnectToWiFi(ssid, pass);
  if (udp.connect(IPAddress(255,255,255,255), 1500)) {
    udp.print(localIP.toString() + ":" + message);
    udp.print(localIP.toString() + ":" + message);
    udp.print(localIP.toString() + ":" + message);
  }
}
