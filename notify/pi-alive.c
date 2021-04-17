/*
  This program requires wiringpi and handles communication with the tinypico esp32.
  It must be running otherwise the tinypico will reset the pi every 5 minutes.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <wiringPi.h>

#define PI_ON 8
#define PI_HERE 10
#define RES_PIN 12 // switch PI_HERE to HIGH when monitor goes HIGH

void setup() {
  wiringPiSetupPhys();

  pinMode(PI_ON, OUTPUT);
  pinMode(PI_HERE, OUTPUT);
  pinMode(RES_PIN, INPUT);

  digitalWrite(PI_ON, HIGH);
  printf("we're on\n");
}

void loop() {
  int monitor = digitalRead(RES_PIN);

  if (monitor == HIGH) {
    digitalWrite(PI_HERE, HIGH);
  } else {
    digitalWrite(PI_HERE, LOW);
  }

  delay(100);

}

void cleanup(int signum, siginfo_t *info, void *ptr) {
  write(STDERR_FILENO, "cleanup\n", sizeof("cleanup\n"));

  digitalWrite(PI_ON, LOW);
  digitalWrite(PI_HERE, LOW);

  exit(0);
}

int main(int argc, char**argv) {
  static struct sigaction _sigact;
  memset(&_sigact, 0, sizeof(_sigact));
  _sigact.sa_sigaction = cleanup;
  _sigact.sa_flags = SA_SIGINFO;

  sigaction(SIGINT, &_sigact, NULL);

  setup();

  puts("warming up 3 seconds...");
  delay(3000);
  puts("starting up now");

	while (1) {
    loop();
	}

	return 0;
}
