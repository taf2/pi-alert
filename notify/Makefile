all: notify listen listener pi-alive monitor

notify: notify.c
	gcc -Wall notify.c -o notify
	#ar rcs $@ $^

listen: listen.c
	gcc -Wall listen.c -o listen

listener: listener.c
	gcc -Wall listener.c -o listener -lwiringPi

pi-alive: pi-alive.c
	gcc -Wall pi-alive.c -o pi-alive -lwiringPi

monitor: monitor.c
	gcc -Wall monitor.c -o monitor -lwiringPi

install:
	sudo cp pi-alive.service /etc/systemd/system/

clean:
	rm -f notify listener
