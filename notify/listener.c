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
#include <pthread.h>

//for Mac OS X
#include <stdlib.h>

#include <wiringPi.h>

#define LOCAL_SERVER_PORT 1500
#define MAX_MSG 100

#define IR_LED 22 // GPIO 25 pull high to enable

static char buffer[1024];

void cleanup(int signum, siginfo_t *info, void *ptr) {
  write(STDERR_FILENO, "cleanup\n", sizeof("cleanup\n"));

  digitalWrite(IR_LED, LOW);

  exit(0);
}

void captureRecording(int videoPoint) {
  digitalWrite(IR_LED, HIGH); // enable IR_LED (they have light sensor's that will turn on the LED or not automatically
  // capture 10 seconds of video
  printf("start recording\n");
  memset(buffer,'\0',1023);
  snprintf(buffer, 1023, "/usr/bin/ffmpeg -y -video_size 1024x768 -i /dev/video0 -f alsa -channels 1 -sample_rate 44100 -i hw:1 -vf \"drawtext=text='%{localtime}': x=(w-tw)/2: y=lh: fontcolor=white: fontsize=24\" -af \"volume=15.0\" -c:v h264_omx -b:v 2500k -c:a libmp3lame -b:a 128k -map 0:v -map 1:a -f flv -t 00:00:20 /tmp/out%d.raw", videoPoint);

  //snprintf(buffer, 1023, "/usr/bin/raspivid --rotation 0 -o /var/www/html/video%d.h264 -w 640 -h 480 -t 10000", videoPoint);
  system(buffer);
  printf("done recording\n");
  // convert the video to mp4 for easier playback
  // NOTE: we commented this out it uses a lot of CPU
  //digitalWrite(YELLOWLED, HIGH);
//  memset(buffer,'\0',1023);
//  snprintf(buffer, 1023, "/usr/bin/ffmpeg -y -framerate 24 -i /var/www/html/video%d.h264 -c copy /var/www/html/video%d.mp4", vid
//  system(buffer);
  //digitalWrite(YELLOWLED, LOW);
  digitalWrite(IR_LED, LOW); // cut the power
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
  sigaction(SIGQUIT, &_sigact, NULL);

  wiringPiSetupPhys();

  pinMode(IR_LED, OUTPUT);

  printf("starting up waiting 5 seconds...\n");
  delay(5000);
  printf("it's ready\n");

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
    if (strstr(msg, "motion")) {
      captureRecording(recordings++);
      delay(4000);

      if (recordings > 100) {
        recordings = 0; // wrap around
      }
    }

  }/* end of server infinite loop */

  return 0;

}
