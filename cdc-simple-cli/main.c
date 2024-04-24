#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <stdio.h>
#include <string.h>

#include "command.h"
#include "usb.h"

static int usb_puts(const char *s, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (usb_write_byte(s[i]) != 0) {
      return -1;
    }
  }
  return len;
}

// initialize the hardware
static void hardware_init(void) {
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

  hardware_init();

  command_init(&usb_cli, usb_puts);

  GlobalInterruptEnable();

  for (;;) {
    if ((value = usb_read_byte()) != -1) {
      mcucli_putc(&usb_cli, value);
    }
    usb_task();
  }
}
