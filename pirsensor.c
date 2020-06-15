/********************************************************************************************
*Filename      : pirsensor.c
*Description   : alarm system
*Author        : Alan
*Website       : www.osoyoo.com
*Update        : 2017/07/04
********************************************************************************************/
#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>

#define BuzzerPin	   1
#define PIRPin       32

int main(void)
{
	// When initialize wiring failed, print messageto screen
	if(wiringPiSetup() == -1){
		printf("setup wiringPi failed !");
		exit(1);
	}
	
	pinMode(BuzzerPin, OUTPUT);
	pinMode(PIRPin,INPUT);

	printf("\n");
	printf("========================================\n");
	printf("|              Alarm                   |\n");
	printf("|    ------------------------------    |\n");
	printf("|        PIR connect to GPIO0          |\n");
	printf("|                                      |\n");
	printf("|        Buzzer connect to GPIO1       |\n");
	printf("|                                      |\n");
	printf("|                                OSOYOO|\n");
	printf("========================================\n");
	printf("\n");
	
	while(1){
		if(!(digitalRead(PIRPin))){
		digitalWrite(BuzzerPin, HIGH);
		printf("\n");
		printf("-------------------|\n");
		printf("|    no alarm...   |\n");
		printf("-------------------|\n");
		delay(1000);
		}
		else{
		digitalWrite(BuzzerPin, LOW);
		delay(500);
		printf("\n");
		printf("===================|\n");
		printf("|      alarm...    |\n");
		printf("===================|\n");
		}
	}

	return 0;
}

