all: tester
upload: tester
	pio run -j 2 --target upload --upload-port /dev/cu.usbserial-01D11F38  && sleep 1;  pio device monitor -p /dev/cu.usbserial-01D11F38
	#pio run -j 2 --target upload  && sleep 1;  pio device monitor

tester: src/main.cc
	pio run

monitor:
	pio device monitor -p /dev/cu.usbserial-01D11F38
#	pio device monitor
