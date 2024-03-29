all: bootloader simple-cdc-cli

bootloader:
	@make -C bootloader

simple-cdc-cli:
	@make -C simple-cdc-cli

upload-bootloader: bootloader
	@make -C bootloader upload

upload-simple-cdc-cli: simple-cdc-cli
	@make -C simple-cdc-cli upload

clean:
	@make -C bootloader clean
	@make -C simple-cdc-cli clean

.PHONY: all bootloader simple-cdc-cli clean
