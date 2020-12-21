#include <Arduino.h>

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
  digitalWrite(EPAPER_POWER_ON, LOW);
  digitalWrite(MP3_PWR, LOW);
  digitalWrite(LED_BLUE_CONFIG, LOW);
}

void loop() {
  bool button_delay = false;
  bool alarm_stop_button_pressed = false;
  bool snooze_button_pressed = false;

  if (digitalRead(USER_BUTTON_PIN)) { alarm_stop_button_pressed = true; }
  if (digitalRead(USER2_BUTTON_PIN)) { snooze_button_pressed = true; }

  if (alarm_stop_button_pressed) {
    Serial.println("alarm stop pressed");
    button_delay = true;
    if (digitalRead(ALARM_BUTTON_LED)) {
      digitalWrite(EPAPER_POWER_ON, LOW);
      digitalWrite(ALARM_BUTTON_LED, LOW);
      digitalWrite(LED_BLUE_CONFIG, HIGH);
    } else {
      digitalWrite(EPAPER_POWER_ON, HIGH);
      digitalWrite(ALARM_BUTTON_LED, HIGH);
      digitalWrite(LED_BLUE_CONFIG, LOW);
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
