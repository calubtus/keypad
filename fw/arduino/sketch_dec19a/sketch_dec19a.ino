#include <Adafruit_NeoPixel.h>
#include <Keyboard.h>

#define NEOPIXEL_PIN A2
#define NUM_PIXELS 1

#define SWITCHA_PIN A1

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();
  strip.setBrightness(25);
  strip.setPixelColor(0, 0xff, 0, 0);
  strip.show(); // Initialize all pixels to 'off'
  pinMode(SWITCHA_PIN, INPUT_PULLUP);

  // Initialize each switch pin as an input
  pinMode(SWITCHA_PIN, INPUT_PULLUP);
  // Begin keyboard functionality
  Keyboard.begin();
}

void loop() {
  strip.setPixelColor(0, 0xff, 0, 0);

  // turn off LED if not pressed!
  if (!digitalRead(SWITCHA_PIN)) {
    strip.setPixelColor(0, 0);
    Keyboard.press('z');
  } else {
    Keyboard.release('z');
  }
  
  strip.show();

  delay(10);
}
