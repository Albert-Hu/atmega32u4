#!/bin/bash

# Check if firmware file is provided as an argument
if [ $# -eq 0 ]; then
  echo "Please provide the firmware file as an argument."
  exit 1
fi

# Upload firmware using avrdude
avrdude -c usbasp -p atmega32u4 -U flash:w:$1:r
