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
	pio lib install "adafruit/Adafruit MS8607"
	pio lib install "adafruit/Adafruit BusIO"
	pio lib install "adafruit/Adafruit Unified Sensor"
	pio lib install "adafruit/Adafruit SSD1306"
	pio lib install "adafruit/Adafruit GFX Library"
	pio lib install "arduino-libraries/NTPClient"
	pio lib install "GxEPD2"
	pio lib install "bblanchon/ArduinoJson"
	pio lib install "nkolban/ESP32 BLE Arduino"


clean:
	pio run --target clean
