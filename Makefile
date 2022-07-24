# For Arduino NANO use:
#
#     make upload
#
# For old Arduino NANO (prior than 2018) use:
#
#     make MODEL=nano:cpu=atmega328old upload
#
# For Arduino UNO use:
#
#     make MODEL=uno upload
#
MODEL=nano
DEVICE=/dev/ttyUSB0
BUILDER=arduino-cli compile --fqbn arduino:avr:$(MODEL) --build-path build "$$1"
UPLOADER=arduino-cli upload --fqbn arduino:avr:$(MODEL) -p $(DEVICE) -vti "$$1"

all: build/ardecoder.ino.hex

build/ardecoder.ino.hex: ardecoder.ino
	mkdir -p build ; \
	$(subst $$1,$<,$(BUILDER))

upload: build/ardecoder.ino.hex
	$(subst $$1,$<,$(UPLOADER))

clean:
	rm -fr build

.phony: clean
