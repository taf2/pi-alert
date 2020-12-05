ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

all: display
upload: display
	pio run -j 2 --target upload --upload-port /dev/cu.SLAB_USBtoUART && sleep 1;  pio device monitor -p /dev/cu.SLAB_USBtoUART

display: src/main.cc src/settings.cc
	pio run

monitor:
	pio device monitor -p /dev/cu.SLAB_USBtoUART

configure:
	pio lib install "Arduino_CRC32"
	pio lib install "adafruit/Adafruit SHARP Memory Display"
	pio lib install "arduino-libraries/NTPClient"
	pio lib install "bblanchon/ArduinoJson"
	pio lib install "ESP Async WebServer"
