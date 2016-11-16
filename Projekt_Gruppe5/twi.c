#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#include "twi.h"

volatile uint8_t twi_timeout = 0x00;

void
TWI_Init (void)
{
  // Port Setup
  DDRC &= ~((1 << PC0)|(1 << PC1));
  PORTC |= (1 << PC0)|(1 << PC1);
  
  // Setup TWI Speed to 400kHZ (TWPS = 1)
  TWBR = 3;
  
  // Using TIMER2 to detect timeout
  TCCR2B = (7 << CS20);
  TIMSK2 = (1 << OCIE2A);
  
  OCR2A = 125;
  
  // Interrupts REQUIRED!
  sei();
}

int16_t
TWI_Start (void)
{
  twi_timeout = 10;
  
  TWCR = (1 << TWINT)|(1 << TWSTA)|(1 << TWEN);
  while ((twi_timeout) && (!(TWCR & (1 << TWINT))));
  
  if (twi_timeout)
    return (int16_t) (TWSR & 0xf8);
  else
    return -1;
}

int16_t
TWI_Address_RW (uint8_t address)
{
  twi_timeout = 10;
  
  TWDR = address;
  TWCR = (1 << TWINT)|(1 << TWEN);
  while ((twi_timeout) && (!(TWCR & (1 << TWINT))));
  
  if (twi_timeout)
    return (int16_t) (TWSR & 0xf8);
  else
    return -1;
}

int16_t
TWI_Write (uint8_t data)
{
  twi_timeout = 10;
  
  TWDR = data;
  TWCR = (1 << TWINT)|(1 << TWEN);
  while ((twi_timeout) && (!(TWCR & (1 << TWINT))));
  
  if (twi_timeout)
    return (int16_t) (TWSR & 0xf8);
  else
    return -1;
}

void
TWI_Stop (void)
{
  twi_timeout = 10;
  
  TWCR = (1 << TWINT)|(1 << TWEN)|(1 << TWSTO);
}

// ISR will be called every 8ms to decrease twi_timeout if > 0
ISR (TIMER2_COMPA_vect)
{
  OCR2A += 125;

  if (twi_timeout)
    twi_timeout--;
}
