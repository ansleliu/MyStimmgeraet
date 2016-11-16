#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdlib.h>
//#include <util/delay.h>

#include "uart.h"
#include "fifo.h"
#include "delay.h"
#define UART_BUFSIZE_IN      16
#define UART_BUFSIZE_OUT     64

uint8_t uart_inbuf[UART_BUFSIZE_IN];
uint8_t uart_outbuf[UART_BUFSIZE_OUT];

fifo_t uart_infifo;
fifo_t uart_outfifo;

void
UART_Init (void)
{
  // Save Status Register and disable Interrupts
  uint8_t sreg = SREG;
  cli();
  
  // Set Baudrate according to datasheet (16MHz -> 9600 Baud -> 103)
  UBRR0 = 103;
  
  // Enable RX, TX and RX Complete Interrupt
  UCSR0B = (1 << RXEN0)|(1 << TXEN0)|(1 << RXCIE0);
  
  // Reset Complete-Flags
  UCSR0A = (1 << RXC0)|(1 << TXC0);
  
  // Reset Status Register
  SREG = sreg;
  
  // Initialize FIFO Buffers
  fifo_init(&uart_infifo, uart_inbuf, UART_BUFSIZE_IN);
  fifo_init(&uart_outfifo, uart_outbuf, UART_BUFSIZE_OUT);
}

int8_t
UART_PutChar (const uint8_t c)
{


  
  // Put char into TX Buffer
  int8_t ret = fifo_put(&uart_outfifo, c);

  // Enable DRE Interrupt
  UCSR0B |= (1 << UDRIE0);
   
  return ret;
}

// Receive Interrupt Routine
ISR(USART0_RX_vect)
{
  fifo_put(&uart_infifo, UDR0);
}

// Data Register Empty Interrupt
ISR(USART0_UDRE_vect)
{
  if (uart_outfifo.count > 0)
    UDR0 = fifo_get_nowait(&uart_outfifo);
  else
    UCSR0B &= ~(1 << UDRIE0);
}

int16_t
UART_GetChar (void)
{
  return fifo_get_nowait(&uart_infifo);
}

uint8_t
UART_GetChar_Wait (void)
{
  return fifo_get_wait(&uart_infifo);
}

void
UART_PutString (const char *s)
{
  do
    {
      UART_PutChar(*s);
    }
  while (*(s++));
}

void
UART_PutInteger (const int i)
{
  // Buffer for Output
  char buffer[10];
  
  // Convert Integer to ASCII, Base 10
  itoa(i, buffer, 10);
  
  UART_PutString(buffer);
}
