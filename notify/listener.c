/* fpont 12/99 */
/* pont.net    */
/* udpServer.c */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h> /* close() */
#include <string.h> /* memset() */
#include <signal.h>

//for Mac OS X
#include <stdlib.h>

#include <wiringPi.h>

#define LOCAL_SERVER_PORT 1500
#define MAX_MSG 100

#define IR1 37 // GPIO 26 
#define IR2 35 // GPIO 19 
#define IR3 33 // GPIO 13
#define IR4 31 // GPIO 6
#define RLED  11 // GPIO 17
//#define RLED  3 // GPIO 2
#define LIGHT_SENSOR  13 // GPIO 27
//#define LIGHT_SENSOR  5 // GPIO 3
#define PWROFF 29 // pwr GPIO 5, wiring 21

static char buffer[1024];

void cleanup(int signum, siginfo_t *info, void *ptr) {
  write(STDERR_FILENO, "cleanup\n", sizeof("cleanup\n"));

  digitalWrite(RLED, LOW);
  digitalWrite(IR1, LOW);
  digitalWrite(IR2, LOW);
  digitalWrite(IR3, LOW);
  digitalWrite(IR4, LOW);

  exit(0);
}

// rpi doesn't have an analog gpio so we have to estimate lightness... maybe we'll have the pir sensor arduinio's send in the packet...
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
  // convert the video to mp4 for easier playback
  // NOTE: we commented this out it uses a lot of CPU
  //digitalWrite(YELLOWLED, HIGH);
//  memset(buffer,'\0',1023);
//  snprintf(buffer, 1023, "/usr/bin/ffmpeg -y -framerate 24 -i /var/www/html/video%d.h264 -c copy /var/www/html/video%d.mp4", vid
//  system(buffer);
  //digitalWrite(YELLOWLED, LOW);
}

int main(int argc, char *argv[]) {

  int sd, rc, n, cliLen;
  struct sockaddr_in cliAddr, servAddr;
  char msg[MAX_MSG];
  int broadcast = 1;

  static struct sigaction _sigact;
  memset(&_sigact, 0, sizeof(_sigact));
  _sigact.sa_sigaction = cleanup;
  _sigact.sa_flags = SA_SIGINFO;

  sigaction(SIGINT, &_sigact, NULL);

  wiringPiSetupPhys();

  pinMode(RLED, OUTPUT);
  pinMode(PWROFF, INPUT);
  //pullUpDnControl(LIGHT_SENSOR, PUD_DOWN);
  pinMode(LIGHT_SENSOR, INPUT);
  pinMode(IR1, OUTPUT);
  pinMode(IR2, OUTPUT);
  pinMode(IR3, OUTPUT);
  pinMode(IR4, OUTPUT);
  pinMode(RLED, OUTPUT);

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

  /* server infinite loop */
  int recordings = 0;

  while (1) {

    /* init buffer */
    memset(msg,0x0,MAX_MSG);

    /* receive message */
    cliLen = sizeof(cliAddr);
    n = recvfrom(sd, msg, MAX_MSG, 0, (struct sockaddr *) &cliAddr, &cliLen);

    if (n<0) {
      printf("%s: cannot receive data \n",argv[0]);
      continue;
    }

    /* print received message */
    printf("%s: from %s:UDP%u : %s \n", argv[0],inet_ntoa(cliAddr.sin_addr), ntohs(cliAddr.sin_port),msg);
    digitalWrite(RLED, HIGH);
    captureRecording(recordings++);
    digitalWrite(RLED, LOW);
    delay(4000);

    if (recordings > 100) {
	    recordings = 0; // wrap around
    }

  }/* end of server infinite loop */

  return 0;

}
