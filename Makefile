all: monitor led pirsensor
monitor: monitor.c
	gcc -Wall monitor.c -o monitor -lwiringPi -lhiredis

led: led.c
	gcc -Wall led.c -o led -lwiringPi

pirsensor: pirsensor.c
	gcc -Wall pirsensor.c -o pirsensor -lwiringPi

# compile arduino projects
trinket-m0-configure:
	arduino-cli core install adafruit:samd

trinket-v3-configure:
	
day_light_monitor: trinketm0_on_when_light/trinketm0_on_when_light.ino
	rm -rf trinketm0_on_when_light/build/ && cd trinketm0_on_when_light && arduino-cli compile --additional-urls "https://adafruit.github.io/arduino-board-index/package_adafruit_index.json" -b adafruit:samd:adafruit_trinket_m0 -v trinketm0_on_when_light  ; cd ..

day_light_monitor_upload: day_light_monitor
	cd trinketm0_on_when_light && arduino-cli upload  --additional-urls "https://adafruit.github.io/arduino-board-index/package_adafruit_index.json" -v -b adafruit:samd:adafruit_trinket_m0 -p ${PORT} trinketm0_on_when_light ; cd ..

	

clean:
	rm -f monitor
	rm -f led
	rm -f pirsensor
