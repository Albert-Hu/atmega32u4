#include <avr/io.h>
#include <avr/wdt.h>
#include <stdio.h>
#include <stdlib.h>

#include "command.h"
#include "gpio.h"
#include "usb.h"
#include "version.h"

#define LINE_BUFFER_SIZE 128
#define ARGUMENT_BUFFER_SIZE 32

typedef struct {
  const char *str;
  gpio_register_t reg;
} str2reg_t;

typedef struct {
  const char *str;
  gpio_pin_t pin;
} str2pin_t;

// declare the command functions
static void process_help(mcucli_t *cli, void *user_data, int argc,
                         char *argv[]);
static void process_gpio(mcucli_t *cli, void *user_data, int argc,
                         char *argv[]);
static void process_version(mcucli_t *cli, void *user_data, int argc,
                         char *argv[]);
static void process_reboot(mcucli_t *cli, void *user_data, int argc,
                         char *argv[]);
static void process_bootloader(mcucli_t *cli, void *user_data, int argc,
                         char *argv[]);

static mcucli_command_t commands[] = {
    {"help", "Print this help message", process_help},
    {"?", "Print this help message", process_help},
    {"gpio",
     "Utility to control GPIO pins.\r\n"
     "  - Usage of control the GPIO register:\r\n"
     "      gpio help, print this help message\r\n"
     "      gpio ?, print this help message\r\n"
     "      gpio all, print all the registers\r\n"
     "      gpio <register>, print the register value\r\n"
     "      gpio <register> <value>, set the register value\r\n"
     "      gpio <pin>, print the pin value\r\n"
     "      gpio <pin> <mode>, set the pin mode, in for input, out for "
     "output\r\n"
     "      gpio <pin> <value>, set the pin value, 0 for low, 1 for high\r\n"
     "  - Available registers:\r\n"
     "      MCUCR, MCU Control Register\r\n"
     "      DDRB DDRC DDRD DDRE DDRF, Data Direction Registers\r\n"
     "      PORTB PORTC PORTD PORTE PORTF, Port Data Registers\r\n"
     "      PINB PINC PIND PINE PINF, Port Input Pins\r\n"
     "  - Available pins:\r\n"
     "      PB0 PB1 PB2 PB3 PB4 PB5 PB6 PB7, Port B\r\n"
     "      PC6 PC7, Port C\r\n"
     "      PD0 PD1 PD2 PD3 PD4 PD5 PD6 PD7, Port D\r\n"
     "      PE2 PE6, Port E\r\n"
     "      PF0 PF1 PF4 PF5 PF6 PF7, Port F\r\n"
     "",
     process_gpio},
    {"version", "Print the firmware version", process_version},
    {"reboot", "Reboot the MCU", process_reboot},
    {"bootloader", "Enter the bootloader", process_bootloader},
};

static mcucli_command_set_t command_set = {
  .commands = commands,
  .num_commands = sizeof(commands) / sizeof(mcucli_command_t),
};

static char line_buffer[LINE_BUFFER_SIZE];
static char *argument_buffer[ARGUMENT_BUFFER_SIZE];
static mcucli_buffer_t buffer = {line_buffer, LINE_BUFFER_SIZE, argument_buffer, ARGUMENT_BUFFER_SIZE};

static str2reg_t str2reg[] = {
    {"MCUCR", GPIO_MCUCR}, {"DDRB", GPIO_DDRB},   {"DDRC", GPIO_DDRC},
    {"DDRD", GPIO_DDRD},   {"DDRE", GPIO_DDRE},   {"DDRF", GPIO_DDRF},
    {"PORTB", GPIO_PORTB}, {"PORTC", GPIO_PORTC}, {"PORTD", GPIO_PORTD},
    {"PORTE", GPIO_PORTE}, {"PORTF", GPIO_PORTF}, {"PINB", GPIO_PINB},
    {"PINC", GPIO_PINC},   {"PIND", GPIO_PIND},   {"PINE", GPIO_PINE},
    {"PINF", GPIO_PINF},
};

static size_t num_str2reg = sizeof(str2reg) / sizeof(str2reg_t);

static str2pin_t str2pin[] = {
    {"PB0", GPIO_PB0}, {"PB1", GPIO_PB1}, {"PB2", GPIO_PB2}, {"PB3", GPIO_PB3},
    {"PB4", GPIO_PB4}, {"PB5", GPIO_PB5}, {"PB6", GPIO_PB6}, {"PB7", GPIO_PB7},
    {"PC6", GPIO_PC6}, {"PC7", GPIO_PC7}, {"PD0", GPIO_PD0}, {"PD1", GPIO_PD1},
    {"PD2", GPIO_PD2}, {"PD3", GPIO_PD3}, {"PD4", GPIO_PD4}, {"PD5", GPIO_PD5},
    {"PD6", GPIO_PD6}, {"PD7", GPIO_PD7}, {"PE2", GPIO_PE2}, {"PE6", GPIO_PE6},
    {"PF0", GPIO_PF0}, {"PF1", GPIO_PF1}, {"PF4", GPIO_PF4}, {"PF5", GPIO_PF5},
    {"PF6", GPIO_PF6}, {"PF7", GPIO_PF7},
};

static size_t num_str2pin = sizeof(str2pin) / sizeof(str2pin_t);

static void to_uppercase(char *str) {
  for (size_t i = 0; str[i] != '\0'; i++) {
    if (str[i] >= 'a' && str[i] <= 'z') {
      str[i] = str[i] - 'a' + 'A';
    }
  }
}

