APP_VERSION        = 0.0.1
APP_TARGET         = app-v$(APP_VERSION)
BOOTLOADER_VERSION = 0.0.1
BOOTLOADER_TARGET  = bootloader-v$(BOOTLOADER_VERSION)
TARGET             = atmega32u4-$(APP_TARGET)-$(BOOTLOADER_TARGET).bin

MCU          = atmega32u4
ARCH         = AVR8
BOARD        = NONE
F_CPU        = 16000000
F_USB        = $(F_CPU)
OPTIMIZATION = s

export APP_VERSION
export APP_TARGET
export BOOTLOADER_VERSION
export BOOTLOADER_TARGET
export MCU
export ARCH
export BOARD
export F_CPU
export F_USB
export OPTIMIZATION

release: app bootloader
	python pack.py --app app/$(APP_TARGET).bin --bootloader bootloader/$(BOOTLOADER_TARGET).bin --output $(TARGET)

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
