/*
  This program requires wiringpi and handles communication with the tinypico esp32.
  It must be running otherwise the tinypico will reset the pi every 5 minutes.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <wiringPi.h>

#define PI_ON 8
#define PI_HERE 10
#define RES_PIN 12 // switch PI_HERE to HIGH when monitor goes HIGH
#define PWROFF 37 // pwr GPIO 26, wiring 25

int interval = 0;
int pingInterval = 0;
int stopPI = 0;

int ping(const char *ip);

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
    if (stopPI > 2) { 
      printf("start shutdown sequence\n");
      system("/usr/bin/sudo /usr/sbin/halt");
    }
    stopPI++;
  } else {
    stopPI = 0;
  }

  ++pingInterval;

  if (pingInterval % 10 == 0) {

    //delay(1000);
    printf("ping interval\n");
    //int status = ping("192.168.2.1"); //system("ping -c 1 192.168.2.1 > /dev/null 2>&1");
    int status = system("ping -c 1 192.168.2.1 > /dev/null 2>&1");
    if (status == 0) {
      // success
      digitalWrite(PI_HERE, HIGH);
    } else {
      // failure
      digitalWrite(PI_HERE, LOW);
    }

    pingInterval = 0;

  } else {
    digitalWrite(PI_HERE, HIGH);
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


	while (1) {
    delay(1000);
    loop();
	}

	return 0;
}

/* see: https://github.com/davidgatti/How-to-Deconstruct-Ping-with-C-and-NodeJS/blob/master/C/pingWithBareMinimum.c */
int ping(const char *ip) {
  //
  // 1. Creating Socket
  //
  int s = socket(PF_INET, SOCK_RAW, 1);

  //
  //  -> Exit the app if the socket failed to be created
  //
  if (s <= 0) {
    perror("Socket Error");
    return -1;
  }

  //
  // 2. Create the ICMP Struct Header
  //
  typedef struct {
    uint8_t type;
    uint8_t code;
    uint16_t chksum;
    uint32_t data;
  } icmp_hdr_t;

  //
  //  3. Use the newly created struct to make a variable.
  //
  icmp_hdr_t pckt;

  //
  //  4. Set the appropriate values to our struct, which is our ICMP header
  //
  pckt.type = 8;          // The echo request is 8
  pckt.code = 0;          // No need
  pckt.chksum = 0xfff7;   // Fixed checksum since the data is not changing
  pckt.data = 0;          // We don't send anything.

  //
  //  5. Creating a IP Header from a struct that exists in another library
  //
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = 0;
  addr.sin_addr.s_addr = inet_addr(ip);

  //
  //  6. Send our PING
  //
  int actionSendResult = sendto(s, &pckt, sizeof(pckt), 0, (struct sockaddr*)&addr, sizeof(addr));

  //
  //  -> Exit the app if the option failed to be set
  //
  if (actionSendResult < 0) {
    perror("Ping Error");
    return 1;
  }

  //
  // 7. Prepare all the necessary variable to handle the response
  //
  unsigned int resAddressSize;
  unsigned char res[30] = "";
  struct sockaddr resAddress;

  //
  //  8. Read the response from the remote host
  //
  int ressponse = recvfrom(s, res, sizeof(res), 0, &resAddress, &resAddressSize);

  //
  //  -> Display the response in its raw form (hex)
  //
  if (ressponse > 0) {
    printf("Message is %d bytes long, and looks like this:\n", ressponse);

    for (int i = 0; i < ressponse; ++i) {
      printf("%x ", res[i]);
    }

    printf("\n");

    close(s);
    return 0;
  } else {
    close(s);
    perror("Response Error");
    return 2;
  }
}
