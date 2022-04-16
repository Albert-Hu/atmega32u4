VERSION      = 0.0.1

TARGET       = atmega32u4-v$(VERSION).bin

MCU          = atmega32u4
ARCH         = AVR8
BOARD        = NONE
F_CPU        = 16000000
F_USB        = $(F_CPU)
OPTIMIZATION = s

export MCU
export ARCH
export BOARD
export F_CPU
export F_USB
export OPTIMIZATION

release: app bootloader
	python pack.py --app app/app.bin --bootloader bootloader/bootloader.bin --output $(TARGET)

.PHONY: app
app:
	make -C app

.PHONY: bootloader
bootloader:
	make -C bootloader

clean:
	make -C app clean
	make -C bootloader clean
	rm -f $(TARGET)
