#!/bin/bash

# Upload the firmware through the Arduino Leonardo bootloader
# Note: The bootloader is only active for 8 seconds after the Arduino Leonardo is reset
# Usage: ./arduino-upload.sh /dev/ttyACM0 firmware.hex

# Check if the number of input arguments are two
if [ $# -ne 2 ]; then
  echo "Please provide the port and the firmware file as arguments."
  exit 1
fi

# Check if the port is valid
if [ ! -e $1 ]; then
  echo "The port $1 does not exist."
  exit 1
fi

# Check if the firmware file is existing and ends with .hex
if [ ! -f $2 ] || [ "${2: -4}" != ".hex" ]; then
  echo "The firmware file $2 does not exist or does not end with .hex."
  exit 1
fi

# Run avrdude to upload the firmware
avrdude -v -patmega32u4 -cavr109 -P$1 -b57600 -D -Uflash:w:$2:i
