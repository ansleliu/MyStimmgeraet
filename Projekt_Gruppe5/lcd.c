#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdint.h>
#include <stdlib.h>

#include "spi.h"
#include "font.h"
#include "lcd.h"
#include "twi.h"
#include "dataflash.h"

uint8_t lcd_framebuffer[8][128];
uint8_t lcd_frameupdate = 0;
uint8_t lcd_textx = 0;
uint8_t lcd_texty = 0;

void
LCD_Send (uint8_t data)
{
  SPI_MasterTransfer(data);
}

void
LCD_Init (void)
{
  SPI_MasterInit();

  /* Set Register Select and Chip Select as Output */
  DDRC |= (1<<PC7)|(1<<PC6);
  
  /* Backup Status Register and disable Interrupts */
  uint8_t sreg = SREG;
  cli();
  
  /* Starting Init Command Sequence */
  LCD_Command_Mode;
  LCD_Chip_Select;

  LCD_Send(LCD_RESET);
  LCD_Send(LCD_BIAS_1_7);
  LCD_Send(LCD_ELECTRONIC_VOLUME_MODE_SET);
  LCD_Send(0x08);
  LCD_Send(LCD_ADC_SELECT_NORMAL);
  LCD_Send(LCD_COMMON_OUTPUT_MODE_REVERSE);
  LCD_Send(LCD_V5_VOLTAGE_REGULATOR | 0x05);
  LCD_Send(LCD_POWER_CONTROLLER_SET | 0x07);
  LCD_Send(LCD_DISPLAY_ON);
  
  LCD_Chip_Unselect;
  LCD_Data_Mode;
  
  LCD_Clear();
  
  /* Restore Status Register */
  SREG = sreg;
  
  // Initialize TWI for Backlight Control
  TWI_Init();
  Backlight_Off();
  
  // Initialize Dataflash
  dataflash_init();
}

void
LCD_Clear (void)
{
  uint8_t x = 0, y = 0;
  
  for (y = 0; y < 8; y++)
    for (x = 0; x < 128; x++)
      lcd_framebuffer[y][x] = 0;
  
  // update every line (8bits height)
  lcd_frameupdate = 0xff;
  LCD_Update();
}

void
LCD_Update (void)
{
  int8_t page = 7;
  
  /* Backup Status Register and disable Interrupts */
  uint8_t sreg = SREG;
  cli();
  
  do
    {
      if (lcd_frameupdate & (1<<page))
        {
          LCD_Chip_Select;
          LCD_Command_Mode;
          
          LCD_Send(LCD_PAGE_ADDRESS_SET | page);
          LCD_Send(LCD_COLUMN_ADDRESS_SET_H);
          LCD_Send(LCD_COLUMN_ADDRESS_SET_L);
          
          LCD_Data_Mode;
          
          for (uint8_t x = 0; x < 128; x++)
            LCD_Send(lcd_framebuffer[page][x]);
          
          LCD_Chip_Unselect;
        }
    }
  while (page--);
  
  lcd_frameupdate = 0;
  
  /* Restore Status Register */
  SREG = sreg;
}

void
LCD_DrawPixel (uint8_t x, uint8_t y, uint8_t mode)
{
  // Check if x and y are within display coordinates
  if ((x < 128) && (y < 64))
    {
      // Precalculate Page and Pixel values
      uint8_t page = (y / 8);
      uint8_t pixel = (1 << (y % 8));
      
      switch (mode)
        {
        case 0:
          // Clear Pixel
          lcd_framebuffer[page][x] &= ~pixel;
          break;
        
        case 2:
          // Toggle Pixel
          lcd_framebuffer[page][x] ^= pixel;
          break;
        
        default:
          // Set Pixel
          lcd_framebuffer[page][x] |= pixel;
          break;
        }
        
      lcd_frameupdate |= (1 << page);
    }
}

