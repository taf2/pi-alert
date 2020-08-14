ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

all: receiver
upload: receiver
	pio run --target upload && sleep 1;  pio device monitor

receiver: targets
	pio run

targets: srcfiles/main.cpp
	ruby build.rb main.cpp

monitor:
	pio device monitor

configure:
	pio lib install "Adafruit DotStar"
	pio lib install "RF24"

clean:
	pio run --target clean
