#ifndef _GPIO_H_
#define _GPIO_H_

#include <stdint.h>

#define BIT_RESET(reg, bit, val)                                               \
  reg = ((reg & ~(1 << bit)) | ((val & 1) << bit))

#define GPIO_ERROR_NONE 0
#define GPIO_ERROR_INVALID_TYPE -1
#define GPIO_ERROR_INVALID_REG -2
#define GPIO_ERROR_INVALID_BIT -3
#define GPIO_ERROR_INVALID_DIR -4

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

/* High level APIs */
int16_t gpio_pullup_disable(uint8_t disable);
uint8_t gpio_pullup_disabled(void);

int16_t gpio_set_directions(uint8_t reg, uint8_t directions);
int16_t gpio_get_directions(uint8_t reg);
int16_t gpio_set_levels(uint8_t reg, uint8_t levels);
int16_t gpio_get_levels(uint8_t reg);

int16_t gpio_set_direction(uint8_t reg, uint8_t bit, uint8_t direction);
int16_t gpio_get_direction(uint8_t reg, uint8_t bit);
int16_t gpio_set_level(uint8_t reg, uint8_t bit, uint8_t level);
int16_t gpio_get_level(uint8_t reg, uint8_t bit);

/* Low level APIs */
int16_t gpio_write(uint8_t reg, uint8_t type, uint8_t value);
int16_t gpio_read(uint8_t reg, uint8_t type);

#endif // _GPIO_H_
