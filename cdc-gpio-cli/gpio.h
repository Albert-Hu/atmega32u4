#ifndef _GPIO_H_
#define _GPIO_H_

#include <stdint.h>

#define BIT_RESET(reg, bit, val)                                               \
  reg = ((reg & ~(1 << bit)) | ((val & 1) << bit))

#define GPIO_ERROR_NONE 0
#define GPIO_ERROR_INVALID_REG -1
#define GPIO_ERROR_INVALID_PIN -2

#define GPIO_DIRECTION_IN 0
#define GPIO_DIRECTION_OUT 1

#define GPIO_TYPE_MCUCR 0x00   // MCU Control Register
#define GPIO_TYPE_DDR 0x02     // Data Direction Register
#define GPIO_TYPE_PORT 0x04    // Port Register
#define GPIO_TYPE_PIN 0x08     // Pin Register
#define GPIO_TYPE_UNKNOWN 0xFF // Unknown Register

#define GPIO_REGISTER_B 0x02
#define GPIO_REGISTER_C 0x04
#define GPIO_REGISTER_D 0x08
#define GPIO_REGISTER_E 0x10
#define GPIO_REGISTER_F 0x20
#define GPIO_REGISTER_U 0xFF // Unknown Register

typedef enum {
  GPIO_UNKNOWN_REG,
  GPIO_MCUCR,
  GPIO_DDRB,
  GPIO_DDRC,
  GPIO_DDRD,
  GPIO_DDRE,
  GPIO_DDRF,
  GPIO_PORTB,
  GPIO_PORTC,
  GPIO_PORTD,
  GPIO_PORTE,
  GPIO_PORTF,
  GPIO_PINB,
  GPIO_PINC,
  GPIO_PIND,
  GPIO_PINE,
  GPIO_PINF,
} gpio_register_t;

typedef enum {
  GPIO_UNKNOWN_PIN,
  GPIO_PB0,
  GPIO_PB1,
  GPIO_PB2,
  GPIO_PB3,
  GPIO_PB4,
  GPIO_PB5,
  GPIO_PB6,
  GPIO_PB7,
  GPIO_PC6,
  GPIO_PC7,
  GPIO_PD0,
  GPIO_PD1,
  GPIO_PD2,
  GPIO_PD3,
  GPIO_PD4,
  GPIO_PD5,
  GPIO_PD6,
  GPIO_PD7,
  GPIO_PE2,
  GPIO_PE6,
  GPIO_PF0,
  GPIO_PF1,
  GPIO_PF4,
  GPIO_PF5,
  GPIO_PF6,
  GPIO_PF7,
} gpio_pin_t;

/* High level APIs */
int16_t gpio_init(void);
int16_t gpio_set_direction(gpio_pin_t pin, uint8_t value);
int16_t gpio_get_direction(gpio_pin_t pin);
int16_t gpio_set_level(gpio_pin_t pin, uint8_t value);
int16_t gpio_get_level(gpio_pin_t pin);

/* Low level APIs */
int16_t gpio_write(gpio_register_t reg, uint8_t value);
int16_t gpio_read(gpio_register_t reg);

#endif // _GPIO_H_
