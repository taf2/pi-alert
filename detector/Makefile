all: detector
upload: detector
	pio run --target upload

detector: targets
	pio run

targets: srcfiles/main.cpp
	ruby build.rb main.cpp

monitor:
	pio device monitor

configure:
	pio lib install "ArduCAM"

clean:
	pio run --target clean