void
LCD_DrawLine (uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t mode)
{
  // look here: http://de.wikipedia.org/wiki/Bresenham-Algorithmus
  int8_t dx = abs(x1 - x0);
  int8_t dy = abs(y1 - y0);
  int8_t sx = x0 < x1 ? 1 : -1;
  int8_t sy = y0 < y1 ? 1 : -1;
  int8_t err = (dx > dy ? dx : -dy) / 2;
  int8_t e2;
  
  for (;;)
    {
      LCD_DrawPixel(x0, y0, mode);
      
      if (x0 == x1 && y0 == y1)
        break;
      
      e2 = err;
      
      if (e2 > -dx)
        {
          err -= dy;
          x0 += sx;
        }
      
      if (e2 < dy)
        {
          err += dx;
          y0 += sy;
        }
    }
}

void
LCD_DrawCircle (uint8_t x0, uint8_t y0, uint8_t radius, uint8_t mode)
{
  // look here: http://de.wikipedia.org/wiki/Bresenham-Algorithmus
  int8_t f = 1 - radius;
  int8_t ddF_x = 0;
  int8_t ddF_y = -2 * radius;
  int8_t x = 0;
  int8_t y = radius;
  
  LCD_DrawPixel(x0, y0 + radius, mode);
  LCD_DrawPixel(x0, y0 - radius, mode);
  LCD_DrawPixel(x0 + radius, y0, mode);
  LCD_DrawPixel(x0 - radius, y0, mode);
  
  while (x < y)
    {
      if (f >= 0)
        {
          y--;
          ddF_y += 2;
          f += ddF_y;
        }
      
      x++;
      ddF_x += 2;
      f += ddF_x + 1;
      
      LCD_DrawPixel(x0 + x, y0 + y, mode);
      LCD_DrawPixel(x0 - x, y0 + y, mode);
      LCD_DrawPixel(x0 + x, y0 - y, mode);
      LCD_DrawPixel(x0 - x, y0 - y, mode);
      LCD_DrawPixel(x0 + y, y0 + x, mode);
      LCD_DrawPixel(x0 - y, y0 + x, mode);
      LCD_DrawPixel(x0 + y, y0 - x, mode);
      LCD_DrawPixel(x0 - y, y0 - x, mode);
    }
}

void
LCD_PutChar (const char c)
{
  // basic support for cr und new line
  switch (c)
    {
    case '\r':
      // Carriage Return
      lcd_textx = 0;
      break;
      
    case '\n':
      // New Line
      if (lcd_texty < 7)
        lcd_texty++;
      break;
      
    default:
      for (uint8_t x = 0; x < 6; x++)
        lcd_framebuffer[lcd_texty][(lcd_textx * 6) + x] = pgm_read_byte(&font[(uint8_t)(c)][x]);
      
      lcd_frameupdate |= (1 << lcd_texty);
  
      if (lcd_textx < 20)
        lcd_textx++;
      break;
    }
}

void
LCD_PutString (const char *s)
{
  // no empty strings allowed!
  if (*s)
    {
      do
        {
          LCD_PutChar(*s);
        }
      while (*(++s));
    }
}

void
LCD_PutString_P (PGM_P s)
{
  while(1)
    {
      unsigned char c = pgm_read_byte (s);
      s++;

      if (c == '\0')
        break;
      
      LCD_PutChar(c);
    }
}

void
LCD_GotoXY (uint8_t x, uint8_t y)
{
  lcd_textx = x;
  lcd_texty = y;
}

void
LCD_WipeLine (unsigned char line)
{
  unsigned char x;
  
  for (x = 0; x < 128; x++)
    lcd_framebuffer[line][x] = 0x00;
  
  lcd_frameupdate |= (1 << line);
}

void
Backlight_Off (void)
{
  TWI_Start();
  TWI_Address_RW(0xc4);
  TWI_Write(0x11);
  TWI_Write(0x00);
  TWI_Write(0x00);
  TWI_Write(0x00);
  TWI_Write(0x00);
  TWI_Write(0x00);
  TWI_Stop();
}

void
Backlight_LED (uint8_t led_selector)
{
  TWI_Start();
  TWI_Address_RW(0xc4);
  TWI_Write(0x15);
  TWI_Write(led_selector);
  TWI_Stop();
}

void
Backlight_PWM (uint8_t pwm, uint8_t prescaler, uint8_t value)
{
  TWI_Start();
  TWI_Address_RW(0xc4);
  
  if (pwm)
    TWI_Write(0x13);
  else
    TWI_Write(0x11);

  TWI_Write(prescaler);
  TWI_Write(value);
  TWI_Stop();
}

