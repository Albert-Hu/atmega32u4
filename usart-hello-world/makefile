MCU = atmega32u4
CC = avr-gcc
OBJCOPY = avr-objcopy
CFLAGS = -mmcu=$(MCU) -Wall
BUILD = build

build:
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -o $(BUILD)/main.elf main.c
	$(OBJCOPY) -O binary $(BUILD)/main.elf $(BUILD)/main.bin

upload: build
	@avrdude -c usbasp -p atmega32u4 -U flash:w:$(BUILD)/main.bin:r

clean:
	rm -rf $(BUILD)

.PHONY: build upload clean