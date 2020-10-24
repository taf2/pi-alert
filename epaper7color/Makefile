ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

all: epaper
upload: epaper
	pio run -j 2 --target upload && sleep 3;  pio device monitor

epaper: targets
	pio run

targets: srcfiles/main.cpp
	ruby build.rb main.cpp

monitor:
	pio device monitor

configure:
	pio lib install "GxEPD2"
	pio lib install "adafruit/Adafruit BusIO"
	pio lib install "adafruit/Adafruit GFX Library"

clean:
	pio run --target clean
