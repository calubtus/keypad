#ifndef LED_H
#define LED_H

#include <stdio.h>

void sendBit(uint8_t bitVal);

void sendByte(uint8_t byte);

void sendColor(uint8_t green, uint8_t red, uint8_t blue);

#endif