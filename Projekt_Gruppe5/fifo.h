#ifndef _FIFO_H
#define _FIFO_H

#include <avr/io.h>
#include <avr/interrupt.h>

typedef struct
{
	uint8_t volatile count;      // # Zeichen im Puffer
	uint8_t size;                // Puffer-Größe
	uint8_t *pread;              // Lesezeiger
	uint8_t *pwrite;             // Schreibzeiger
	uint8_t read2end, write2end; // # Zeichen bis zum Überlauf Lese-/Schreibzeiger
} fifo_t;

void fifo_init (fifo_t*, uint8_t* buf, const uint8_t size);
uint8_t fifo_put (fifo_t*, const uint8_t data);
uint8_t fifo_get_wait (fifo_t*);
int16_t fifo_get_nowait (fifo_t*);

#endif // _FIFO_H
