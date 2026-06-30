#include "ti_msp_dl_config.h"
#include "serial.h"


void serial_sendchar(UART_Regs *uart, const uint8_t data)
{
    DL_UART_transmitDataBlocking(uart,data);    
}
void serial_sendstring(UART_Regs *uart, const uint8_t * str)
{
    while(*str)
    {
        serial_sendchar(uart,*str);
        str++;
    }
}
int fputs(const char* restrict s, FILE* restrict stream)
{
    uint16_t i, len;
    len = strlen(s);
    for(i=0; i<len; i++)
    {
        DL_UART_transmitDataBlocking(UART_0_INST, (uint8_t)s[i]);  
    }
    return len;
}

int puts(const char * ptr)
{
    int count = fputs(ptr, stdout); 
    count += fputs("\n", stdout);
    return count;
}

int fputc(int c, FILE *stream)
{
    DL_UART_transmitDataBlocking(UART_0_INST, (uint8_t)c);
    return c;
}