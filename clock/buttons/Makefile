all: tester
upload: tester
	pio run -j 2 --target upload  && sleep 1;  pio device monitor

tester: src/main.cc
	pio run

monitor:
	pio device monitor
