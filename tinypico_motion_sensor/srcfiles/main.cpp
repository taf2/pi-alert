#include <TinyPICO.h>

#include <SPI.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <Wire.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <AsyncUDP.h>
#include <ArduinoOTA.h>

#define uS_TO_S_FACTOR 1000000  // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP  60       // Time ESP32 will go to sleep (in seconds)
#define LED 14
#define LED_GPIO GPIO_NUM_14
#define PIR 4
#define PIR_GPIO GPIO_NUM_4

/**
 * @fn int strend(const char *s, const char *t)
 * @brief Searches the end of string s for string t
 * @param s the string to be searched
 * @param t the substring to locate at the end of string s
 * @return one if the string t occurs at the end of the string s, and zero otherwise
 * ses: https://codereview.stackexchange.com/questions/54722/determine-if-one-string-occurs-at-the-end-of-another/54724
 */
int strend(const char *s, const size_t ls, const char *t) {
  const size_t lt = strlen(t); // find length of t
  if (ls >= lt) {  // check if t can fit in s
    // point s to where t should start and compare the strings from there
    return (0 == memcmp(t, s + (ls - lt), lt));
  }
  return 0; // t was longer than s
}

enum WakeUpMode { WakeTimer, WakeMotion, WakeOther };

bool WaitingOnUpdate = false;

static void connectToWiFi(const char * ssid, const char * pwd);

static void notify(String message);

static void updateMode();
static bool hasUpdates();

static const char *ssid = "<%= @config[:ssid] %>";
static const char *pass = "<%= @config[:pass] %>";
//static const char *node = "<%= @config[:events][:name] %>";
//
IPAddress localIP;

// Initialise the TinyPICO library
TinyPICO tp = TinyPICO();

// return true when woken by timer otherwise return false
WakeUpMode wakeup_reason() {
  esp_sleep_wakeup_cause_t wr;

  wr = esp_sleep_get_wakeup_cause();

  switch (wr) {
  case ESP_SLEEP_WAKEUP_EXT0:
    Serial.println("Wakeup caused by external signal using RTC_IO");
    return WakeMotion;
  case ESP_SLEEP_WAKEUP_EXT1:
    Serial.println("Wakeup caused by external signal using RTC_CNTL");
    return WakeOther;
  case ESP_SLEEP_WAKEUP_TIMER:
    Serial.println("Wakeup caused by timer");
    return WakeTimer;
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    Serial.println("Wakeup caused by touchpad");
    return WakeOther;
  case ESP_SLEEP_WAKEUP_ULP:
    Serial.println("Wakeup caused by ULP program");
    return WakeOther;
  default:
    Serial.printf("Wakeup was not caused by deep sleep: %d\n",wr);
    return WakeOther;
  }
  return WakeOther;
}

void setup() {
  Serial.begin(115200);
  pinMode(PIR, INPUT_PULLDOWN);
  pinMode(LED, OUTPUT);


  WakeUpMode wake = wakeup_reason();

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_sleep_enable_ext0_wakeup(PIR_GPIO , HIGH);

  tp.DotStar_SetPower( false );

  // if we were woken by timer go into check updates mode for 10 seconds
  if (wake == WakeTimer) {
    connectToWiFi(ssid, pass);

    if (hasUpdates()) {
      updateMode();
    } else {
      esp_deep_sleep_start();
    }
  } else if (wake == WakeMotion) {
    digitalWrite(LED, HIGH);
    connectToWiFi(ssid, pass);

    notify("motion");

    if (hasUpdates()) {
      updateMode();
    } else {
      do {
        Serial.println("wait for motion");
        delay(1000); // delay 5 more seconds for motion to timeout
      } while(digitalRead(PIR) == HIGH);
      esp_deep_sleep_start();
    }
  } else {
    esp_deep_sleep_start();
  }
}

void loop() {
  if (WaitingOnUpdate) {
    Serial.println("waiting on updates");
    ArduinoOTA.handle();
  } else {
    Serial.println("sleep error!");
    ESP.restart();
  }
  delay(1000);
}

void connectToWiFi(const char * ssid, const char * pwd) {
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
  Serial.print("My IP Address is: ");
  Serial.println(WiFi.localIP());
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

void notify(String message) {
  AsyncUDP sender;

  if (sender.connect(IPAddress(255,255,255,255), 1500)) {
    // sending our broadcast message to anyone listening
    sender.print(localIP.toString() + ":" + message);
    sender.close();
    Serial.println("sent motion alarm");
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
