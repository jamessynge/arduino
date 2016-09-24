// Goal is to see how smoothly I can display a pattern that is not pixel aligned
// (i.e. dither it).

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

void setup() {
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  while (!strip.canShow());
  delay(1000);  // Wait one second.
}

#define MAX_TIME 10000

// Avoiding the use of floating point, so simulate with a form of fixed point.
// Instead of floating point values from 0 to 1.0, or 0 to 24 (for the number of pixels),
// we'll use 0 to 65535 (uint16_t), which then allows us to multiply them producing
// a value that fits in a uint32_t.

// A possible function for producing a color intensity at a particular position:
// Results below a and above c are lo; values linearly go from lo to hi as one
// moves away from a and c and toward b (between a and c). At b hi is returned.
uint8_t triangleFunc(
    uint16_t a, uint16_t b, uint16_t c, uint16_t pos, uint8_t lo, uint8_t hi) {
  if (pos <= a || pos >= c) {
    return lo;
  }
  uint32_t offset;
  uint32_t range;
  if (pos <= b) {
    offset = pos - a;
    range = b - a;
  } else {
    range = c - b;
    offset = c - pos;
  }

  // Floating point version:
  // return     int(float(offset) / float(range) * (hi - lo) + lo)
  // ==         int(float(offset * (hi - lo)) / float(range) + lo)
  // Should look in graphic gems, or similar, for ideas on fast integer math.

  const uint8_t intensity_delta = hi - lo;
  uint32_t numerator = offset * intensity_delta;
  uint32_t quotientX256 = (numerator << 8) / range;
  uint32_t quotient = quotientX256 >> 8;
  if (quotient < intensity_delta && quotientX256 & 0x80) {
    // Round up.
    quotient++;
  }
  return quotient + lo;
}

// A possible function for producing color intensity at a particular time.
int intensity1(unsigned long time, uint16_t cycle_length, uint16_t pixel) {
  uint16_t t = time % cycle_length;
  return 0;
}

int intensity2(unsigned long time, uint16_t cycle_length, uint16_t pixel) {
  return 0;
}

const uint16_t R_A = 0;
const uint16_t R_B = 4000;
const uint16_t R_C = 6000;
const uint16_t R_MAX = (10000 / NPIXELS) * NPIXELS; // Integral multiple of NPIXELS please!
const uint16_t R_STEP = R_MAX / NPIXELS;

int red(unsigned long time, uint16_t pixel) {
  uint16_t pos = (uint32_t(pixel) * R_STEP + time) % R_MAX;
  return triangleFunc(R_A, R_B, R_C, pos, 1, 64);
}

unsigned long last_time = 0;
uint8_t saw_same_time = 0;

void loop() {
  unsigned long time = millis();
  if (time == last_time) {
    saw_same_time = 1;
    return;
  }
  last_time = time;
  for (uint16_t pixel = 0; pixel < NPIXELS; ++pixel) {
    int r = red(time, pixel);
    if (pixel == 0 && saw_same_time) {
      strip.setPixelColor(pixel, r, 0, 200);
      saw_same_time = 0;
    } else {
      strip.setPixelColor(pixel, r, 0, 0);
    }
  }
  strip.show();
}

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

