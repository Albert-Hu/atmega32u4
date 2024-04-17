#!/usr/bin/env bash

SRC_DIR=$(pwd)

if [ $# -eq 1 ]; then
  if [ -d $1 ]; then
    SRC_DIR=$(pwd)/$1
  else
    echo "Error: $1 is not a directory in this folder"
    exit 1
  fi
fi

# run docker container and mount the current directory to /src
docker run -it --rm --device "/dev/bus/usb:/dev/bus/usb" -v $SRC_DIR:/src -w /src atmega32u4 /bin/bash
