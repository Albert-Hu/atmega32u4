#!/bin/bash

# Check if firmware file is provided as an argument
if [ $# -eq 0 ]; then
  echo "Please provide the firmware file as an argument."
  exit 1
fi

# Configuration for avrdude
PROGRAMMER="usbasp"
MCU="atmega32u4"

# Firmware file
FIRMWARE="$1"

# Upload firmware using avrdude
avrdude -c $PROGRAMMER -p $MCU -U flash:w:$FIRMWARE:r
