#include <avr/io.h>
#include <avr/wdt.h>
#include <stdio.h>
#include <stdlib.h>

#include "command.h"
#include "gpio.h"
#include "usb.h"
#include "version.h"

typedef struct {
  char *name;
  uint8_t type;
  uint8_t reg;
} gpio_register_t;

typedef struct {
  char *name;
  uint8_t reg;
  uint8_t bit;
} gpio_pin_t;

// declare the command functions
static void process_help(mcucli_command_t *command, void *user_data, int argc,
                         char *argv[]);
static void process_gpio(mcucli_command_t *command, void *user_data, int argc,
                         char *argv[]);
static void process_version(mcucli_command_t *command, void *user_data,
                            int argc, char *argv[]);
static void process_reboot(mcucli_command_t *command, void *user_data, int argc,
                           char *argv[]);
static void process_bootloader(mcucli_command_t *command, void *user_data,
                               int argc, char *argv[]);

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

static size_t num_commands = sizeof(commands) / sizeof(mcucli_command_t);

static gpio_register_t gpio_registers[] = {
    {"MCUCR", GPIO_TYPE_MCUCR, 0},
    {"DDRB", GPIO_TYPE_DDR, GPIO_REGISTER_B},
    {"DDRC", GPIO_TYPE_DDR, GPIO_REGISTER_C},
    {"DDRD", GPIO_TYPE_DDR, GPIO_REGISTER_D},
    {"DDRE", GPIO_TYPE_DDR, GPIO_REGISTER_E},
    {"DDRF", GPIO_TYPE_DDR, GPIO_REGISTER_F},
    {"PORTB", GPIO_TYPE_PORT, GPIO_REGISTER_B},
    {"PORTC", GPIO_TYPE_PORT, GPIO_REGISTER_C},
    {"PORTD", GPIO_TYPE_PORT, GPIO_REGISTER_D},
    {"PORTE", GPIO_TYPE_PORT, GPIO_REGISTER_E},
    {"PORTF", GPIO_TYPE_PORT, GPIO_REGISTER_F},
    {"PINB", GPIO_TYPE_PIN, GPIO_REGISTER_B},
    {"PINC", GPIO_TYPE_PIN, GPIO_REGISTER_C},
    {"PIND", GPIO_TYPE_PIN, GPIO_REGISTER_D},
    {"PINE", GPIO_TYPE_PIN, GPIO_REGISTER_E},
    {"PINF", GPIO_TYPE_PIN, GPIO_REGISTER_F},
};

static size_t num_gpio_registers =
    sizeof(gpio_registers) / sizeof(gpio_register_t);

static gpio_pin_t gpio_pins[] = {
    {"PB0", GPIO_REGISTER_B, PB0}, {"PB1", GPIO_REGISTER_B, PB1},
    {"PB2", GPIO_REGISTER_B, PB2}, {"PB3", GPIO_REGISTER_B, PB3},
    {"PB4", GPIO_REGISTER_B, PB4}, {"PB5", GPIO_REGISTER_B, PB5},
    {"PB6", GPIO_REGISTER_B, PB6}, {"PB7", GPIO_REGISTER_B, PB7},
    {"PC6", GPIO_REGISTER_C, PC6}, {"PC7", GPIO_REGISTER_C, PC7},
    {"PD0", GPIO_REGISTER_D, PD0}, {"PD1", GPIO_REGISTER_D, PD1},
    {"PD2", GPIO_REGISTER_D, PD2}, {"PD3", GPIO_REGISTER_D, PD3},
    {"PD4", GPIO_REGISTER_D, PD4}, {"PD5", GPIO_REGISTER_D, PD5},
    {"PD6", GPIO_REGISTER_D, PD6}, {"PD7", GPIO_REGISTER_D, PD7},
    {"PE2", GPIO_REGISTER_E, PE2}, {"PE6", GPIO_REGISTER_E, PE6},
    {"PF0", GPIO_REGISTER_F, PF0}, {"PF1", GPIO_REGISTER_F, PF1},
    {"PF4", GPIO_REGISTER_F, PF4}, {"PF5", GPIO_REGISTER_F, PF5},
    {"PF6", GPIO_REGISTER_F, PF6}, {"PF7", GPIO_REGISTER_F, PF7},
};

static size_t num_gpio_pins = sizeof(gpio_pins) / sizeof(gpio_pin_t);

static void to_uppercase(char *str) {
  for (size_t i = 0; str[i] != '\0'; i++) {
    if (str[i] >= 'a' && str[i] <= 'z') {
      str[i] = str[i] - 'a' + 'A';
    }
  }
}

static void unknown_command(void *user_data, const char *command) {
  UNUSED(user_data);
  printf("Unknown command: %s\r\n", command);
}

