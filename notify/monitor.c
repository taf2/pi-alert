/*
 * Monitor 
 * 1. activate camera when receiving wifi signal from a remote sensor
 * 2. monitor light sensor to activate ir led's for night vision camera
 * 3. listen for trinket m0 signals to indicate we should start shutdown sequence
 * 4. send captured video/pictures to remote for more processing analysis
 *
 *
 **/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>

#define LOCAL_SERVER_PORT 1500
#define MAX_MSG 100

#include <wiringPi.h>

#define IR1 19 // GPIO 10
#define IR2 21 // GPIO 9
#define IR3 15 // GPIO 22
#define IR4 24 // GPIO 8
#define RLED  28 // GPIO 26
//#define RLED  3 // GPIO 2
#define LIGHT_SENSOR  26 // GPIO 7
#define LIGHT_TINY  40 // GPIO 21

// receive a single from trinket m0 that our battery is about to go out
#define PWROFF 37 // pwr GPIO 26, wiring 25

static char buffer[1024];

int isLight() {
  int light = digitalRead(LIGHT_TINY);
  if (light == HIGH) {
    // when the sun is out use the second light sensor not ideal we'll maybe want to have the tiny pico do this and rewire to avoid this hacky solution via pi...
    light = LOW;
    int count = 0;
    int max = 55000;
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
  } else {
    return LOW;
  }
}

void setup() {
  wiringPiSetupPhys();

  pinMode(PWROFF, INPUT);
  //pullUpDnControl(LIGHT_SENSOR, PUD_DOWN);
  pinMode(LIGHT_TINY, INPUT);
  pinMode(LIGHT_SENSOR, INPUT);
  pinMode(IR1, OUTPUT);
  pinMode(IR2, OUTPUT);
  pinMode(IR3, OUTPUT);
  pinMode(IR4, OUTPUT);
  pinMode(RLED, OUTPUT);
  int light = digitalRead(LIGHT_TINY);
  printf("light: %d\n", light);

  digitalWrite(RLED, HIGH);
  digitalWrite(IR1, HIGH);
  digitalWrite(IR2, HIGH);
  digitalWrite(IR3, HIGH);
  digitalWrite(IR4, HIGH);
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

void captureRecording(int videoPoint) {
  // capture 10 seconds of video
  int light = isLight();
  if (light == HIGH) {
	  printf("it's light out no need for the ir leds\n");
	  digitalWrite(IR1, LOW);
	  digitalWrite(IR2, LOW);
	  digitalWrite(IR3, LOW);
	  digitalWrite(IR4, LOW);
  } else {
	  printf("it's freakin dark out turn on ir leds!\n");
	  digitalWrite(IR1, HIGH);
	  digitalWrite(IR2, HIGH);
	  digitalWrite(IR3, HIGH);
	  digitalWrite(IR4, HIGH);
  }
  printf("start recording\n");
  memset(buffer,'\0',1023);
  snprintf(buffer, 1023, "/usr/bin/raspivid --rotation 0 -o /var/www/html/video%d.h264 -w 640 -h 480 -t 10000", videoPoint);
  system(buffer);
  printf("done recording\n");
}

int main(int argc, char**argv) {
  int sd, rc, n, cliLen;
  struct sockaddr_in cliAddr, servAddr;
  char msg[MAX_MSG];
  int broadcast = 1;

  static struct sigaction _sigact;
  memset(&_sigact, 0, sizeof(_sigact));
  _sigact.sa_sigaction = cleanup;
  _sigact.sa_flags = SA_SIGINFO;

  sigaction(SIGINT, &_sigact, NULL);

  setup();

  puts("warming up 1 seconds...");
  delay(1000);
  puts("starting up now");

  /* socket creation */
  sd=socket(AF_INET, SOCK_DGRAM, 0);
  if (sd<0) {
    printf("%s: cannot open socket \n",argv[0]);
    exit(1);
  }

  if (setsockopt(sd, SOL_SOCKET, SO_BROADCAST, &broadcast,sizeof broadcast) == -1) {
    perror("setsockopt (SO_BROADCAST)");
    exit(1);
  }

  /* bind local server port */
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servAddr.sin_port = htons(LOCAL_SERVER_PORT);
  rc = bind (sd, (struct sockaddr *) &servAddr,sizeof(servAddr));

  if (rc<0) {
    printf("%s: cannot bind port number %d \n", argv[0], LOCAL_SERVER_PORT);
    exit(1);
  }

  printf("%s: waiting for data on port UDP %u\n", argv[0],LOCAL_SERVER_PORT);

  double tdiff = 0;
  time_t lastMotionTime = 0;
  int recordings = 0;

  /* server infinite loop */
  while (1) {

    /* init buffer */
    memset(msg,0x0,MAX_MSG);

    /* receive message */
    cliLen = sizeof(cliAddr);
    n = recvfrom(sd, msg, MAX_MSG, 0, (struct sockaddr *) &cliAddr, &cliLen);

    if (n<0) {
      printf("%s: cannot receive data \n", argv[0]);
      continue;
    }

    /* print received message */
    printf("%s: from %s:UDP%u : %s \n", argv[0],inet_ntoa(cliAddr.sin_addr), ntohs(cliAddr.sin_port),msg);

    if (strncmp(argv[0], ":motion", 6) == 0) {
      time_t motionTime = time(NULL);

      tdiff  = difftime(motionTime, lastMotionTime);
 
      if (lastMotionTime == 0 || tdiff > 5.0) {
        // if the message is motion start recording
        digitalWrite(RLED, HIGH);
        captureRecording(recordings++);
        digitalWrite(RLED, LOW);
      } else {
        printf("too little time between last motion event and now\n");
      }
      lastMotionTime = time(NULL);
    }

    if (recordings > 100) {
	    recordings = 0; // wrap around
    }

  }/* end of server infinite loop */

  return 0;
}
