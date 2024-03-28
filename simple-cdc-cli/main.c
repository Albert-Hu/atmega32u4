#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <stdio.h>
#include <string.h>

#include "command.h"
#include "usb.h"

static int usb_putchar(char c) { return (int)usb_write_byte(c); }

// initialize the hardware
static void init_hardware(void) {
  // disable watchdog if enabled by bootloader/fuses
  MCUSR &= ~(1 << WDRF);
  wdt_disable();

  // init lufa usb CDC device
  usb_init();
}

// The entry point for the application code
int main(void) {
  uint16_t value;
  mcucli_t usb_cli;

  init_hardware();

  command_init(&usb_cli, usb_putchar);

  GlobalInterruptEnable();

  for (;;) {
    if ((value = usb_read_byte()) != -1) {
      mcucli_push_char(&usb_cli, value);
      // printf("Input: 0x%02x\r\n", value);
    }
    usb_task();
  }
}
