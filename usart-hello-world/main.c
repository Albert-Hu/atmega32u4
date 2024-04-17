#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <stdio.h>

#define UNUSED(x) (void)(x)

#define USART_BAUDRATE (57600)
#define F_CPU (16000000UL)
#define USART_UBRR1H (((F_CPU / (16UL * USART_BAUDRATE)) - 1) >> 8)
#define USART_UBRR1L ((F_CPU / (16UL * USART_BAUDRATE)) - 1)

static FILE usart_io;

static int usart_putc(const char c, FILE *stream) {
  UNUSED(stream);
  loop_until_bit_is_set(UCSR1A, UDRE1);
  UDR1 = c;
  return 0;
}

static int usart_getc(FILE *stream) {
  UNUSED(stream);
  loop_until_bit_is_set(UCSR1A, RXC1);
  return UDR1;
}

static void init_hardware(void) {
  cli();

  MCUSR &= ~(1 << WDRF);

  wdt_disable();

  fdev_setup_stream(&usart_io, usart_putc, usart_getc, _FDEV_SETUP_RW);

  stdin = &usart_io;
  stdout = &usart_io;

  /* Set the usart baudrate */
  UBRR1H = USART_UBRR1H;
  UBRR1L = USART_UBRR1L;

  /* Enable receiver and transmitter */
  UCSR1B = (1 << TXEN1) | (1 << RXEN1);

  /* Set frame format: 8data, 2 stop bit */
  UCSR1C = (1 << USBS1) | (3 << UCSZ10);

  sei();
}

int main(void) {

  init_hardware();

  printf("Hello, World!\r\n");

  while (1)
    ;
}
