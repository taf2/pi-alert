ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
# /dev/cu.usbserial-01D10E63
# /dev/cu.SLAB_USBtoUART
all: detector
upload: detector
#	pio run -j 2 --target upload --upload-port /dev/cu.usbserial-01D10E63  && sleep 1;  pio device monitor -p /dev/cu.usbserial-01D10E63
	pio run -j 2 --target upload  && sleep 1;  pio device monitor

detector: targets
	pio run

targets: srcfiles/main.cpp
	ruby build.rb main.cpp

monitor:
	#pio device monitor -p /dev/cu.usbserial-01D10E63
	pio device monitor

configure:
	pio lib install "Arduino_CRC32"
	pio lib install "adafruit/Adafruit MS8607"
	pio lib install "adafruit/Adafruit BusIO"
	pio lib install "adafruit/Adafruit Unified Sensor"
	pio lib install "adafruit/Adafruit SSD1306"
	pio lib install "adafruit/Adafruit GFX Library"
	pio lib install "arduino-libraries/NTPClient"
	pio lib install "GxEPD2"
	pio lib install "bblanchon/ArduinoJson"
	pio lib install "nkolban/ESP32 BLE Arduino"
	#pio lib install "arduino-libraries/SD"
#	pio lib install "adafruit/Adafruit VS1053 Library"
	#pio lib uninstall SD@0.0.0-alpha+sha.041f788250
#	pio lib uninstall "arduino-libraries/SD"
	#rm -rf .pio/libdeps/featheresp32/SD@0.0.0-alpha+sha.041f788250/


clean:
	pio run --target clean
