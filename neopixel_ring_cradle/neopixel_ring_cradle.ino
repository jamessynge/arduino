// Goal is to look sort of like newton's cradle, which
// is also sort of like the old PDP-11 front panel lights,
// which folks wrote programs to make chase each other (snake?).

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


Adafruit_NeoPixel target = Adafruit_NeoPixel(NPIXELS, 1, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  while (!strip.canShow());
  delay(1000);  // Wait one second.
}

// Adjust the values in strip towards the values in target.
// Return true IFF there is no difference.
bool moveToTarget() {
  bool no_diff = true;
  uint8_t* in_ptr = target.getPixels();
  uint8_t* in_end = in_ptr + (3 * NPIXELS);
  uint8_t* out_ptr = strip.getPixels();
  while (in_ptr < in_end) {
    uint8_t in = *in_ptr;
    uint8_t out = *out_ptr;
    if (in != out) {
      no_diff = false;
      if (in < out) {
        *out_ptr = out - 1;
      } else {
        *out_ptr = out + 1;
      }
    }
    in_ptr++;
    out_ptr++;
  }
  return no_diff;
}

class Cradle {
 public:
  Cradle(
 
 
 private:
};






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
      strip.setPixelColor(pixel, 50, 50, 50);
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

