ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

all: detector
upload: detector
	pio run --target upload && sleep 3;  pio device monitor

detector: targets
	pio run

targets: srcfiles/main.cpp
	ruby build.rb main.cpp

monitor:
	pio device monitor

configure:
	pio lib install "TinyPICO Helper Library"

clean:
	pio run --target clean
