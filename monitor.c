/*
 * Monitor sensors and inputs from battery and PIR sensor
 **/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <hiredis/hiredis.h>


#include <wiringPi.h>

// receive a single from arduinio trinket that our battery is about to go out
#define PWROFF 37
#define REDLED 36
#define BLUELED 33
#define GREENLED 31
#define PIRPIN  7

#define MAX_CAPTURES 256
#define REDIS_HOST "192.168.2.50"
#define VIDEO_COUNTER "/home/pi/video.counter"

// a value between 1 and 10
// if not init we'll default to 1
static int videoPoint = 0;
static char buffer[128];

void setup() {
  FILE *file = fopen(VIDEO_COUNTER, "rb");

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

void captureRecording(int videoPoint) {
  // capture 10 seconds of video
  digitalWrite(REDLED, HIGH);
  memset(buffer,'\0',127);
  snprintf(buffer, 127, "/usr/bin/raspivid -o /var/www/html/video%d.h264 -t 10000", videoPoint);
  system(buffer);
  digitalWrite(REDLED, LOW);
  // convert the video to mp4 for easier playback
  digitalWrite(BLUELED, HIGH);
  memset(buffer,'\0',127);
  snprintf(buffer, 127, "/usr/bin/ffmpeg -y -framerate 24 -i /var/www/html/video%d.h264 -c copy /var/www/html/video%d.mp4", videoPoint, videoPoint);
  system(buffer);
  digitalWrite(BLUELED, LOW);
}

void advanceVideoPoint(int* videoPoint) {
  FILE *file = NULL;
  ++(*videoPoint);
  if ((*videoPoint) > MAX_CAPTURES) {
    *videoPoint = 0; // wrap around
  }

  file = fopen(VIDEO_COUNTER, "wb");
  if (file) {
    fwrite(videoPoint, sizeof(int), 1, file);
    fclose(file);
  }
  file = NULL;
}


void loop() {
  int pwroff = digitalRead(PWROFF); 
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
    captureRecording(videoPoint);
    advanceVideoPoint(&videoPoint);
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
