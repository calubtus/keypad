#ifndef UART_H
#define UART_H

#define BAUD 38400UL

#define xstr(s) str(s)
#define str(s) #s

void uart_init(void);
void uart_print(const char* str);

#endif