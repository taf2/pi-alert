ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

all: light
upload: light
	pio run -j 2 --target upload && sleep 3;  pio device monitor

light: targets
	pio run

targets: srcfiles/main.cpp
	ruby build.rb main.cpp

monitor:
	pio device monitor

configure:
	pio lib install "Adafruit DotStar"

clean:
	pio run --target clean
