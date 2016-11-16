#ifndef _LCD_H_
#define _LCD_H_

#define LCD_Chip_Select         (PORTC &= ~(1<<PC6))
#define LCD_Chip_Unselect       (PORTC |= (1<<PC6))
#define LCD_Command_Mode        (PORTC &= ~(1<<PC7))
#define LCD_Data_Mode           (PORTC |= (1<<PC7))

#define LCD_DISPLAY_ON                    0b10101111
#define LCD_DISPLAY_OFF                   0b10101110
#define LCD_DISPLAY_START_LINE_SET        0b01000000
#define LCD_PAGE_ADDRESS_SET              0b10110000
#define LCD_COLUMN_ADDRESS_SET_H          0b00010000
#define LCD_COLUMN_ADDRESS_SET_L          0b00000000
#define LCD_ADC_SELECT_NORMAL             0b10100000
#define LCD_ADC_SELECT_REVERSE            0b10100001
#define LCD_DISPLAY_NORMAL                0b10100110
#define LCD_DISPLAY_REVERSE               0b10100111
#define LCD_DISPLAY_ALL_POINTS_ON         0b10100101
#define LCD_NORMAL_DISPLAY_MODE           0b10100100
#define LCD_BIAS_1_9                      0b10100010
#define LCD_BIAS_1_7                      0b10100011
#define LCD_READ_MODIFY_WRITE             0b11100000
#define LCD_END                           0b11101110
#define LCD_RESET                         0b11100010
#define LCD_COMMON_OUTPUT_MODE_NORMAL     0b11000000
#define LCD_COMMON_OUTPUT_MODE_REVERSE    0b11001000
#define LCD_POWER_CONTROLLER_SET          0b00101000
#define LCD_V5_VOLTAGE_REGULATOR          0b00100000
#define LCD_ELECTRONIC_VOLUME_MODE_SET    0b10000001
#define LCD_STATIC_INDICATOR_ON           0b10101101
#define LCD_STATIC_INDICATOR_OFF          0b10101100
#define LCD_NOP                           0b11100011
#define LCD_TEST                          0b11110000

#define BL_RED_ON        (1 << 0)
#define BL_GREEN_ON      (1 << 2)
#define BL_BLUE_ON       (1 << 4)
#define BL_RED_PWM0      (2 << 0)
#define BL_GREEN_PWM0    (2 << 2)
#define BL_BLUE_PWM0     (2 << 4)
#define BL_RED_PWM1      (3 << 0)
#define BL_GREEN_PWM1    (3 << 2)
#define BL_BLUE_PWM1     (3 << 4)

#include <avr/pgmspace.h>


extern uint8_t lcd_framebuffer[8][128];
extern uint8_t lcd_frameupdate;

void LCD_Init (void);
void LCD_Clear (void);
void LCD_Update (void);
void LCD_DrawPixel (uint8_t x, uint8_t y, uint8_t mode);
void LCD_DrawLine (uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t mode);
void LCD_DrawCircle (uint8_t x0, uint8_t y0, uint8_t radius, uint8_t mode);
void LCD_PutChar (const char c);
void LCD_PutString (const char *s);
void LCD_PutString_P (PGM_P s);
void LCD_GotoXY (uint8_t x, uint8_t y);
void LCD_WipeLine (unsigned char line);
void LCD_SavePage (unsigned int page);
void LCD_LoadPage (unsigned int page);

void LCD_Zahlen (uint8_t Zahl);
void Metronom_Zahl (uint8_t zahl);


void Backlight_Off (void);
void Backlight_LED (uint8_t led_selector);
void Backlight_PWM (uint8_t pwm, uint8_t prescaler, uint8_t value);

#endif
