#include <stdint.h>
#include "fifo.h"

static uint8_t _fifo_get (fifo_t* f);

void fifo_init (fifo_t* f, uint8_t* buffer, const uint8_t size)
{
  f->count = 0;
  f->pread = f->pwrite = buffer;
  f->read2end = f->write2end = f->size = size;
}

uint8_t fifo_get_wait (fifo_t* f)
{
  while (!f->count);
  
  return _fifo_get(f);	
}

int16_t fifo_get_nowait (fifo_t* f)
{
  if (!f->count)
    return -1;
    
  return (int)_fifo_get(f);	
}

uint8_t fifo_put (fifo_t* f, const uint8_t data)
{
  if (f->count >= f->size)
    return 0;
    
  uint8_t* pwrite = f->pwrite;
  
  *(pwrite++) = data;
  
  uint8_t write2end = f->write2end;
  
  if (--write2end == 0)
    {
      write2end = f->size;
      pwrite -= write2end;
    }
  
  f->write2end = write2end;
  f->pwrite = pwrite;

  uint8_t sreg = SREG;
  cli();
  f->count++;
  SREG = sreg;
  
  return 1;
}

static uint8_t 
_fifo_get (fifo_t* f)
{
  uint8_t* pread = f->pread;
  uint8_t data = *(pread++);
  uint8_t read2end = f->read2end;
  
  if (--read2end == 0)
    {
      read2end = f->size;
      pread -= read2end;
    }
  
  f->pread = pread;
  f->read2end = read2end;
  
  uint8_t sreg = SREG;
  cli();
  f->count--;
  SREG = sreg;
  
  return data;
}
