/*
 * Monitor sensors and inputs from battery and PIR sensor
 *
 * monitor multiple GPIO pins
 *
 * input pins:
 *
 *   PWROFF - tells us we are low on battery and need to execute shutdown sequence right away
 *   PIRPIN - tells us we have detected motion within our field of view and should activate the camera and start recording
 *            we'll save our recording into an http accessible folder and xadd into a redis stream to have it reviewed
 *
 * output pins
 *
 * REDLED - light this up when we're recording
 * YELLOWLED - this should be on when this program is running otherwise it should be off
 * BLUELED - this is light when the PIR sensor is detecting motion we should not touch it since the PIR sensor has it hard wired
 *
 **/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <hiredis/hiredis.h>


#include <wiringPi.h>

#define DELAY_BETWEEN_ALARM 15

// receive a single from arduinio trinket that our battery is about to go out
#define PWROFF 29 // moved this so it's closer to the trinket
#define REDLED 36
#define BLUELED 37 // blue
#define YELLOWLED 31 // yellow, note used but, reserved for a processing pin if after capturing video we need some indicator that we're processing.
#define PIRPIN 32

#define MAX_CAPTURES 256
#define REDIS_HOST "192.168.2.50"
#define HOST_IP "192.168.2.75"
#define HOST_NAME "basement"
#define VIDEO_COUNTER "/home/pi/video.counter"

// a value between 1 and 10
// if not init we'll default to 1
static int videoPoint = 0;
static char buffer[1024];
static time_t lastAlarm = 0;

void setup() {
  FILE *file = fopen(VIDEO_COUNTER, "rb");

  if (file) {
    fread(&videoPoint, sizeof(int), 1, file);
    fclose(file);
    printf("Starting with %d\n", videoPoint);
  }

  lastAlarm = time(0);

  wiringPiSetupPhys();

  pinMode(PWROFF, INPUT);
  pinMode(PIRPIN, INPUT);
  pinMode(REDLED, OUTPUT);
  pinMode(BLUELED, OUTPUT);
  pinMode(YELLOWLED, OUTPUT);

  digitalWrite(REDLED, HIGH); // start up HIGH so we can see it working
  digitalWrite(YELLOWLED, HIGH); // while we're running turn this to high
  digitalWrite(BLUELED, HIGH);
}

void signalNewRecording(int videoPoint) {
//  redisContext *c;
//  redisReply *reply;

//  struct timeval timeout = { 1, 500000 }; // 1.5 seconds

//  c = redisConnectWithTimeout(REDIS_HOST, 6379, timeout);

   // XADD: ERR Invalid stream ID specified as stream command argument
                                       // //'["object_detect", {"name": "basement", "url": "http://192.168.2.57/video17.mp4"}]'
  memset(buffer,'\0',1023);
  snprintf(buffer, 1023, "redis-cli -h %s XADD tasks '*' task '[\"object_detect\", {\"name\": \"%s\", \"url\": \"http://%s/video%d.h264\"}]'", REDIS_HOST, HOST_NAME, HOST_IP, videoPoint);
  printf("execute redis cmd: '''%s'''\n", buffer);
  system(buffer);

//  reply = redisCommand(c, buffer);
//  printf("XADD: %s\n", reply->str);
//  freeReplyObject(reply);
//  redisFree(c);

  // xadd tasks * task '["object_detect", {"name": "basement", "url": "http://192.168.2.57/video17.mp4"}]'
  // we need to xadd to the tasks redis stream (indicate our name and the url from which you can fetch our recording) 
}

void captureRecording(int videoPoint) {
  // capture 10 seconds of video
  digitalWrite(REDLED, HIGH);
  memset(buffer,'\0',1023);
  snprintf(buffer, 1023, "/usr/bin/raspivid --rotation 270 -o /var/www/html/video%d.h264 -w 640 -h 480 -t 10000", videoPoint);
  system(buffer);
  // convert the video to mp4 for easier playback
  // NOTE: we commented this out it uses a lot of CPU
  //digitalWrite(YELLOWLED, HIGH);
//  memset(buffer,'\0',1023);
//  snprintf(buffer, 1023, "/usr/bin/ffmpeg -y -framerate 24 -i /var/www/html/video%d.h264 -c copy /var/www/html/video%d.mp4", videoPoint, videoPoint);
//  system(buffer);
  //digitalWrite(YELLOWLED, LOW);

  signalNewRecording(videoPoint);
  digitalWrite(REDLED, LOW);
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

int cycles = 0;

void loop() {
  int secondsSinceLastAlarm = 0;
  time_t alarmTime = 0;
  int pwroff = digitalRead(PWROFF); 
  ++cycles;

  printf("pwroff reading: %d\n",  pwroff);
  /*
   * use this block of code to test LED functions
      digitalWrite(REDLED, HIGH);
      delay(1000);
      return;
  */

  if (pwroff && cycles > 20) {
    printf("we should start the shutdown sequence\n");
    digitalWrite(REDLED, HIGH);
    digitalWrite(YELLOWLED, LOW);
    delay(500);
    system("/sbin/shutdown -h now");
    delay(10000);
    return;
  }
  delay(500);

  if (digitalRead(PIRPIN)) {
    digitalWrite(BLUELED, HIGH);
    alarmTime = time(0);
    secondsSinceLastAlarm = (int)difftime(alarmTime, lastAlarm);
    if (secondsSinceLastAlarm > DELAY_BETWEEN_ALARM) {
      printf("Alarm %d!\n", videoPoint);
      digitalWrite(REDLED, HIGH);
      lastAlarm = alarmTime;
      captureRecording(videoPoint);
      advanceVideoPoint(&videoPoint);
      lastAlarm = time(0);
    } else {
      printf("Last alarmed within the same minute, %d seconds remaining.\n", (DELAY_BETWEEN_ALARM-secondsSinceLastAlarm));
      if ((secondsSinceLastAlarm % 2) == 1) {
        digitalWrite(REDLED, LOW);
      } else {
        digitalWrite(REDLED, HIGH);
      }
      delay(500);
    }
  } else {
    digitalWrite(BLUELED, LOW);
    //puts("No alarm...");
    digitalWrite(REDLED, LOW);
  }

}

void cleanup(int signum, siginfo_t *info, void *ptr) {
  write(STDERR_FILENO, "cleanup\n", sizeof("cleanup\n"));

  digitalWrite(REDLED, LOW);
  digitalWrite(BLUELED, LOW);
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

  puts("warming up 3 seconds...");
  delay(3000);
  puts("starting up now");

	while (1) {
    loop();
	}

	return 0;
}
