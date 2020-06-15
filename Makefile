all: monitor led pirsensor
monitor: monitor.c
	gcc -Wall monitor.c -o monitor -lwiringPi -lhiredis

led: led.c
	gcc -Wall led.c -o led -lwiringPi

pirsensor: pirsensor.c
	gcc -Wall pirsensor.c -o pirsensor -lwiringPi

clean:
	rm -f monitor
	rm -f led
	rm -f pirsensor
