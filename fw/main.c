#include <avr/io.h>
#include <util/delay.h>
#include "usb.h"
#include "uart.h"

int main(void) {
    // Set port C pin 7 as an output
    DDRC |= (1 << PORTC7);
    // turn off the LED
    PORTC &= ~(1 << PORTC7);

    // Set port F pin 6 as an input
    DDRF &= ~(1 << PORTF6);
    // Enable pull-up resistor on PF6
    PORTF |= (1 << PORTF6);

    // Set port F pin 5 as an output
    DDRF |= (1 << PORTF5);

    sendColor(0x00, 0xff, 0x00); // green, red, blue


    uart_init();
	// usb_init();
    while (1) {
        // Check if switch is pressed
        if (!(PINF & (1 << PORTF6))) {
            // Switch is pressed, turn on the LED
            PORTC |= (1 << PORTC7);
            uart_print("b");
        } else {
            // Switch is not pressed, turn off the LED
            PORTC &= ~(1 << PORTC7);
        }

        _delay_ms(50); // Debounce delay
    }
}
