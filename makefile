all: bootloader cdc-simple-cli cdc-gpio-cli

bootloader:
	@make -C bootloader

cdc-simple-cli:
	@make -C cdc-simple-cli

cdc-gpio-cli:
	@make -C cdc-gpio-cli

upload-bootloader: bootloader
	@make -C bootloader upload

upload-cdc-simple-cli: cdc-simple-cli
	@make -C cdc-simple-cli upload

clean:
	@make -C bootloader clean
	@make -C cdc-simple-cli clean
	@make -C cdc-gpio-cli clean

.PHONY: all bootloader cdc-simple-cli cdc-gpio-cli clean
