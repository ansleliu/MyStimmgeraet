#ifndef _TWI_H_
#define _TWI_H_

void TWI_Init (void);
int16_t TWI_Start (void);
int16_t TWI_Address_RW (uint8_t address);
int16_t TWI_Write (uint8_t data);
void TWI_Stop (void);
void LCD_BacklightLED (uint8_t led_selector);

#endif
