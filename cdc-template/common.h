#ifndef _ATMEGA32U4_COMMON_H_
#define _ATMEGA32U4_COMMON_H_

#include <stdint.h>

#define TYPE_TEST 0x00
#define TYPE_I2C 0x01
#define TYPE_SPI 0x02
#define TYPE_UART 0x03
#define TYPE_GPIO 0x04
#define TYPE_RESET 0xff

#define REQUEST_BUSY 0x00
#define REQUEST_DONE 0x01

#define RESPONSE_NOT_READY 0x00
#define RESPONSE_READY 0x01

struct request {
  uint8_t type;
  uint8_t command;
  uint8_t length;
  uint8_t reserved;
  uint8_t data[60];
};

struct response {
  uint8_t type;
  uint8_t status;
  uint8_t length;
  uint8_t reserved;
  uint8_t data[60];
};

#endif // _ATMEGA32U4_COMMON_H_