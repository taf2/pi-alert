all: monitor
monitor: monitor.c
	gcc -Wall monitor.c -o monitor -lwiringPi -lhiredis
clean:
	rm -f monitor
