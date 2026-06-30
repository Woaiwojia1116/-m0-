#include "delay.h"


void delay_ms(uint16_t ms)
{
    while(ms--)
      delay_cycles(32000);
}
void delay_us(uint16_t us)
{
  while(us--)
    delay_cycles(32);
}