MCU = atmega32u4
CFLAGS = -mmcu=$(MCU) -Wall
BUILD = build

build:
	mkdir -p $(BUILD)
	avr-gcc $(CFLAGS) -o $(BUILD)/main.elf main.c
	avr-objcopy -O binary $(BUILD)/main.elf $(BUILD)/main.bin
	avr-size --mcu=$(MCU) --format=avr $(BUILD)/main.elf

upload: build
	@avrdude -c usbasp -p atmega32u4 -U flash:w:$(BUILD)/main.bin:r

clean:
	rm -rf $(BUILD)

.PHONY: build upload clean