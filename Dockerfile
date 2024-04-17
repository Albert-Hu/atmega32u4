FROM ubuntu:20.04
RUN apt update && apt install -y gcc-avr avr-libc make python3 python3-pip avrdude dfu-programmer
