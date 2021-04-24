#include <TinyPICO.h>

#include <SPI.h>
#include <WiFi.h>
#include <Wire.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <AsyncUDP.h>

#define LED 25
#define PIR 14
#define AUDIO 4

static void connectToWiFi(const char * ssid, const char * pwd);

static const char *ssid = "<%= @config[:ssid] %>";
static const char *pass = "<%= @config[:pass] %>";
//static const char *node = "<%= @config[:events][:name] %>";
//
const int SampleWindow = 50; // Sample window width in mS (50 mS = 20Hz)

// Initialise the TinyPICO library
TinyPICO tp = TinyPICO();

AsyncUDP udp;

void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason) {
  case ESP_SLEEP_WAKEUP_EXT0:
    Serial.println("Wakeup caused by external signal using RTC_IO");
    break;
  case ESP_SLEEP_WAKEUP_EXT1:
    Serial.println("Wakeup caused by external signal using RTC_CNTL");
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
    Serial.println("Wakeup caused by timer");
    break;
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    Serial.println("Wakeup caused by touchpad");
    break;
  case ESP_SLEEP_WAKEUP_ULP:
    Serial.println("Wakeup caused by ULP program");
    break;
  default:
    Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason);
    break;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(AUDIO, INPUT);
//  connectToWiFi(ssid, pass);
  pinMode(PIR, INPUT);
  pinMode(LED, OUTPUT);

  digitalWrite(LED, HIGH);

  print_wakeup_reason();


  esp_sleep_enable_ext0_wakeup(GPIO_NUM_14, HIGH);

  connectToWiFi(ssid, pass);

  tp.DotStar_SetPower( false );

  // send a signal
  if (udp.connect(IPAddress(255,255,255,255), 1500)) {
    Serial.println("Ping-Alive");
    udp.print("Ping-Alive");
  }*/

  // wait for motion to wake up
  esp_deep_sleep_start();
}

void loop() {
  unsigned long startMillis = millis();  // Start of sample window
  unsigned int peakToPeak   = 0;         // Peak-to-Peak level

  unsigned int sample    = 0;
  unsigned int signalMax = 0;
  unsigned int signalMin = 1023;

  // collect data for 50 mS
  while (millis() - startMillis < SampleWindow) {
    sample = analogRead(AUDIO);
    if (sample < 1023) { // toss out spurious readings
      if (sample > signalMax) {
        signalMax = sample;  // save just the max levels
      } else if (sample < signalMin) {
        signalMin = sample;  // save just the min levels
      }
    }
  }

  peakToPeak = signalMax - signalMin;  // max - min = peak-peak amplitude
  double volts = (peakToPeak * 3.3) / 1024;  // convert to volts

  Serial.println(volts);
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
  Serial.print("My IP Address is: ");
  Serial.println(WiFi.localIP());
}
