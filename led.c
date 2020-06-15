/*
 * Test program for Led verification
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <wiringPi.h>

#define REDLED 36
#define YELLOWLED 31 // yellow, note used but, reserved for a processing pin if after capturing video we need some indicator that we're processing.


void setup() {
  wiringPiSetupPhys();
  pinMode(REDLED, OUTPUT);
//  pinMode(BLUELED, OUTPUT); // connected to controlled via PIR pin being high
  pinMode(YELLOWLED, OUTPUT);
  digitalWrite(REDLED, HIGH); // start up HIGH so we can see it working
  digitalWrite(YELLOWLED, HIGH); // while we're running turn this to high
}

void loop() {
}

void cleanup(int signum, siginfo_t *info, void *ptr) {
  write(STDERR_FILENO, "cleanup\n", sizeof("cleanup\n"));

  digitalWrite(REDLED, LOW);
//  digitalWrite(BLUELED, LOW);
  digitalWrite(YELLOWLED, LOW);

  exit(0);
}

int main(int argc, char**argv) {
  static struct sigaction _sigact;
  memset(&_sigact, 0, sizeof(_sigact));
  _sigact.sa_sigaction = cleanup;
  _sigact.sa_flags = SA_SIGINFO;

  sigaction(SIGINT, &_sigact, NULL);

  setup();

  puts("starting up now");

	while (1) {
    loop();
	}

	return 0;
}