static void process_help(mcucli_command_t *command, void *user_data, int argc,
                         char *argv[]) {
  for (size_t i = 0; i < num_commands; i++) {
    printf("- %s\r\n", commands[i].name);
    printf("%s\r\n\r\n", commands[i].help);
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
  size_t pin = 0;

  while (pin < num_gpio_pins && strcmp(name, gpio_pins[pin].name) != 0) {
    pin++;
  }

  if (pin < num_gpio_pins) {
    int16_t level = gpio_get_level(gpio_pins[pin].reg, gpio_pins[pin].bit);
    int16_t direction =
        gpio_get_direction(gpio_pins[pin].reg, gpio_pins[pin].bit);
    if (level >= 0 && direction >= 0) {
      printf("%s: %s, %s\r\n", gpio_pins[pin].name,
             direction == GPIO_DIRECTION_IN ? "IN" : "OUT",
             level ? "HIGH" : "LOW");
    } else {
      printf("Failed to read pin: %s\r\n", name);
    }
  } else {
    printf("Unknown pin: %s\r\n", name);
  }
}

static void set_pin_mode(const char *name, const char *mode) {
  size_t pin = 0;

  while (pin < num_gpio_pins && strcmp(name, gpio_pins[pin].name) != 0) {
    pin++;
  }

  if (pin < num_gpio_pins) {
    if (mode[0] == 'I' && mode[1] == 'N' && mode[2] == '\0') {
      if (gpio_set_direction(gpio_pins[pin].reg, gpio_pins[pin].bit,
                             GPIO_DIRECTION_IN) < 0) {
        printf("Failed to set pin mode: %s\r\n", name);
      }
    } else if (mode[0] == 'O' && mode[1] == 'U' && mode[2] == 'T' &&
               mode[3] == '\0') {
      if (gpio_set_direction(gpio_pins[pin].reg, gpio_pins[pin].bit,
                             GPIO_DIRECTION_OUT) < 0) {
        printf("Failed to set pin mode: %s\r\n", name);
      }
    } else {
      printf("Invalid mode: %s\r\n", mode);
    }
  } else {
    printf("Unknown pin: %s\r\n", name);
  }
}

static void set_pin_value(const char *name, const char *value) {
  size_t pin = 0;

  while (pin < num_gpio_pins && strcmp(name, gpio_pins[pin].name) != 0) {
    pin++;
  }

  if (pin < num_gpio_pins) {
    if (value[0] == '0' && value[1] == '\0') {
      if (gpio_set_level(gpio_pins[pin].reg, gpio_pins[pin].bit, 0) < 0) {
        printf("Failed to set pin value: %s\r\n", name);
      }
    } else if (value[0] == '1' && value[1] == '\0') {
      if (gpio_set_level(gpio_pins[pin].reg, gpio_pins[pin].bit, 1) < 0) {
        printf("Failed to set pin value: %s\r\n", name);
      }
    } else {
      printf("Invalid value: %s\r\n", value);
    }
  } else {
    printf("Unknown pin: %s\r\n", name);
  }
}

static void print_register_value(const char *name) {
  if (name[0] == 'A' && name[1] == 'L' && name[2] == 'L') {
    for (size_t i = 0; i < num_gpio_registers; i++) {
      print_register_value(gpio_registers[i].name);
    }
  } else {
    size_t reg = 0;

    while (reg < num_gpio_registers &&
           strcmp(name, gpio_registers[reg].name) != 0) {
      reg++;
    }

    if (reg < num_gpio_registers) {
      int16_t value =
          gpio_read(gpio_registers[reg].reg, gpio_registers[reg].type);
      if (value < 0) {
        printf("Failed to read register: %s\r\n", name);
      } else {
        printf("%s: 0x%02X\r\n", gpio_registers[reg].name, ((int)value) & 0xFF);
      }
    } else {
      printf("Unknown register: %s\r\n", name);
    }
  }
}

static void set_register_value(const char *name, uint8_t value) {
  size_t reg = 0;

  while (reg < num_gpio_registers &&
         strcmp(name, gpio_registers[reg].name) != 0) {
    reg++;
  }

  if (reg < num_gpio_registers) {
    if (gpio_write(gpio_registers[reg].reg, gpio_registers[reg].type, value) <
        0) {
      printf("Failed to set register value: %s\r\n", name);
    }
  } else {
    printf("Unknown register: %s\r\n", name);
  }
}

static void process_gpio(mcucli_command_t *command, void *user_data, int argc,
                         char *argv[]) {
  UNUSED(user_data);

  do {
    if (argc == 0 ||
        (argc == 1 && (strcmp(argv[0], "help") == 0 || argv[0][0] == '?'))) {
      printf("%s\r\n", command->help);
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

static void process_version(mcucli_command_t *command, void *user_data,
                            int argc, char *argv[]) {
  printf("Firmware version: %s\r\n", VERSION);
}

static void process_reboot(mcucli_command_t *command, void *user_data, int argc,
                           char *argv[]) {
  // reboot the MCU
  wdt_enable(WDTO_15MS);
  for (;;)
    ;
}

static void process_bootloader(mcucli_command_t *command, void *user_data,
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
