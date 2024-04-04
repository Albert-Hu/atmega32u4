#include <avr/io.h>
#include <stddef.h>

#include "gpio.h"

typedef struct {
  gpio_pin_t pin;
  gpio_register_t direction;
  gpio_register_t level;
  uint8_t bit;
} gpio_pin_map_t;

static gpio_pin_map_t gpio_pin_map[] = {
    {GPIO_PB0, GPIO_DDRB, GPIO_PORTB, PB0},
    {GPIO_PB1, GPIO_DDRB, GPIO_PORTB, PB1},
    {GPIO_PB2, GPIO_DDRB, GPIO_PORTB, PB2},
    {GPIO_PB3, GPIO_DDRB, GPIO_PORTB, PB3},
    {GPIO_PB4, GPIO_DDRB, GPIO_PORTB, PB4},
    {GPIO_PB5, GPIO_DDRB, GPIO_PORTB, PB5},
    {GPIO_PB6, GPIO_DDRB, GPIO_PORTB, PB6},
    {GPIO_PB7, GPIO_DDRB, GPIO_PORTB, PB7},
    {GPIO_PC6, GPIO_DDRC, GPIO_PORTC, PC6},
    {GPIO_PC7, GPIO_DDRC, GPIO_PORTC, PC7},
    {GPIO_PD0, GPIO_DDRD, GPIO_PORTD, PD0},
    {GPIO_PD1, GPIO_DDRD, GPIO_PORTD, PD1},
    {GPIO_PD2, GPIO_DDRD, GPIO_PORTD, PD2},
    {GPIO_PD3, GPIO_DDRD, GPIO_PORTD, PD3},
    {GPIO_PD4, GPIO_DDRD, GPIO_PORTD, PD4},
    {GPIO_PD5, GPIO_DDRD, GPIO_PORTD, PD5},
    {GPIO_PD6, GPIO_DDRD, GPIO_PORTD, PD6},
    {GPIO_PD7, GPIO_DDRD, GPIO_PORTD, PD7},
    {GPIO_PE2, GPIO_DDRE, GPIO_PORTE, PE2},
    {GPIO_PE6, GPIO_DDRE, GPIO_PORTE, PE6},
    {GPIO_PF0, GPIO_DDRF, GPIO_PORTF, PF0},
    {GPIO_PF1, GPIO_DDRF, GPIO_PORTF, PF1},
    {GPIO_PF4, GPIO_DDRF, GPIO_PORTF, PF4},
    {GPIO_PF5, GPIO_DDRF, GPIO_PORTF, PF5},
    {GPIO_PF6, GPIO_DDRF, GPIO_PORTF, PF6},
    {GPIO_PF7, GPIO_DDRF, GPIO_PORTF, PF7},
};

static size_t num_gpio_pins = sizeof(gpio_pin_map) / sizeof(gpio_pin_map_t);

int16_t gpio_init(void) {
  gpio_write(GPIO_MCUCR, 0x00);
  gpio_write(GPIO_DDRB, 0x00);
  gpio_write(GPIO_DDRC, 0x00);
  gpio_write(GPIO_DDRD, 0x00);
  gpio_write(GPIO_DDRE, 0x00);
  gpio_write(GPIO_DDRF, 0x00);
  gpio_write(GPIO_PORTB, 0x00);
  gpio_write(GPIO_PORTC, 0x00);
  gpio_write(GPIO_PORTD, 0x00);
  gpio_write(GPIO_PORTE, 0x00);
  gpio_write(GPIO_PORTF, 0x00);
  return GPIO_ERROR_NONE;
}

int16_t gpio_set_direction(gpio_pin_t pin, uint8_t value) {
  uint8_t bit = 0;
  int16_t original = 0;
  gpio_register_t reg = GPIO_UNKNOWN_REG;

  for (int i = 0; i < num_gpio_pins; i++) {
    if (gpio_pin_map[i].pin == pin) {
      bit = gpio_pin_map[i].bit;
      reg = gpio_pin_map[i].direction;
      break;
    }
  }

  if (reg == GPIO_UNKNOWN_REG) {
    return GPIO_ERROR_INVALID_PIN;
  }

  original = gpio_read(reg);

  if (original < 0) {
    return original;
  }

  return gpio_write(reg, BIT_RESET(original, bit, value));
}

