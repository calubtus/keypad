#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include "usb.h"
#include "uart.h"
#include <avr/interrupt.h>
char buffer[40];  // Buffer for formatted string

int main(void) {
    // Set LED(Green) at PC6
    DDRC |= (1 << PORTC6);
    PORTC &= ~(1 << PORTC6);

    // Set LED(Red) at PE6
    DDRE |= (1 << PORTE6);
    PORTE &= ~(1 << PORTE6);

    // Set port F pin 6 as an input
    DDRF &= ~(1 << PORTF6);
    // Enable pull-up resistor on PF6
    PORTF |= (1 << PORTF6);

    // Set port F pin 5 as an output
    DDRF |= (1 << PORTF5);
    PORTF |= (1 << PORTF5);

    // Need to find a new pin for LED programming.
    // sendColor(0x00, 0xff, 0x00); // green, red, blue

    uart_init();
	usb_init();
    while (1) {

        // Check if switch is pressed
        if (!(PINF & (1 << PORTF6))) {
            PORTC |= (1 << PORTC6);
            sprintf(buffer, "usbAddressConfig=%x\r\n", usbAddressConfig);
            uart_print(buffer);
        } else {
            // Switch is not pressed, turn off the LED
            PORTC &= ~(1 << PORTC6);
        }
        _delay_ms(50); // Debounce delay
    }
}
