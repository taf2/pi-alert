#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MotorShield.h>

// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield();
// Or, create it with a different I2C address (say for stacking)
// Adafruit_MotorShield AFMS = Adafruit_MotorShield(0x61);

// Connect a stepper motor with 200 steps per revolution (1.8 degree)
// to motor port #2 (M3 and M4)
Adafruit_StepperMotor *myMotor = AFMS.getStepper(200, 2);

void blink(int pin, int seconds) {
  for (int i = 0; i < seconds; ++i) {
    digitalWrite(pin, HIGH);
    delay(500);
    digitalWrite(pin, LOW);
    delay(500);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(A0, OUTPUT);
  pinMode(A1, OUTPUT);
  pinMode(A2, INPUT);

	AFMS.begin();  // create with the default frequency 1.6KHz

	myMotor->setSpeed(30);  // 30 rpm


}

void loop() {
  int motion = digitalRead(A2);
  if (motion == HIGH) {
    blink(A0, 1);
		/*Serial.println("Single coil steps");
		myMotor->step(100, FORWARD, SINGLE);
		myMotor->step(100, BACKWARD, SINGLE);

		Serial.println("Double coil steps");
		myMotor->step(100, FORWARD, DOUBLE);
		myMotor->step(100, BACKWARD, DOUBLE);

		Serial.println("Interleave coil steps");
		myMotor->step(100, FORWARD, INTERLEAVE);
		myMotor->step(100, BACKWARD, INTERLEAVE);

		Serial.println("Microstep steps");*/
		myMotor->step(50, FORWARD, MICROSTEP);
		myMotor->step(50, BACKWARD, MICROSTEP);
    blink(A1, 1);
  }
}