static void unknown_command(mcucli_t *cli, void *user_data, const char *command) {
  UNUSED(cli);
  UNUSED(user_data);
  printf("Unknown command: %s\r\n", command);
}

static void process_help(mcucli_t *cli, void *user_data, int argc,
                         char *argv[]) {
  for (size_t i = 0; i < command_set.num_commands; i++) {
    printf("- %s\r\n", command_set.commands[i].name);
    printf("%s\r\n\r\n", command_set.commands[i].help);
  }
}

static uint8_t is_mode(const char *name) {
  return (name[0] == 'I' && name[1] == 'N' && name[2] == '\0') ||
         (name[0] == 'O' && name[1] == 'U' && name[2] == 'T' &&
          name[3] == '\0');
}

static uint8_t is_pin(const char *name) {
  uint8_t result = 1;

  result &= name[0] == 'P';
  result &= name[1] == 'B' || name[1] == 'C' || name[1] == 'D' ||
            name[1] == 'E' || name[1] == 'F';
  result &= name[2] >= '0' && name[2] <= '7';
  result &= name[3] == '\0';

  return result;
}

static void print_pin_status(const char *name) {
  for (size_t i = 0; i < num_str2pin; i++) {
    if (strcmp(name, str2pin[i].str) == 0) {
      int16_t direction = gpio_get_direction(str2pin[i].pin);
      int16_t level = gpio_get_level(str2pin[i].pin);
      if (direction < 0 || level < 0) {
        printf("Failed to read pin %s\r\n", name);
      } else {
        printf("%s: %s, %s\r\n", name, direction ? "OUT" : "IN",
               level ? "HIGH" : "LOW");
      }
      break;
    }
  }
}

static void set_pin_mode(const char *name, const char *mode) {
  for (size_t i = 0; i < num_str2pin; i++) {
    if (strcmp(name, str2pin[i].str) == 0) {
      if (mode[0] == 'I' && mode[1] == 'N' && mode[2] == '\0') {
        if (gpio_set_direction(str2pin[i].pin, 0) < 0) {
          printf("Failed to set pin %s to input mode\r\n", name);
        }
      } else {
        if (gpio_set_direction(str2pin[i].pin, 1) < 0) {
          printf("Failed to set pin %s to output mode\r\n", name);
        }
      }
      break;
    }
  }
}

static void set_pin_value(const char *name, const char *value) {
  for (size_t i = 0; i < num_str2pin; i++) {
    if (strcmp(name, str2pin[i].str) == 0) {
      if (gpio_set_level(str2pin[i].pin, (uint8_t)strtol(value, NULL, 0)) < 0) {
        printf("Failed to set pin %s to %s\r\n", name, value);
      }
      break;
    }
  }
}

static void print_register_value(const char *name) {
  if (name[0] == 'A' && name[1] == 'L' && name[2] == 'L' && name[3] == '\0') {
    for (size_t i = 0; i < num_str2reg; i++) {
      print_register_value(str2reg[i].str);
    }
  } else {
    for (size_t i = 0; i < num_str2reg; i++) {
      if (strcmp(name, str2reg[i].str) == 0) {
        int16_t value = gpio_read(str2reg[i].reg);
        if (value < 0) {
          printf("Failed to read register %s\r\n", name);
        } else {
          printf("%s: 0x%02X\r\n", name, ((int)value) & 0xFF);
        }
        break;
      }
    }
  }
}

static void set_register_value(const char *name, uint8_t value) {
  for (size_t i = 0; i < num_str2reg; i++) {
    if (strcmp(name, str2reg[i].str) == 0) {
      if (gpio_write(str2reg[i].reg, value) < 0) {
        printf("Failed to write 0x%02X to register %s\r\n", ((int)value) & 0xFF,
               name);
      }
      break;
    }
  }
}

static void process_gpio(mcucli_t *cli, void *user_data, int argc,
                         char *argv[]) {
  UNUSED(user_data);

  do {
    if (argc == 0 ||
        (argc == 1 && (strcmp(argv[0], "help") == 0 || argv[0][0] == '?'))) {
      printf("%s\r\n", command_set.commands[2].help);
      break;
    }

    to_uppercase(argv[0]);

    if (argc == 1) {
      if (is_pin(argv[0]))
        print_pin_status(argv[0]);
      else
        print_register_value(argv[0]);
    } else if (argc == 2) {
      to_uppercase(argv[1]);
      if (is_pin(argv[0])) {
        if (is_mode(argv[1])) {
          set_pin_mode(argv[0], argv[1]);
        } else {
          set_pin_value(argv[0], argv[1]);
        }
      } else {
        set_register_value(argv[0], (uint8_t)strtol(argv[1], NULL, 0));
      }
    } else {
      printf("Invalid number of arguments\r\n");
    }
  } while (0);
}

static void process_version(mcucli_t *cli, void *user_data, int argc,
                         char *argv[]) {
  printf("Firmware version: %s\r\n", VERSION);
}

static void process_reboot(mcucli_t *cli, void *user_data, int argc,
                         char *argv[]) {
  // reboot the MCU
  wdt_enable(WDTO_15MS);
  for (;;)
    ;
}

static void process_bootloader(mcucli_t *cli, void *user_data, int argc,
                         char *argv[]) {
  usb_disable();

  /* Relocate the interrupt vector table */
  MCUCR = (1 << IVCE);
  MCUCR = 0;

  // enter the bootloader
  ((void (*)(void))0x7000)();
}

void command_init(mcucli_t *cli, bytes_write_t write) {
  mcucli_init(cli, NULL, &buffer, &command_set, write, unknown_command);
}
