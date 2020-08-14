ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

all: simple
upload: simple
	pio run --target upload && sleep 2;  pio device monitor

simple: targets
	pio run

targets: srcfiles/main.cpp
	ruby build.rb main.cpp

monitor:
	pio device monitor

configure:
	pio lib install "Adafruit DotStar"

clean:
	pio run --target clean
