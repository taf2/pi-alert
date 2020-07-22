all: minicam
upload: minicam
	pio run --target upload

minicam: src/main.cpp
	pio run

monitor:
	pio device monitor

clean:
	pio run --target clean
