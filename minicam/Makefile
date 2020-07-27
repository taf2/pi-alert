all: minicam
upload: minicam
	pio run --target upload

minicam: targets
	pio run

targets: srcfiles/main.cpp
	ruby build.rb main.cpp

monitor:
	pio device monitor #--baud 115200 --rts 0 --dts 0

configure:
	pio lib install "Redis for Arduino"

clean:
	pio run --target clean
