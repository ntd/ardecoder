all: build
	arduino-builder -build-path build -fqbn archlinux-arduino:avr:nano:cpu=atmega328 -hardware /usr/share/arduino/hardware -tools /usr/bin ardecoder.ino

build:
	mkdir build

clean:
	rm -fr build

.phony: clean