void
LCD_SavePage (unsigned int page)
{
  // transfer framebuffer to dataflash using buffer 2
  unsigned char line = 0;
  
  for (line = 0; line < 8; line++)
    {
      dataflash_buffer_write(2, 0, 128, lcd_framebuffer[line]);
      dataflash_buffer_to_page(page + line, 2);
    }
}

void
LCD_LoadPage (unsigned int page)
{
  // transfer dataflash page to framebuffer
  unsigned char line = 0;

  for (line = 0; line < 8; line++)
    {
      dataflash_read(page + line, 0, 128, lcd_framebuffer[line]);
    }

  // mark all lines to be updated
  lcd_frameupdate = 0xff;
  LCD_Update();
}


// Nachfolgende Funktionen sind eingefügt worden

void LCD_Zahlen (uint8_t Zahl)
{
	
	for (uint8_t x= 0; x < 6; x++)										// x wird von null bis 5 hochgezählt.
	lcd_framebuffer[lcd_texty][lcd_textx + x] = Extrazeichen [Zahl][x]; // In die Matrix lcd_framebuffer wird auf der Position lcd_texty und lcd_textx (beide durch funktion GoToXY erhalten, y geht von 0 bis 7 (Zeile), x von 0 - 127 (Pixel, Spalten))+ x Wert von Matrix Zahl geschrieben.
	lcd_frameupdate |= (1 << lcd_texty);								// Matrix Zahl beinhaltet Zahlen von 0-9, Pfeil rechts/links, Punkt und Loschen eines Blockes. 
}


void Metronom_Zahl (uint8_t zahl)													// Über die Funktion wird die Animation im Metronommenue ausgegeben.
{
	
LCD_GotoXY(66,2);																	// Startwert für x,y.
	
	
for (uint8_t y= 0; y < 6; y++)														// Zahlen erstrecken sich über 6 Zeilen.
	for (uint8_t x= 0; x < 48; x++)													// Und über 48 Pixel in X-Richtung.
	{
		switch(zahl)
		{
			case 1:
			lcd_framebuffer[lcd_texty + y][lcd_textx + x] = ~ Eins [y][x];			// Invertierte 1. Matrix Eins beinhaltet Bitbelegung für Ansteuerung der einzelnen Pixel.
			break;
			
			case 2:
			lcd_framebuffer[lcd_texty + y][lcd_textx + x] = Zwei [y][x];			// Matrix Zwei beinhaltet Bitbelegung für Ansteuerung der einzelnen Pixel.
			break;
			
			case 3:
			lcd_framebuffer[lcd_texty + y][lcd_textx + x] = Drei [y][x];			// Matrix Drei beinhaltet Bitbelegung für Ansteuerung der einzelnen Pixel.
			break;
			
			case 4:
			lcd_framebuffer[lcd_texty + y][lcd_textx + x] = Vier [y][x];			// Matrix Vier beinhaltet Bitbelegung für Ansteuerung der einzelnen Pixel.
			break;
			
			case 5:
			lcd_framebuffer[lcd_texty + y][lcd_textx + x] = Fuenf [y][x];			// Matrix Fuenf beinhaltet Bitbelegung für Ansteuerung der einzelnen Pixel.
			break;
			
			case 6:
			lcd_framebuffer[lcd_texty + y][lcd_textx + x] = Sechs [y][x];			// Matrix Sechs beinhaltet Bitbelegung für Ansteuerung der einzelnen Pixel.
			break;
			
			case 7:
			lcd_framebuffer[lcd_texty + y][lcd_textx + x] = Sieben [y][x];			// Matrix Sieben beinhaltet Bitbelegung für Ansteuerung der einzelnen Pixel.
			break;
			
			case 8:
			lcd_framebuffer[lcd_texty + y][lcd_textx + x] = Acht [y][x];			// Matrix Acht beinhaltet Bitbelegung für Ansteuerung der einzelnen Pixel.
			break;
						
		}
		lcd_frameupdate |= (1 << (lcd_texty + y));
		
	}

}

