/**
 * watching pin 8, 10 and using 12 on the raspberry pi to know whether the pi is running normally
 */
#include <TinyPICO.h>

#include <math.h>
#include <SPI.h>
#include <WiFi.h>
#include <Wire.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <AsyncUDP.h>
#include <ArduinoOTA.h>


// status led for the tinypico to tell us if the pi is on or off
#define LED 26
#define LED_GPIO GPIO_NUM_26

// these pins handle daylight detection for the tinypico, we can use this to maybe adjust power consumption based on solar being available
#define PHOTOCELL 14
// we connect this to the pi to give it easier read on whether it's daylight or NOT pull HIGH when light low when dark
#define PI_DAY_LIGHT_ON 22
#define PI_DAY_LIGHT_ON_GPIO GPIO_NUM_22

// these pins handle signaling between pi and tinypico so the tinypico can know that the pi is on
#define PI_ON 19 // maps to physical pin 8 on the pi
#define PI_HERE 32 // maps to physical pin 10 on the pi
#define RES_PIN 33 // maps to physical pin 12 on the pi
#define PI_TURN_OFF 21 // we raise this HIGH when the pi should start it's shutdown sequence, maps to physical pin 37 on the pi
#define PI_TURN_OFF_GPIO GPIO_NUM_21

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

#define uS_TO_S_FACTOR 1000000  // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP  60       // Time ESP32 will go to sleep (in seconds)

unsigned int interval = 0;
bool healthCheckMode = false;
int samples[NUMSAMPLES];

static void signalPIOn();
static void signalPIOff();
static void powerNotGood();
static void powerGood();
static void ConnectToWiFi(const char * ssid, const char * pwd);
static void notify(String message);

// update code
bool WaitingOnUpdate = false;
static void updateMode();
static bool hasUpdates();

void checkHealth(int light);
int checkLight();


static const char *ssid = "hqfish";
static const char *pass = "670fisher";
AsyncUDP udp;
IPAddress localIP;
bool IsConnected = false;

TinyPICO tp = TinyPICO();

