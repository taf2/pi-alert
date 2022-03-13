/*
 * Copyright (c) 2021, Sensirion AG
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Sensirion AG nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <TinyPICO.h>
#include <Arduino.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>
#include <WiFi.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SharpMem.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <AsyncUDP.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>

// any pins can be used
#define SHARP_SCK  18
#define SHARP_MOSI 19
#define SHARP_SS   5
#define BLACK 0
#define WHITE 1

static void ConnectToWiFi(const char * ssid, const char * pwd);
static void notify(String message);
static void updateMode();
static bool hasUpdates();

static char buf[80];
static time_t currentSecond = 0;
static bool WaitingOnUpdate = false;
static const char *ssid = "<%= @config[:ssid] %>";
static const char *pass = "<%= @config[:pass] %>";
static AsyncUDP udp;
static IPAddress localIP;
static bool IsConnected = false;
static WiFiUDP ntpUDP;
static NTPClient timeClient(ntpUDP);

static TinyPICO tp = TinyPICO();
static Adafruit_SharpMem display(SHARP_SCK, SHARP_MOSI, SHARP_SS, 400, 240);

static unsigned short intervalsSinceLastReading = 0;
static unsigned short intervalsSinceLastNotify = 0;

SensirionI2CScd4x scd4x;

void printUint16Hex(uint16_t value) {
  Serial.print(value < 4096 ? "0" : "");
  Serial.print(value < 256 ? "0" : "");
  Serial.print(value < 16 ? "0" : "");
  Serial.print(value, HEX);
}

void printSerialNumber(uint16_t serial0, uint16_t serial1, uint16_t serial2) {
  Serial.print("Serial: 0x");
  printUint16Hex(serial0);
  printUint16Hex(serial1);
  printUint16Hex(serial2);
  Serial.println();
}

void fetchWeather(const char *zipcode, NTPClient &timeClient) {
  WiFiClient client;
  HTTPClient http;
  char url[128]; // usually 98 bytes so this should be safe
  snprintf(url, 128, "http://api.openweathermap.org/data/2.5/weather?zip=%s,us&appid=%s", zipcode, "<%= @config[:weather] %>");
  http.setConnectTimeout(10000);// timeout in ms
  http.setTimeout(10000); // 10 seconds
  http.begin(client, url);
  int r =  http.GET();
	if (r < 0) {
		Serial.println("error fetching weather");
    http.end();
		return;
	}
  String body = http.getString();
  http.end();
  // TODO: get the weather icon
  /*
   * {"coord":{"lon":-76.56,"lat":39.08},
   *  "weather":[{"id":802,"main":"Clouds","description":"scattered clouds","icon":"03d"}],
   *  "base":"stations",
   *  "main":{"temp":284.8,"feels_like":280.93,"temp_min":283.15,"temp_max":286.48,"pressure":1020,"humidity":82},
   *  "visibility":10000,"wind":{"speed":5.1,"deg":180},
   *  "clouds":{"all":40},"dt":1604237734,
   *  "sys":{"type":1,"id":5056,"country":"US","sunrise":1604230454,"sunset":1604268317},
   *  "timezone":-18000,"id":0,"name":"Severna Park","cod":200}
   */
  Serial.println("weather response");
  Serial.println(body);
  Serial.println(body.length());
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, body);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  JsonObject obj = doc.as<JsonObject>();
  // contents -> { quotes -> [ { quote: "...", length: , author: ".."  },  ] }
  //JsonObject main = obj[String("main")].as<JsonObject>();
  int _timezoneOffset = obj[String("timezone")].as<int>();
  timeClient.setTimeOffset(_timezoneOffset);
	Serial.println("finished updating timezone");
}

void setup() {
  Serial.begin(115200);
  tp.DotStar_SetPower( false );

  Wire.begin();

  // start & clear the display
  display.begin();
  display.clearDisplay();

  uint16_t error;
  char errorMessage[256];

  scd4x.begin(Wire);

  // stop potentially previously started measurement
  error = scd4x.stopPeriodicMeasurement();
  if (error) {
    Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }

  uint16_t serial0;
  uint16_t serial1;
  uint16_t serial2;
  error = scd4x.getSerialNumber(serial0, serial1, serial2);
  if (error) {
    Serial.print("Error trying to execute getSerialNumber(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else {
    printSerialNumber(serial0, serial1, serial2);
  }

  // Start Measurement
  error = scd4x.startPeriodicMeasurement();
  if (error) {
    Serial.print("Error trying to execute startPeriodicMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }

  Serial.println("Waiting for first measurement... (5 sec)");
  ConnectToWiFi(ssid, pass);
  timeClient.begin();
  timeClient.update(); // get current time refreshed
  //-14400
  //timeClient.setTimeOffset(-18000);
  fetchWeather("21146", timeClient);
}

String message;
String timeMessage;
String dateMessage;

void loop() {
  if (WaitingOnUpdate) {
    Serial.println("waiting on updates");
    ArduinoOTA.handle();
  } else {
    uint16_t error;
    char errorMessage[256];

    if (intervalsSinceLastReading == 0) {
      display.clearDisplay();
    }
    // text display tests
    display.setTextSize(4);
    display.setTextColor(BLACK);
    display.setCursor(10,10);
    if (message.length()) {
      display.println(message);
    }
    if (timeMessage.length()) {
      display.setTextSize(8);
      display.println(timeMessage);
    }
    if (dateMessage.length()) {
      display.setTextSize(4);
      display.println(dateMessage);
    }
    display.refresh();
    delay(500);
    intervalsSinceLastReading++;
    intervalsSinceLastNotify++;

    if (intervalsSinceLastReading > 10) {
      // Read Measurement
      uint16_t co2;
      float temperature;
      float humidity;
      error = scd4x.readMeasurement(co2, temperature, humidity);
      if (error) {
        Serial.print("Error trying to execute readMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
        message = errorMessage;
      } else if (co2 == 0) {
        Serial.println("Invalid sample detected, skipping.");
      } else {
        currentSecond = timeClient.getEpochTime();
        if (currentSecond <= 0) {
          Serial.println("time not set yet... keep trying");
          timeClient.update(); // refresh we're not ready yet...
          currentSecond = timeClient.getEpochTime();
        }
        temperature = (temperature * 9/5) + 32;
        Serial.println(String("Co2:") + co2 + "\tTemperature:" + temperature + "\tHumidity:" + humidity);
        snprintf(buf, sizeof(buf), "%dco2 %dF %dH\n", (int)co2, (int)temperature, (int)humidity);

        message = String(buf);
        if (currentSecond > 0) {
          struct tm  ts;
          ts = *localtime(&currentSecond);
          strftime(buf, sizeof(buf), "%l:%M%p", &ts);
          timeMessage = String(buf);
          strftime(buf, sizeof(buf), "\n   %a %b, %d", &ts);
          dateMessage = String(buf);
        } else {
          timeMessage = String("loading...");
        }
      }
      intervalsSinceLastReading = 0;
      if (intervalsSinceLastNotify > 60) {
        intervalsSinceLastNotify = 0;
        notify(message);
        if (hasUpdates()) {
          updateMode();
        } else {
          timeClient.update();
        }
      }
    }
  }
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
