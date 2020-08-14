ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

all: relay
upload: relay
	pio run --target upload && sleep 3;  pio device monitor

relay: targets
	pio run

targets: srcfiles/main.cpp
	ruby build.rb main.cpp

monitor:
	pio device monitor

configure:
	curl -L https://github.com/adafruit/WiFiNINA/archive/master.zip > ./WiFiNINA-master.zip
	pio lib install ./WiFiNINA-master.zip

clean:
	pio run --target clean
