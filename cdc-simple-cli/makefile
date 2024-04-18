MCU          = atmega32u4
ARCH         = AVR8
BOARD        = NONE
F_CPU        = 16000000
F_USB        = $(F_CPU)
OPTIMIZATION = s
TARGET       = main
MCUCLI_SRC   = $(wildcard ../libs/mcucli/src/*.c)
SRC          = $(wildcard *.c) $(MCUCLI_SRC) $(LUFA_SRC_USB) $(LUFA_SRC_USBCLASS)
LUFA_PATH    = ../libs/lufa/LUFA
CC_FLAGS     = -DUSE_LUFA_CONFIG_HEADER -Iconfig/ -I../libs/mcucli/include
LD_FLAGS     =
OBJDIR       = build/obj

build: all
	@mv $(filter-out $(TARGET).c,$(shell ls $(TARGET)*)) build

upload: build
	@avrdude -c usbasp -p atmega32u4 -U flash:w:build/$(TARGET).hex:i

DMBS_LUFA_PATH ?= $(LUFA_PATH)/Build/LUFA
include $(DMBS_LUFA_PATH)/lufa-sources.mk
include $(DMBS_LUFA_PATH)/lufa-gcc.mk

DMBS_PATH      ?= $(LUFA_PATH)/Build/DMBS/DMBS
include $(DMBS_PATH)/core.mk
include $(DMBS_PATH)/cppcheck.mk
include $(DMBS_PATH)/doxygen.mk
include $(DMBS_PATH)/dfu.mk
include $(DMBS_PATH)/gcc.mk
include $(DMBS_PATH)/hid.mk
include $(DMBS_PATH)/avrdude.mk
include $(DMBS_PATH)/atprogram.mk

clean:
	@rm -rf build
