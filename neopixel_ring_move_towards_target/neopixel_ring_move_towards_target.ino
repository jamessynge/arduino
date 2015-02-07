// neopixel_ring_dither worked OK, but I noticed that I can see the shift register
// action as the values are shifted into the ring. This leads me to wonder what it
// looks like if each pixel value adjusts a single step at a time, slowly moving
// towards its target value.

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

// Target doesn't use a real pin, and you shouldn't call begin or show on it;
// instead is supports moveToTarget.
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
    offset = c - pos;
    range = c - b;
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
const uint16_t R_B = 1000;
const uint16_t R_C = 2500;
const uint16_t R_MAX = (10000 / NPIXELS) * NPIXELS; // Integral multiple of NPIXELS please!
const uint16_t R_STEP = R_MAX / NPIXELS;

int red(unsigned long time, uint16_t pixel) {
  uint16_t pos = (uint32_t(pixel) * R_STEP + time) % R_MAX;
  return triangleFunc(R_A, R_B, R_C, pos, 0, 50);
}

const uint16_t G_A = 0;
const uint16_t G_B = 400;
const uint16_t G_C = 1000;
const uint16_t G_MAX = (5000 / NPIXELS) * NPIXELS; // Integral multiple of NPIXELS please!
const uint16_t G_STEP = G_MAX / NPIXELS;

int green(unsigned long time, uint16_t pixel) {
  uint16_t pos = (uint32_t(pixel) * G_STEP + time) % G_MAX;
  return triangleFunc(G_A, G_B, G_C, pos, 0, 50);
}

const uint16_t B_A = 0;
const uint16_t B_B = 2500;
const uint16_t B_C = 4000;
const uint16_t B_MAX = (15000 / NPIXELS) * NPIXELS; // Integral multiple of NPIXELS please!
const uint16_t B_STEP = B_MAX / NPIXELS;

int blue(unsigned long time, uint16_t pixel) {
  uint16_t pos = (uint32_t(pixel) * B_STEP + time) % B_MAX;
  return triangleFunc(B_A, B_B, B_C, pos, 0, 50);
}

unsigned long last_time = 0;
void loop() {
  unsigned long time = millis();
  if (time != last_time) {
    last_time = time;
    for (uint16_t pixel = 0; pixel < NPIXELS; ++pixel) {
      int r = red(time, pixel);
      int g = green(time, pixel);
      int b = blue(time, pixel);
      target.setPixelColor(pixel, r, g, b);
    }
  }
  moveToTarget();
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

