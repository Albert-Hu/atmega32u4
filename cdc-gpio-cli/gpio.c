#include <avr/io.h>

#include "gpio.h"

int16_t gpio_pullup_disable(uint8_t disable) {
  uint8_t mcucr = MCUCR;
  BIT_RESET(mcucr, PUD, disable);
  MCUCR = mcucr;
  return GPIO_ERROR_NONE;
}

uint8_t gpio_pullup_disabled(void) { return (MCUCR >> PUD) & 1; }

int16_t gpio_set_directions(uint8_t reg, uint8_t directions) {
  return gpio_write(reg, GPIO_TYPE_DDR, directions);
}

int16_t gpio_get_directions(uint8_t reg) {
  return gpio_read(reg, GPIO_TYPE_DDR);
}

int16_t gpio_set_levels(uint8_t reg, uint8_t levels) {
  return gpio_write(reg, GPIO_TYPE_PORT, levels);
}

int16_t gpio_get_levels(uint8_t reg) {
  uint8_t ddr, port, pin;

  switch (reg) {
  case GPIO_REGISTER_B:
  case GPIO_REGISTER_C:
  case GPIO_REGISTER_D:
  case GPIO_REGISTER_E:
  case GPIO_REGISTER_F:
    ddr = gpio_read(reg, GPIO_TYPE_DDR);
    port = gpio_read(reg, GPIO_TYPE_PORT);
    pin = gpio_read(reg, GPIO_TYPE_PIN);
    break;
  default:
    return GPIO_ERROR_INVALID_REG;
  }

  return (ddr & port) | (~ddr & pin);
}

int16_t gpio_set_direction(uint8_t reg, uint8_t bit, uint8_t direction) {
  uint8_t directions;

  switch (reg) {
  case GPIO_REGISTER_B:
    directions = DDRB;
    break;
  case GPIO_REGISTER_C:
    directions = DDRC;
    break;
  case GPIO_REGISTER_D:
    directions = DDRD;
    break;
  case GPIO_REGISTER_E:
    directions = DDRE;
    break;
  case GPIO_REGISTER_F:
    directions = DDRF;
    break;
  default:
    return GPIO_ERROR_INVALID_REG;
  }

  BIT_RESET(directions, bit, direction);

  return gpio_write(reg, GPIO_TYPE_DDR, directions);
}

int16_t gpio_get_direction(uint8_t reg, uint8_t bit) {
  switch (reg) {
  case GPIO_REGISTER_B:
  case GPIO_REGISTER_C:
  case GPIO_REGISTER_D:
  case GPIO_REGISTER_E:
  case GPIO_REGISTER_F:
    return (gpio_read(reg, GPIO_TYPE_DDR) >> bit) & 1;
  default:
    return GPIO_ERROR_INVALID_REG;
  }
  return GPIO_ERROR_INVALID_REG;
}

int16_t gpio_set_level(uint8_t reg, uint8_t bit, uint8_t level) {
  uint8_t levels;

  switch (reg) {
  case GPIO_REGISTER_B:
    levels = PORTB;
    break;
  case GPIO_REGISTER_C:
    levels = PORTC;
    break;
  case GPIO_REGISTER_D:
    levels = PORTD;
    break;
  case GPIO_REGISTER_E:
    levels = PORTE;
    break;
  case GPIO_REGISTER_F:
    levels = PORTF;
    break;
  default:
    return GPIO_ERROR_INVALID_REG;
  }

  BIT_RESET(levels, bit, level);

  return gpio_write(reg, GPIO_TYPE_PORT, levels);
}

int16_t gpio_get_level(uint8_t reg, uint8_t bit) {
  switch (reg) {
  case GPIO_REGISTER_B:
  case GPIO_REGISTER_C:
  case GPIO_REGISTER_D:
  case GPIO_REGISTER_E:
  case GPIO_REGISTER_F:
    return (gpio_get_levels(reg) >> bit) & 1;
  default:
    return GPIO_ERROR_INVALID_REG;
  }
  return GPIO_ERROR_INVALID_REG;
}

int16_t gpio_write(uint8_t reg, uint8_t type, uint8_t value) {
  switch (type) {
  case GPIO_TYPE_MCUCR:
    MCUCR = value;
    break;
  case GPIO_TYPE_DDR:
    switch (reg) {
    case GPIO_REGISTER_B:
      DDRB = value;
      break;
    case GPIO_REGISTER_C:
      DDRC = value & (_BV(PC6) | _BV(PC7));
      break;
    case GPIO_REGISTER_D:
      DDRD = value;
      break;
    case GPIO_REGISTER_E:
      DDRE = value & (_BV(PE2) | _BV(PE6));
      break;
    case GPIO_REGISTER_F:
      DDRF = value &
             (_BV(PF0) | _BV(PF1) | _BV(PF4) | _BV(PF5) | _BV(PF6) | _BV(PF7));
      break;
    default:
      return GPIO_ERROR_INVALID_REG;
    }
    break;
  case GPIO_TYPE_PORT:
    switch (reg) {
    case GPIO_REGISTER_B:
      PORTB = value;
      break;
    case GPIO_REGISTER_C:
      PORTC = value & (_BV(PC6) | _BV(PC7));
      break;
    case GPIO_REGISTER_D:
      PORTD = value;
      break;
    case GPIO_REGISTER_E:
      PORTE = value & (_BV(PE2) | _BV(PE6));
      break;
    case GPIO_REGISTER_F:
      PORTF = value &
              (_BV(PF0) | _BV(PF1) | _BV(PF4) | _BV(PF5) | _BV(PF6) | _BV(PF7));
      break;
    default:
      return GPIO_ERROR_INVALID_REG;
    }
    break;
  default:
    return GPIO_ERROR_INVALID_TYPE;
  }
  return GPIO_ERROR_NONE;
}

int16_t gpio_read(uint8_t reg, uint8_t type) {
  switch (type) {
  case GPIO_TYPE_MCUCR:
    return MCUCR;
  case GPIO_TYPE_DDR:
    switch (reg) {
    case GPIO_REGISTER_B:
      return DDRB;
    case GPIO_REGISTER_C:
      return DDRC;
    case GPIO_REGISTER_D:
      return DDRD;
    case GPIO_REGISTER_E:
      return DDRE;
    case GPIO_REGISTER_F:
      return DDRF;
    default:
      return GPIO_ERROR_INVALID_REG;
    }
  case GPIO_TYPE_PORT:
    switch (reg) {
    case GPIO_REGISTER_B:
      return PORTB;
    case GPIO_REGISTER_C:
      return PORTC;
    case GPIO_REGISTER_D:
      return PORTD;
    case GPIO_REGISTER_E:
      return PORTE;
    case GPIO_REGISTER_F:
      return PORTF;
    default:
      return GPIO_ERROR_INVALID_REG;
    }
  case GPIO_TYPE_PIN:
    switch (reg) {
    case GPIO_REGISTER_B:
      return PINB;
    case GPIO_REGISTER_C:
      return PINC;
    case GPIO_REGISTER_D:
      return PIND;
    case GPIO_REGISTER_E:
      return PINE;
    case GPIO_REGISTER_F:
      return PINF;
    default:
      return GPIO_ERROR_INVALID_REG;
    }
  default:
    return GPIO_ERROR_INVALID_TYPE;
  }
  return GPIO_ERROR_INVALID_TYPE;
}
