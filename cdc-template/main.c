#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <stdio.h>
#include <string.h>

#include "usb.h"

// initialize the hardware
void init_hardware(void) {
  // disable watchdog if enabled by bootloader/fuses
  MCUSR &= ~(1 << WDRF);
  wdt_disable();

  // disable clock division
  clock_prescale_set(clock_div_1);

  // init lufa usb CDC device
  usb_init();
}

// The entry point for the application code
int main(void) {
  char c;
  init_hardware();

  GlobalInterruptEnable();

  for (;;) {
    c = getc(stdin);
    // check if c is a visible character in the ASCII table
    if (c >= 32 && c <= 126 || c == '\n' || c == '\r' || c == '\t') {
      // echo the character back to the terminal
      putc(c, stdout);
    }
    usb_task();
  }
}
