#include <avr/wdt.h>
#include <stdio.h>

#include "command.h"
#include "usb.h"
#include "version.h"

// declare the command functions
static void command_help(mcucli_command_t *command, void *user_data, int argc,
                         char *argv[]);
static void command_version(mcucli_command_t *command, void *user_data,
                            int argc, char *argv[]);
static void command_reboot(mcucli_command_t *command, void *user_data, int argc,
                           char *argv[]);
static void command_bootloader(mcucli_command_t *command, void *user_data,
                               int argc, char *argv[]);

static mcucli_command_t commands[] = {
    {"help", "Print this help message", command_help},
    {"?", "Print this help message", command_help},
    {"version", "Print the firmware version", command_version},
    {"reboot", "Reboot the MCU", command_reboot},
    {"bootloader", "Enter the bootloader", command_bootloader},
};

static size_t num_commands = sizeof(commands) / sizeof(mcucli_command_t);

static void unknown_command(void *user_data, const char *command) {
  UNUSED(user_data);
  printf("Unknown command: %s\r\n", command);
}

static void command_help(mcucli_command_t *command, void *user_data, int argc,
                         char *argv[]) {
  for (size_t i = 0; i < num_commands; i++) {
    printf("- %s\r\n", commands[i].name);
    printf("%s\r\n\r\n", commands[i].help);
  }
}

static void command_version(mcucli_command_t *command, void *user_data,
                            int argc, char *argv[]) {
  printf("Firmware version: %s\r\n", VERSION);
}

static void command_reboot(mcucli_command_t *command, void *user_data, int argc,
                           char *argv[]) {
  // reboot the MCU
  wdt_enable(WDTO_15MS);
  for (;;)
    ;
}

static void command_bootloader(mcucli_command_t *command, void *user_data,
                               int argc, char *argv[]) {
  usb_disable();

  /* Relocate the interrupt vector table */
  MCUCR = (1 << IVCE);
  MCUCR = 0;

  // enter the bootloader
  ((void (*)(void))0x7000)();
}

void command_init(mcucli_t *cli, byte_writer_t writer) {
  mcucli_init(cli, commands, num_commands, writer, unknown_command, NULL);
}
