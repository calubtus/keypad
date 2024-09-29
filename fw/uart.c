#define BAUD 38400UL // TODO: Unsure why this ends up mapping to a 19200 baud rate for PuTTY

#include <avr/io.h>
#include <util/setbaud.h>
#include "uart.h"

void uart_init(void){
    // Set the BAUD rate
    UBRR1H = UBRRH_VALUE;
    UBRR1L = UBRRL_VALUE;

    // Set the Mode & Frame Parameters
    UCSR1C = (1 << UCSZ11) | (1 << UCSZ10);  // Asynchronous, 8-data, No parity, 1-stop

    // Enable USART0 Transmitter and Receiver
    UCSR1B = (1<<RXEN1) | (1 << TXEN1);

}

void uart_transmit(unsigned char data){
    // Wait for empty transmit buffer
    while(!(UCSR1A & (1 << UDRE1)));

    // Put data into buffer, sends the data
    UDR1 = data;

}

unsigned char uart_receive(void){
    // Wait for data to be received
    while(!(UCSR1A & (1 << RXC1)));

    // Get and return received data from buffer
    return UDR1;

}

void uart_print(const char* str) {
    while (*str) {
        uart_transmit(*str++);
    }
}
