#include <avr/io.h>
#include <stdint.h>

#include "spi.h"

// Prevent Double Initialization
unsigned short SPI_flag = 0;

void
SPI_MasterInit (void)
{
  /* Check if already initialized */
  if (!(SPI_flag & 1))
    {
      /* Set MOSI and SCK output */
      DDRB |= (1<<PB5)|(1<<PB7);
      
      /* Enable SPI, Master */
      SPCR = (1<<SPE)|(1<<MSTR);
      
      /* Set Double SPI Speed Bit, SPI clock will be fck/2 */
      SPSR = (1<<SPI2X);
      
      /* Set SPI Init Flag */
      SPI_flag = 1;
    }
}

void
SPI_MasterTransfer (unsigned char c)
{
  /* Start transmission */
  SPDR = c;
  
  /* Wait for transmission complete */
  while (!(SPSR & (1<<SPIF)));
}

unsigned char
SPI_MasterTransferRead (unsigned char c)
{
  /* Start transmission */
  SPDR = c;
  
  /* Wait for transmission complete */
  while (!(SPSR & (1<<SPIF)));
  
  /* Return incoming character */
  return SPDR;
}
