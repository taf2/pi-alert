ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

all: tunes
upload: tunes
	pio run -j 2 --target upload && sleep 3;  pio device monitor

tunes: targets
	pio run

targets: srcfiles/main.cpp
	ruby build.rb main.cpp

monitor:
	pio device monitor

configure:
	pio lib install "adafruit/Adafruit VS1053 Library"
	mkdir -p src

clean:
	pio run --target clean
