#include <avr/io.h>
#include <stdint.h>

#include "spi.h"
#include "dataflash.h"

void
dataflash_opcode_and_address (unsigned char opcode, unsigned int page, unsigned int offset)
{
  SPI_MasterTransfer(opcode);
  SPI_MasterTransfer((unsigned char)(page >> 7) & 0x1f);
  SPI_MasterTransfer(((unsigned char)(offset >> 8) & 0x01) | ((unsigned char)(page) << 1));
  SPI_MasterTransfer((unsigned char)(offset));
}

void
dataflash_wait (void)
{
  DATAFLASH_Chip_Select;
  SPI_MasterTransfer(DATAFLASH_STATUS_REGISTER_READ);
  
  while (!(SPI_MasterTransferRead(0x00) & 0x80));
  
  DATAFLASH_Chip_Unselect;
}

void
dataflash_init (void)
{
  // AT45DB081 doesn't actually need an intialization,
  // only the ChipSelect should be configured as an output.
  DDRB |= (1 << PB4);
  DATAFLASH_Chip_Unselect;
  SPI_MasterInit();
}

void
dataflash_buffer_to_page (unsigned int page, unsigned char buffer)
{
  dataflash_wait();
  DATAFLASH_Chip_Select;

  switch (buffer)
    {
    default:
      dataflash_opcode_and_address
        (DATAFLASH_BUFFER_1_TO_MAIN_MEMORY_PAGE_PROGRAM_WITH_BUILT_IN_ERASE,
         page, 0x00);
      break;
    
    case 2:
      dataflash_opcode_and_address
        (DATAFLASH_BUFFER_2_TO_MAIN_MEMORY_PAGE_PROGRAM_WITH_BUILT_IN_ERASE,
         page, 0x00);
      break;
    }
  DATAFLASH_Chip_Unselect;
}

void
dataflash_page_to_buffer (unsigned int page, unsigned char buffer)
{
  dataflash_wait();
  DATAFLASH_Chip_Select;

  switch (buffer)
    {
    default:
      dataflash_opcode_and_address 
        (DATAFLASH_MAIN_MEMORY_PAGE_TO_BUFFER_1_TRANSFER,
         page, 0x00);
      break;
    
    case 2:
      dataflash_opcode_and_address 
        (DATAFLASH_MAIN_MEMORY_PAGE_TO_BUFFER_2_TRANSFER,
         page, 0x00);
      break;
    }
  DATAFLASH_Chip_Unselect;
}

void
dataflash_buffer_read (unsigned char buffer, unsigned int offset,
                       unsigned int length, unsigned char *array)
{
  dataflash_wait();
  DATAFLASH_Chip_Select;

  switch (buffer)
    {
    default:
      dataflash_opcode_and_address 
        (DATAFLASH_BUFFER_1_READ_LF,
         0x00, offset);
      break;
    
    case 2:
      dataflash_opcode_and_address 
        (DATAFLASH_BUFFER_2_READ_LF,
         0x00, offset);
      break;
    }
  
  offset = 0x00;
  
  while (length--)
    {
      array[offset++] = SPI_MasterTransferRead(0x00);
    }

  DATAFLASH_Chip_Unselect;
}

void
dataflash_buffer_write (unsigned char buffer, unsigned int offset,
                        unsigned int length, unsigned char *array)
{
  dataflash_wait();
  DATAFLASH_Chip_Select;

  switch (buffer)
    {
    default:
      dataflash_opcode_and_address 
        (DATAFLASH_BUFFER_1_WRITE,
         0x00, offset);
      break;
    
    case 2:
      dataflash_opcode_and_address 
        (DATAFLASH_BUFFER_2_WRITE,
         0x00, offset);
      break;
    }
  
  offset = 0x00;

  while (length--)
    {
      SPI_MasterTransfer(array[offset++]);
    }

  DATAFLASH_Chip_Unselect;
}

void
dataflash_read (unsigned int page, unsigned int offset,
                unsigned int length, unsigned char *array)
{
  dataflash_wait();
  DATAFLASH_Chip_Select;

  dataflash_opcode_and_address(DATAFLASH_CONTINUOUS_ARRAY_READ_LF, page, offset);
  
  offset = 0x00;
  
  while (length--)
    {
      array[offset++] = SPI_MasterTransferRead(0x00);
    }

  DATAFLASH_Chip_Unselect;
}

void
dataflash_chip_erase (void)
{
  dataflash_wait();
  DATAFLASH_Chip_Select;
  SPI_MasterTransfer(DATAFLASH_CHIP_ERASE_1);
  SPI_MasterTransfer(DATAFLASH_CHIP_ERASE_2);
  SPI_MasterTransfer(DATAFLASH_CHIP_ERASE_3);
  SPI_MasterTransfer(DATAFLASH_CHIP_ERASE_4);
  DATAFLASH_Chip_Unselect;
}
