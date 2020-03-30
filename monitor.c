#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>

#define PWROFF 37

int main(int argc, char**argv) {
	int pwroff = 0;   
        wiringPiSetupPhys();
	//wiringPiSetup();

	pinMode(PWROFF, INPUT);
	while (1) {
		pwroff = digitalRead(PWROFF); 
		printf("pwroff reading: %d\n",  pwroff);
		if (pwroff) {
			printf("we should start the shutdown sequence\n");
			system("/sbin/shutdown -h now");
		}
		delay(1000);
	}
	return 0;
}
