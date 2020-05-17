/*
 * Monitor sensors and inputs from battery and PIR sensor
 **/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include <wiringPi.h>

// receive a single from arduinio trinket that our battery is about to go out
#define PWROFF 37
#define REDLED 36
#define BLUELED 33
#define GREENLED 31
#define PIRPIN  7

// a value between 1 and 10
// if not init we'll default to 1
int videoPoint = 0;

void setup() {
  FILE *file = fopen("/home/pi/video.counter", "rb");

  if (file) {
    fread(&videoPoint, sizeof(int), 1, file);
    fclose(file);
    printf("Starting with %d\n", videoPoint);
  }

  wiringPiSetupPhys();

	pinMode(PWROFF, INPUT);
	pinMode(PIRPIN, INPUT);
  pinMode(REDLED, OUTPUT);
  pinMode(BLUELED, OUTPUT);
  pinMode(GREENLED, OUTPUT);

  digitalWrite(REDLED, HIGH);
  digitalWrite(BLUELED, HIGH);
  digitalWrite(GREENLED, HIGH);
}

void loop() {
  FILE *file = NULL;
  int pwroff = digitalRead(PWROFF); 
  char buffer[128];
  printf("pwroff reading: %d\n",  pwroff);

  if (pwroff) {
    printf("we should start the shutdown sequence\n");
    system("/sbin/shutdown -h now");
  }
  delay(1000);

  if (!(digitalRead(PIRPIN))) {
    puts("No alarm...");
    digitalWrite(REDLED, LOW);
    digitalWrite(BLUELED, LOW);
    digitalWrite(GREENLED, LOW);
  } else {
    printf("Alarm %d!\n", videoPoint);
    digitalWrite(REDLED, HIGH);
    digitalWrite(GREENLED, HIGH);
    // capture 10 seconds of video
    system("/usr/bin/raspivid -o /home/pi/video.h264 -t 10000");
    digitalWrite(REDLED, LOW);
    digitalWrite(BLUELED, HIGH);
    snprintf(buffer, 127, "/usr/bin/ffmpeg -y -framerate 24 -i /home/pi/video.h264 -c copy /var/www/html/video%d.mp4", videoPoint);
    system(buffer);
    digitalWrite(BLUELED, LOW);

    ++videoPoint;
    if (videoPoint > 9) {
      videoPoint = 0; // wrap around
    }

    file = fopen("/home/pi/video.counter", "wb");
    if (file) {
      fwrite(&videoPoint, sizeof(int), 1, file);
      fclose(file);
    }
    file = NULL;
  }

}

void cleanup(int signum, siginfo_t *info, void *ptr) {
  write(STDERR_FILENO, "cleanup\n", sizeof("cleanup\n"));

  digitalWrite(REDLED, LOW);
  digitalWrite(BLUELED, LOW);
  digitalWrite(GREENLED, LOW);

  exit(0);
}

int main(int argc, char**argv) {
  static struct sigaction _sigact;
  memset(&_sigact, 0, sizeof(_sigact));
  _sigact.sa_sigaction = cleanup;
  _sigact.sa_flags = SA_SIGINFO;

  sigaction(SIGINT, &_sigact, NULL);

  setup();

  puts("warming up");
  delay(2000);
  puts("starting up now");

	while (1) {
    loop();
	}

	return 0;
}
