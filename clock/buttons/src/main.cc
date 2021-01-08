#include <Arduino.h>
#include <ArduinoJson.h>

#define VBATPIN A13
#define EPAPER_POWER_ON 12 // control power to the display epaper control ItsyBitsy board
#define LED_BLUE_CONFIG 13
#define USER2_BUTTON_PIN 14 // external user interface button to toggle on / off alarm snooze function e.g. trigger alarm again
#define ALARM_BUTTON_LED 15
#define USER_BUTTON_PIN 21 // external user interface button to toggle on / off alarm
#define MP3_PWR 27 // connected to the enable pin on the buck converter 3.3v to mp3 df mini player
#define SNOOZE_BUTTON_LED 32
#define BUZZER 33


void setup() {
  Serial.begin(115200);
  Serial1.begin(9600); // communications to the epaper ItsyBitsy

  pinMode(EPAPER_POWER_ON, OUTPUT);
  pinMode(LED_BLUE_CONFIG, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(MP3_PWR, OUTPUT);
  digitalWrite(BUZZER, HIGH); // ensure 
  digitalWrite(MP3_PWR, LOW);
  digitalWrite(LED_BLUE_CONFIG, LOW);

  pinMode(USER_BUTTON_PIN, INPUT_PULLDOWN);
  pinMode(USER2_BUTTON_PIN, INPUT_PULLDOWN);
  pinMode(ALARM_BUTTON_LED, OUTPUT);
  pinMode(SNOOZE_BUTTON_LED, OUTPUT);

  digitalWrite(ALARM_BUTTON_LED, LOW);
  digitalWrite(SNOOZE_BUTTON_LED, LOW);
  digitalWrite(EPAPER_POWER_ON, HIGH);
  digitalWrite(MP3_PWR, LOW);
  digitalWrite(LED_BLUE_CONFIG, LOW);
}

short test_seq = 0;

void loop() {
  bool button_delay = false;
  bool alarm_stop_button_pressed = false;
  bool snooze_button_pressed = false;

  if (Serial1.available()) {
    StaticJsonDocument<128> doc;
    DeserializationError err = deserializeJson(doc, Serial1);
    if (err == DeserializationError::Ok) {
      Serial.println("received doc");
      const String status = doc["status"].as<String>();
      if (status == "success") {
        Serial.println("success");
      } else {
        Serial.println("error");
      }
    }
  }

  if (digitalRead(USER_BUTTON_PIN)) { alarm_stop_button_pressed = true; }
  if (digitalRead(USER2_BUTTON_PIN)) { snooze_button_pressed = true; }

  if (alarm_stop_button_pressed) {
    Serial.println("alarm stop pressed");
    button_delay = true;
    if (digitalRead(ALARM_BUTTON_LED) && test_seq == 0) {
      digitalWrite(EPAPER_POWER_ON, LOW);
      digitalWrite(ALARM_BUTTON_LED, LOW);
      digitalWrite(LED_BLUE_CONFIG, LOW);
    } else {
      digitalWrite(EPAPER_POWER_ON, HIGH);
      delay(1000); // give the device 1 second to start up and then send the serial message
      digitalWrite(ALARM_BUTTON_LED, HIGH);
      digitalWrite(LED_BLUE_CONFIG, HIGH);

      if (test_seq == 0) {
        Serial.println("write to Serial1");
        StaticJsonDocument<1024> doc;
        doc["time"] = 1608604137;
        doc["ftime"] = "10:30";
        doc["fdate"] = "Dec 29th";
        doc["temp"] = 32;
        doc["quote"] = "awesome stuff";
        doc["author"] = "me";
        size_t bytesWritten = serializeJson(doc, Serial1);
        Serial.println(bytesWritten);
      } else {
        Serial.println("write to Serial1");
        StaticJsonDocument<1024> doc;
        doc["clear"] = true;
        size_t bytesWritten = serializeJson(doc, Serial1);
        Serial.println(bytesWritten);
      }
      Serial.print("test seq:");
      Serial.println(test_seq);

      ++test_seq;
      if (test_seq > 2) { // allow clear twice
        test_seq = 0;
      }
    }
  }

  if (snooze_button_pressed) {
    Serial.println("snooze pressed");
    button_delay = true;
    if (digitalRead(SNOOZE_BUTTON_LED)) {
      digitalWrite(MP3_PWR, LOW);
      digitalWrite(SNOOZE_BUTTON_LED, LOW);
      digitalWrite(LED_BLUE_CONFIG, HIGH);
    } else {
      digitalWrite(LED_BLUE_CONFIG, LOW);
      digitalWrite(MP3_PWR, HIGH);
      digitalWrite(SNOOZE_BUTTON_LED, HIGH);
      delay(1000);
      digitalWrite(BUZZER, LOW); // play music
      delay(100);
      digitalWrite(BUZZER, HIGH); // play music
    }
  }

  if (button_delay) {
    delay(500);
  }
}
