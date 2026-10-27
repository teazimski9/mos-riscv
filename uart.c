#include "uart.h"

#define UART_BASE 0x10000000UL
#define UART_THR  0
#define UART_RBR  0
#define UART_LSR  5
#define LSR_RX_READY (1U << 0)
#define LSR_TX_IDLE (1U << 5)

static volatile unsigned char *const uart =
    (volatile unsigned char *)UART_BASE;

void uart_putc(char character)
{
    while ((uart[UART_LSR] & LSR_TX_IDLE) == 0) {
    }
    uart[UART_THR] = (unsigned char)character;
}

char uart_getc(void)
{
    while ((uart[UART_LSR] & LSR_RX_READY) == 0) {
    }
    return (char)uart[UART_RBR];
}

void uart_write(const char *text)
{
    while (*text != '\0') {
        if (*text == '\n') {
            uart_putc('\r');
        }
        uart_putc(*text++);
    }
}
