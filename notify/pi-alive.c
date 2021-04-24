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
#define PWROFF 37 // pwr GPIO 26, wiring 25

int interval = 0;
int shutdown = 0;

void setup() {
  wiringPiSetupPhys();

  pinMode(PI_ON, OUTPUT);
  pinMode(PI_HERE, OUTPUT);
  pinMode(RES_PIN, INPUT);
  pinMode(PWROFF, INPUT);

  digitalWrite(PI_ON, HIGH);
  printf("we're on\n");
}

void loop() {
  int monitor = digitalRead(RES_PIN);

  if (monitor == HIGH) {
    interval++;
    if (interval > 0 && interval < 2) {
      digitalWrite(PI_HERE, LOW);
    } else {
      digitalWrite(PI_HERE, HIGH);
    }
  } else {
    interval = 0;
    digitalWrite(PI_HERE, LOW);
  }

  if (digitalRead(PWROFF) == HIGH) {
    if (shutdown > 2) { 
      printf("start shutdown sequence\n");
      system("/usr/bin/sudo /usr/sbin/halt");
    }
    shutdown++;
  } else {
    shutdown = 0;
  }

  //delay(1000);
  int status = system("ping -c 1 192.168.2.1 > /dev/null 2>&1");
  if (status == 0) {
    // success
    digitalWrite(PI_HERE, HIGH);
  } else {
    // failure
    digitalWrite(PI_HERE, LOW);
  }

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

  delay(1000);

	while (1) {
    loop();
	}

	return 0;
}
