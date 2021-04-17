/*
 * Monitor 
 * 1. activate camera when receiving wifi signal from a remote sensor
 * 2. monitor light sensor to activate ir led's for night vision camera
 * 3. listen for trinket m0 signals to indicate we should start shutdown sequence
 * 4. send captured video/pictures to remote for more processing analysis
 *
 *
 **/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>

#include <wiringPi.h>

#define IR1 37 // GPIO 26 
#define IR2 35 // GPIO 19 
#define IR3 33 // GPIO 13
#define IR4 31 // GPIO 6
#define RLED  11 // GPIO 17
//#define RLED  3 // GPIO 2
#define LIGHT_SENSOR  13 // GPIO 27
//#define LIGHT_SENSOR  5 // GPIO 3

// receive a single from trinket m0 that our battery is about to go out
#define PWROFF 29 // pwr GPIO 5, wiring 21

int isLight() {
	int light = LOW;
	int count = 0;
	int max = 150000;
	pinMode(LIGHT_SENSOR, OUTPUT);
	digitalWrite(LIGHT_SENSOR, LOW);
	delay(100);
	pinMode(LIGHT_SENSOR, INPUT);
	do {
		light = digitalRead(LIGHT_SENSOR); 
		count++;
	} while (count < max && light == LOW);
	printf("count: %d %d\n", count, light);
	return count < max ? HIGH : LOW;
}

void setup() {
  wiringPiSetupPhys();

  pinMode(PWROFF, INPUT);
  //pullUpDnControl(LIGHT_SENSOR, PUD_DOWN);
  pinMode(LIGHT_SENSOR, INPUT);
  pinMode(IR1, OUTPUT);
  pinMode(IR2, OUTPUT);
  pinMode(IR3, OUTPUT);
  pinMode(IR4, OUTPUT);
  pinMode(RLED, OUTPUT);

  digitalWrite(RLED, HIGH);
  digitalWrite(IR1, HIGH);
  digitalWrite(IR2, HIGH);
  digitalWrite(IR3, HIGH);
  digitalWrite(IR4, HIGH);
}

void loop() {
  int light = isLight();

  if (light == HIGH) {
	  printf("it's light\n");
	  digitalWrite(RLED, LOW);
  } else {
	  printf("it's dark\n");
	  digitalWrite(RLED, HIGH);
  }

  delay(1000);

}

void cleanup(int signum, siginfo_t *info, void *ptr) {
  write(STDERR_FILENO, "cleanup\n", sizeof("cleanup\n"));

  digitalWrite(RLED, LOW);
  digitalWrite(IR1, LOW);
  digitalWrite(IR2, LOW);
  digitalWrite(IR3, LOW);
  digitalWrite(IR4, LOW);

  exit(0);
}

int main(int argc, char**argv) {
  static struct sigaction _sigact;
  memset(&_sigact, 0, sizeof(_sigact));
  _sigact.sa_sigaction = cleanup;
  _sigact.sa_flags = SA_SIGINFO;

  sigaction(SIGINT, &_sigact, NULL);

  setup();

  puts("warming up 1 seconds...");
  delay(1000);
  puts("starting up now");

  while (1) {
    loop(); 
  }

  return 0;
}
