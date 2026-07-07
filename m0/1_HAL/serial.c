#include "ti_msp_dl_config.h"
#include "serial.h"


void serial_sendchar(UART_Regs *uart, const uint8_t data)
{
    DL_UART_transmitDataBlocking(uart, data);
}
void serial_sendstring(UART_Regs *uart, const uint8_t * str)
{
    while(*str)
    {
        serial_sendchar(uart, *str);
        str++;
    }
}

/* 直接发送接口（不经过 printf），可靠 */
void serial_printf(const char *fmt, ...)
{
    /* 简单实现：调用 vsnprintf 再逐字节发送 */
    char buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    serial_sendstring(UART_0_INST, (const uint8_t *)buf);
}

/* -------- newlib 底层重定向：printf 最终走到这里 -------- */
int fputs(const char* restrict s, FILE* restrict stream)
{
    uint16_t i, len;
    len = strlen(s);
    for(i = 0; i < len; i++)
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

/* newlib printf 实际调用的 syscall：重定向到 UART0 */
int _write(int fd, char *ptr, int len)
{
    (void)fd;
    for (int i = 0; i < len; i++)
    {
        DL_UART_transmitDataBlocking(UART_0_INST, (uint8_t)ptr[i]);
    }
    return len;
}

int _read(int fd, char *ptr, int len)
{
    (void)fd;
    (void)ptr;
    (void)len;
    return -1;
}
