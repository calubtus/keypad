#include <avr/io.h>
#include <util/delay.h>
// #include "led.h"

void sendBit(uint8_t bitVal) {
    if (bitVal) {
        // Send a '1' bit
        PORTF |= (1 << PORTF5);  // Set pin high
        _delay_us(0.8);  // Approximately 800 ns
        PORTF &= ~(1 << PORTF5); // Set pin low
        _delay_us(0.45); // Approximately 450 ns
    } else {
        // Send a '0' bit
        PORTF |= (1 << PORTF5);  // Set pin high
        _delay_us(0.4);  // Approximately 400 ns
        PORTF &= ~(1 << PORTF5); // Set pin low
        _delay_us(0.85); // Approximately 850 ns
    }
}

void sendByte(uint8_t byte) {
    for (int8_t i = 7; i >= 0; i--) {
        sendBit(byte & (1 << i));
    }
}

void sendColor(uint8_t green, uint8_t red, uint8_t blue) {
    sendByte(green);
    sendByte(red);
    sendByte(blue);
    _delay_ms(50); // RES
}