#ifndef __SERIAL_H
#define __SERIAL_H
#include <string.h>
#include <stdio.h>

void serial_sendchar(UART_Regs *uart, const uint8_t data);
void serial_sendstring(UART_Regs *uart, const uint8_t * str);

#endif