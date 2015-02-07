// Goal is to figure out how fast we can update the neopixel ring.
// According to specs for the WS2812B 3-color led chip,
// the refresh rate is (up to?) 400Hz, and the specs
// also claim that with a refresh rate of 30Hz, you can update
// a cascade of 1024 chips.
// For the NeoPixel Ring 24 driven by the trinket pro 5v (16MHz),
// I get a measurement of around 800 micros for executing
// strip.show() and waiting until strip.canShow() returns true.
// Note that we can use the ~50 micros that canShow consumes
// to do other work.

#include <Adafruit_NeoPixel.h>

#define PIN 6
#define NPIXELS 24

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

void displayUnsigned(unsigned long v) {
  uint16_t pixel = 0;
  while (pixel < NPIXELS - 4) {
    if ((v & 0x1) == 0) {
      strip.setPixelColor(pixel, 1, 1, 1);
    } else {
      strip.setPixelColor(pixel, 0, 0, 128);
    }
    v = v >> 1;
    pixel++;
  }
}

void setup() {
  strip.begin();
  strip.setPixelColor(NPIXELS - 1, 255, 255, 255);
  strip.setPixelColor(NPIXELS - 2, 255, 0, 0);
  strip.setPixelColor(NPIXELS - 3, 0, 255, 0);
  strip.setPixelColor(NPIXELS - 4, 0, 0, 255);
  strip.show(); // Initialize all pixels to 'off'
  while (!strip.canShow());
}

void loop() {
  // Measure how long the time is from showing pixels to being able to call show again.
  unsigned long start = micros();
  strip.show();
  while (!strip.canShow());
  unsigned long end = micros();
  displayUnsigned(end - start);
  delay(1000);
}