void setup() {
  Serial.begin(115200);
  pinMode(PI_ON, INPUT);
  pinMode(PI_HERE, INPUT);
  pinMode(RES_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(THERMISTOR_PIN, INPUT);
  pinMode(LED, OUTPUT);
  pinMode(PI_TURN_OFF, OUTPUT);
  pinMode(PI_EN_POWER, OUTPUT);
  pinMode(POWER_GOOD, INPUT_PULLDOWN);
  pinMode(POWER_SAVE, OUTPUT);
  pinMode(PHOTOCELL, INPUT);
  pinMode(PI_DAY_LIGHT_ON, OUTPUT);

  digitalWrite(PI_TURN_OFF, LOW);
  digitalWrite(FAN_PIN, HIGH); // keep fan from turning ON... if we had an oposite transistor maybe we could keep this low??

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
    Serial.println("it's light");
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

  float picoVolts = tp.GetBatteryVoltage();

  int on = digitalRead(PI_ON);
  if (on == HIGH) {
    if (healthCheckMode) {
      // run a health check
      digitalWrite(RES_PIN, HIGH);
      interval++;
      int piHere = digitalRead(PI_HERE);
      Serial.println((String("doing health check: ") + interval) + " piHere: " + piHere);
      if (interval > 0 && interval < 3) {
        if (piHere == HIGH) {
          // something wrong the pi should not have set this HIGH yet
          Serial.println("pi should be low still");
        } else {
          Serial.println("pi here is low");
        }
      } else if (interval > 5) {
        interval = 0;
        healthCheckMode = false;
        if (piHere == LOW) {
          Serial.println("appears the PI is not on!");
          notify("pi:ack:fail");
          digitalWrite(LED, LOW);
          gpio_hold_en(LED_GPIO);
          // for now keep looping while pi is not this is not what we'd want to actually do!
          // // we could cycle the en power pin...
          //return;
        } else {
          Serial.println("PI is on");
          digitalWrite(LED, HIGH);
          gpio_hold_en(LED_GPIO);
          notify((String("pi:on:") + c) + ":" + light  + ":" + picoVolts); // send pi on with the temp
        }
        //gpio_deep_sleep_hold_en();
        if (hasUpdates()) {
          updateMode();
        } else {
          esp_deep_sleep_start();
        }
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
    notify(String("pi:dead") + ":" + picoVolts);
    digitalWrite(LED, LOW);
    gpio_hold_en(LED_GPIO);
    // for now keep looping while pi is not this is not what we'd want to actually do!
    //return;
    //gpio_deep_sleep_hold_en();
    if (hasUpdates()) {
      updateMode();
    } else {
      esp_deep_sleep_start();
    }
  }
}

void powerGood() {
  Serial.println("power good");
  digitalWrite(PI_EN_POWER, HIGH);
  gpio_hold_en(PI_EN_POWER_GPIO);

  digitalWrite(PI_TURN_OFF, LOW);
  gpio_hold_en(PI_TURN_OFF_GPIO);

  digitalWrite(POWER_SAVE, HIGH);
  gpio_hold_en(POWER_SAVE_GPIO);
}
void powerNotGood() {
  signalPIOn();

  digitalWrite(PI_EN_POWER, LOW);
  gpio_hold_en(PI_EN_POWER_GPIO);
  digitalWrite(POWER_SAVE, LOW);
  gpio_hold_en(POWER_SAVE_GPIO);
}
static void signalPIOn() {
  digitalWrite(PI_TURN_OFF, LOW);
  gpio_hold_en(PI_TURN_OFF_GPIO);
}
static void signalPIOff() {
  digitalWrite(PI_TURN_OFF, HIGH);
}

void checkBattery() {
  int lowbat = digitalRead(POWER_GOOD); // get value of battery reading  HIGH indicates low battery LOW indicates plenty of battery
  if (lowbat == HIGH) {
    Serial.println("power may not be good");
    float picoVolts = tp.GetBatteryVoltage();
    bool isCharging = tp.IsChargingBattery();
    char buf[512];

    picoVolts = roundf(picoVolts * 100) / 100;
    int roundedVolts = (int)(picoVolts * 10);
    if (roundedVolts >= 42 && !isCharging) {
      //digitalWrite(LED, LOW);
      //gpio_hold_en(LED_GPIO);
      // pico battery is fully charged and not draining or charging so even though we're getting power not good
      // from the verter... power does actually appear good to us... proceed as normal
      powerGood();
      return;
    }
    snprintf(buf, 512, "power not good: %f and %s and %d", picoVolts, (isCharging ? "charging" : "draining"), roundedVolts);
    Serial.println(buf);
    digitalWrite(LED, LOW);
    // turning down systems - raspberry pi off
    signalPIOff();
    delay(10000); // give the PI 10 seconds to shutdown and then we cut the power
    powerNotGood();

    // tell everyone we're low battery and then we go bye bye
    notify(String("bat:low:") + (isCharging ? "charging:" : "draining:") + picoVolts);

    // go to sleep until we have more battery
    gpio_hold_en(LED_GPIO);
    //gpio_deep_sleep_hold_en();
    if (hasUpdates()) {
      updateMode();
    } else {
      esp_deep_sleep_start();
    }
  } else {
    powerGood();
  }
}

void loop() {
  if (WaitingOnUpdate) {
    Serial.println("waiting on updates");
    ArduinoOTA.handle();
  } else {
    checkBattery();
    checkHealth(checkLight());
  }
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

bool hasUpdates() {
  AsyncUDP listener;

  Serial.println("check for updates 1 second...");

  // now start listening
  if (listener.listen(1500)) {
    Serial.println("\twaiting on updates on 1500....");
    // now listen for other packets for a few seconds e.g. to see if we have any updates
    bool gotUpdate = false;
    listener.onPacket([&gotUpdate](AsyncUDPPacket packet) {
      Serial.print("UDP Packet Type: ");
      Serial.print(packet.isBroadcast()?"Broadcast":packet.isMulticast()?"Multicast":"Unicast");
      Serial.print(", From: ");
      Serial.print(packet.remoteIP());
      Serial.print(":");
      Serial.print(packet.remotePort());
      Serial.print(", To: ");
      Serial.print(packet.localIP());
      Serial.print(":");
      Serial.print(packet.localPort());
      Serial.print(", Length: ");
      Serial.print(packet.length());
      Serial.print(", Data: ");
      Serial.write(packet.data(), packet.length());
      Serial.println();
      // check our data
      if (strncmp((const char*)packet.data(), "request:update", packet.length()) == 0) {
        gotUpdate = true; 
        //reply to the client
        packet.printf("Ack:Update");
      }
    });
    delay(1 * 1000);
    Serial.println(String("Status:") + (gotUpdate ? " Update! " : "Standbye"));
    return gotUpdate;
  } else {
    Serial.println("failed to listen on 1500!");
    return false;
  }
}
void updateMode() {
  WaitingOnUpdate = true;
  Serial.println("Going into Update Mode");
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  }).onEnd([]() {
    Serial.println("\nEnd");
  }).onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  }).onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();
}
