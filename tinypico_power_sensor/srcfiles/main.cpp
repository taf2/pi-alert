/**
 * monitor power via Adafruit_INA260, power sense breakout board
 */
#include <TinyPICO.h>

#include <SPI.h>
#include <WiFi.h>
#include <Wire.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <AsyncUDP.h>
#include <ArduinoOTA.h>

#include <Adafruit_INA260.h>

#define LED_FULL 15
#define LED_HALF 27
#define LED_LOW 26
#define BUTTON 14
#define POWER_GOOD 4

#define uS_TO_S_FACTOR 1000000  // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP  120       // Time ESP32 will go to sleep (in seconds)
static void notify(String message);

static const char *ssid = "<%= @config[:ssid] %>";
static const char *pass = "<%= @config[:pass] %>";
AsyncUDP udp;
IPAddress localIP;
bool IsConnected = false;

// update code
bool WaitingOnUpdate = false;
static void updateMode();
static bool hasUpdates();

Adafruit_INA260 ina260 = Adafruit_INA260();


TinyPICO tp = TinyPICO();

bool print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason) {
  case ESP_SLEEP_WAKEUP_EXT0:
    Serial.println("Wakeup caused by external signal using RTC_IO");
    return true;
  case ESP_SLEEP_WAKEUP_EXT1:
    Serial.println("Wakeup caused by external signal using RTC_CNTL");
    return true;
  case ESP_SLEEP_WAKEUP_TIMER:
    Serial.println("Wakeup caused by timer");
    return false;
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    Serial.println("Wakeup caused by touchpad");
    return false;
  case ESP_SLEEP_WAKEUP_ULP:
    Serial.println("Wakeup caused by ULP program");
    return false;
  default:
    Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason);
    return false;
  }
}

void checkReadings(bool activeLED=false) {

  float current = ina260.readCurrent();
  float volts   = ina260.readBusVoltage();
  float power   = ina260.readPower();
  int   powerGood = digitalRead(POWER_GOOD);

  Serial.print("Current: ");
  Serial.print(current);
  Serial.println(" mA");

  Serial.print("Bus Voltage: ");
  Serial.print(volts);
  Serial.println(" mV");

  Serial.print("Power: ");
  Serial.print(power);
  Serial.println(" mW");

  Serial.println();
  if (activeLED) {
    Serial.println("pressed button");

    if (volts > 3952) {
      digitalWrite(LED_FULL, HIGH);
    } else if (3500) {
      digitalWrite(LED_HALF, HIGH);
    } else {
      digitalWrite(LED_LOW, HIGH);
    }

    delay(5000); // keep active for 5 seconds

  }
  notify(String("bat:cur") + ":" + current + ":volts:" + volts + ":power:" + power + ((powerGood == LOW) ? ":pg" : ":pb"));
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_FULL, OUTPUT);
  pinMode(LED_HALF, OUTPUT);
  pinMode(LED_LOW, OUTPUT);
  pinMode(BUTTON, INPUT);
  pinMode(POWER_GOOD, INPUT_PULLDOWN);

  while (!ina260.begin()) {
    Serial.println("Couldn't find INA260 chip");
    digitalWrite(LED_LOW, HIGH);
    delay(1000);
    digitalWrite(LED_LOW, LOW);
    delay(1000);
  }
  Serial.println("Found INA260 chip");

  // allow button press to wake up
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_14,HIGH);

  // wake up on timer
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  checkReadings(print_wakeup_reason());

  if (hasUpdates()) {
    updateMode();
  } else {
    esp_deep_sleep_start();
  }
}

void loop() {
  if (WaitingOnUpdate) {
    Serial.println("waiting on updates");
    ArduinoOTA.handle();
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

void notify(String message) {
  ConnectToWiFi(ssid, pass);
  if (udp.connect(IPAddress(255,255,255,255), 1500)) {
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
