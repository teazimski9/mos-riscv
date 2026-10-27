#ifndef UART_H
#define UART_H

void uart_putc(char character);
char uart_getc(void);
void uart_write(const char *text);

#endif
