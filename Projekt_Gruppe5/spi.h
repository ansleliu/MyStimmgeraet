#ifndef _SPI_H_
#define _SPI_H_

void SPI_MasterInit (void);
void SPI_MasterTransfer (unsigned char c);
unsigned char SPI_MasterTransferRead (unsigned char c);

/* Section only needed, when LCD is on USART1 in SPI Mode */
#if BOARD_REVISION < 3

void SPI2_MasterInit (void);
unsigned char SPI2_MasterTransfer (unsigned char c);

#endif

#endif