int16_t gpio_get_direction(gpio_pin_t pin) {
  uint8_t bit = 0;
  uint8_t directions = 0;
  gpio_register_t reg = GPIO_UNKNOWN_REG;

  for (int i = 0; i < num_gpio_pins; i++) {
    if (gpio_pin_map[i].pin == pin) {
      bit = gpio_pin_map[i].bit;
      reg = gpio_pin_map[i].direction;
      break;
    }
  }

  if (reg == GPIO_UNKNOWN_REG) {
    return GPIO_ERROR_INVALID_PIN;
  }

  directions = gpio_read(reg);

  return (directions < 0) ? directions : (directions >> bit) & 1;
}

int16_t gpio_set_level(gpio_pin_t pin, uint8_t value) {
  uint8_t bit = 0;
  int16_t original = 0;
  gpio_register_t reg = GPIO_UNKNOWN_REG;

  for (int i = 0; i < num_gpio_pins; i++) {
    if (gpio_pin_map[i].pin == pin) {
      bit = gpio_pin_map[i].bit;
      reg = gpio_pin_map[i].level;
      break;
    }
  }

  if (reg == GPIO_UNKNOWN_REG) {
    return GPIO_ERROR_INVALID_PIN;
  }

  original = gpio_read(reg);

  if (original < 0) {
    return original;
  }

  return gpio_write(reg, BIT_RESET(original, bit, value));
}

int16_t gpio_get_level(gpio_pin_t pin) {
  uint8_t bit = 0;
  uint16_t result = 0;
  gpio_register_t reg = GPIO_UNKNOWN_REG;

  for (int i = 0; i < num_gpio_pins; i++) {
    if (gpio_pin_map[i].pin == pin) {
      bit = gpio_pin_map[i].bit;
      reg = gpio_pin_map[i].level;
      break;
    }
  }

  if (reg == GPIO_UNKNOWN_REG) {
    return GPIO_ERROR_INVALID_PIN;
  }

  result = gpio_read(reg);

  return (result < 0) ? result : (result >> bit) & 1;
}

int16_t gpio_write(gpio_register_t reg, uint8_t value) {
  switch (reg) {
  case GPIO_MCUCR:
    MCUCR = value & _BV(PUD);
    break;
  case GPIO_DDRB:
    DDRB = value;
    break;
  case GPIO_DDRC:
    DDRC = value & (_BV(PC6) | _BV(PC7));
    break;
  case GPIO_DDRD:
    DDRD = value;
    break;
  case GPIO_DDRE:
    DDRE = value & (_BV(PE2) | _BV(PE6));
    break;
  case GPIO_DDRF:
    DDRF = value &
           (_BV(PF0) | _BV(PF1) | _BV(PF4) | _BV(PF5) | _BV(PF6) | _BV(PF7));
    break;
  case GPIO_PORTB:
    PORTB = value;
    break;
  case GPIO_PORTC:
    PORTC = value & (_BV(PC6) | _BV(PC7));
    break;
  case GPIO_PORTD:
    PORTD = value;
    break;
  case GPIO_PORTE:
    PORTE = value & (_BV(PE2) | _BV(PE6));
    break;
  case GPIO_PORTF:
    PORTF = value &
            (_BV(PF0) | _BV(PF1) | _BV(PF4) | _BV(PF5) | _BV(PF6) | _BV(PF7));
    break;
  default:
    return GPIO_ERROR_INVALID_REG;
  }
  return GPIO_ERROR_NONE;
}

int16_t gpio_read(gpio_register_t reg) {
  switch (reg) {
  case GPIO_MCUCR:
    return MCUCR;
  case GPIO_DDRB:
    return DDRB;
  case GPIO_DDRC:
    return DDRC;
  case GPIO_DDRD:
    return DDRD;
  case GPIO_DDRE:
    return DDRE;
  case GPIO_DDRF:
    return DDRF;
  case GPIO_PORTB:
    return PORTB;
  case GPIO_PORTC:
    return PORTC;
  case GPIO_PORTD:
    return PORTD;
  case GPIO_PORTE:
    return PORTE;
  case GPIO_PORTF:
    return PORTF;
  case GPIO_PINB:
    return PINB;
  case GPIO_PINC:
    return PINC;
  case GPIO_PIND:
    return PIND;
  case GPIO_PINE:
    return PINE;
  case GPIO_PINF:
    return PINF;
  default:
    return GPIO_ERROR_INVALID_REG;
  }
  return GPIO_ERROR_INVALID_REG;
}
