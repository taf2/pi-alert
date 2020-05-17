   #define LED A2 // pin 4 a4

int on = 1;

void setup() {
  pinMode(LED, OUTPUT);
  digitalWrite(LED, 254); // we have light but the battery appears low start charging

}

void loop() {

  if (on) {
      digitalWrite(LED, 254); // we have light but the battery appears low start charging
      on = 0;
  } else {
      digitalWrite(LED, 0); // we have light but the battery appears low start charging
      on = 1;
  }

  delay(2000);
}
