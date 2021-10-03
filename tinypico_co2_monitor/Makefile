ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

all: detector
upload: detector
	pio run -j 2 --target upload && sleep 1;  pio device monitor

detector: targets
	pio run

targets: srcfiles/main.cpp
	ruby build.rb main.cpp

monitor:
	pio device monitor

configure:
	pio lib install "TinyPICO Helper Library"
	pio lib install "adafruit/Adafruit SHARP Memory Display"
	pio lib install "sensirion/Sensirion I2C SCD4x @ ^0.3.1"
	pio lib install "adafruit/Adafruit GFX Library"
	pio lib install "adafruit/Adafruit BusIO"
	pio lib install "bblanchon/ArduinoJson"
	pio lib install "arduino-libraries/NTPClient"
#	pio lib install "SoftwareSerialEsp32"
#	pio lib install "Adafruit VC0706 Serial Camera Library"

clean:
	pio run --target clean
