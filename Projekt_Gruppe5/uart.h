#ifndef _UART_H_
#define _UART_H_

void UART_Init (void);
int8_t UART_PutChar (const uint8_t c);
int16_t UART_GetChar (void);
uint8_t UART_GetChar_Wait (void);
void UART_PutString (const char *s);
void UART_PutInteger (const int i);

#endif
