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
// Pixels are initialized to 0 (off).
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// Target doesn't use a real pin, and you shouldn't call begin or show on it;
// instead it supports moveToTarget.
Adafruit_NeoPixel target = Adafruit_NeoPixel(NPIXELS, 1, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  // If analog input pin 0 is unconnected, random analog
  // noise will cause the call to randomSeed() to generate
  // different seed numbers each time the sketch runs.
  // randomSeed() will then shuffle the random function.
  randomSeed(analogRead(0));

  while (!strip.canShow());  // Don't start loop until pixels initialized.
  delay(1000);  // Wait one second 
}

void loop() {
  colorChase();
  fadeToZero();
  theaterChase(strip.Color(0, 0, 64), 50); // Blue
  fadeToZero();

  fadingChase2();
  fadeToZero();
  theaterChase(strip.Color(64, 0, 0), 50); // Red
  fadeToZero();

  fadingChase();
  fadeToZero();
  theaterChase(strip.Color(0, 64, 0), 50); // Green
  fadeToZero();
}

void drawFade10(int pixel, uint8_t r, uint8_t g, uint8_t b) {
  // We assume here that np is greater than 10, which is the
  // number of pixel values we set. We also assume that np*2
  // is less than the max positive integer.
  const int np = strip.numPixels();
  if (pixel < 0) {
    pixel = np;
  } else {
    pixel = (pixel % np) + np;
  }
#ifdef PIXEL
#undef PIXEL
#endif
#define PIXEL ((pixel--) % np)
  strip.setPixelColor(PIXEL, r, g, b);
  r >>= 1;
  g >>= 1;
  b >>= 1;
  strip.setPixelColor(PIXEL, r, g, b);
  r -= r >> 2;
  g >>= 1;
  b >>= 1;
  strip.setPixelColor(PIXEL, r, g, b);
  r >>= 1;
  g >>= 1;
  b >>= 1;
  strip.setPixelColor(PIXEL, r, g, b);
  r -= r >> 2;
  g >>= 1;
  b >>= 1;
  strip.setPixelColor(PIXEL, r, g, b);
  r >>= 1;
  g >>= 1;
  b >>= 1;
  strip.setPixelColor(PIXEL, r, g, b);
  r -= r >> 2;
  g >>= 1;
  b >>= 1;
  strip.setPixelColor(PIXEL, r, g, b);
  r >>= 1;
  g >>= 1;
  b >>= 1;
  strip.setPixelColor(PIXEL, r, g, b);
  r -= r >> 2;
  g >>= 1;
  b >>= 1;
  strip.setPixelColor(PIXEL, r, g, b);
  strip.setPixelColor(PIXEL, 0, 0, 0);
#undef PIXEL
}

void fadingChase() {
  int np = strip.numPixels();
  int np2 = np / 2;
  int waitFor = 50;
  while (waitFor > 0) {
    for (int i = 0; i < np; i++) {
      drawFade10(i, 150, 75, 150);
      drawFade10(i + np2, 75, 150, 75);
      strip.show();
      delay(waitFor);
    }
    waitFor--;
  }
}

// Returns true if have faded to black (off);
// false returned if any intensity is non-zero.
bool integerFade(Adafruit_NeoPixel* ring, uint8_t reduce) {
  bool allAreZero = true;
  int np = ring->numPixels();
  uint8_t* ptr = ring->getPixels();
  uint8_t* end = ptr + (3 * np);
  while (ptr < end) {
    uint8_t c = *ptr;
    if (c <= reduce) {
      *ptr = 0;
    } else {
      *ptr = c - reduce;
      allAreZero = false;
    }
    ptr++;
  }
  return allAreZero;
}

void fadeToZero() {
  bool allAreZero = false;
  while (!allAreZero) {
    allAreZero = integerFade(&strip, 1);
    strip.show();
    delay(20);
  }
}

void setMinimumPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b) {
  uint32_t c = strip.getPixelColor(n);
  uint8_t r0 = (c >> 16) & 0xff;
  uint8_t g0 = (c >> 8) & 0xff;
  uint8_t b0 = c & 0xff;
  strip.setPixelColor(n, max(r0, r), max(g0, g), max(b0, b));  
}

void fadingChase2() {
  const int np = strip.numPixels();
//  int np2 = np / 2;
//  int waitFor = 50;
  int i = 0;
  int j = np/3;
  int k = (np * 2) / 3;
  unsigned long time = millis();
  unsigned long lastTime = time;
  unsigned long step = 100;  // milliseconds between updates
  unsigned long nextStepTime = time + step;
  unsigned long nextSpeedTime = time + 10 * step;

  while (step > 0) {
    while (!strip.canShow());
    while (time == lastTime) {
      time = millis();
    }
    lastTime = time;
    integerFade(&strip, 1);

    // Is it time to advance the bright pixels?
    if (time >= nextStepTime || time <= 10) {
      i = (i + 1) % np;
      j = (j + 1) % np;
      k = (k + 1) % np;
      nextStepTime += step;
      if (time >= nextSpeedTime) {
        step--;
        nextSpeedTime += step * 10;
      }
    }

    setMinimumPixelColor(i, 128, 64, 64);
    setMinimumPixelColor(j, 64, 128, 64);
    setMinimumPixelColor(k, 64, 64, 128);

    strip.show();
  }
}

// Encapsulates a triangle function:
//      positions 0 to a have intensity 'lo'.
//      positions a to b have intensities linearly varying between 'lo' and 'hi'.
//      positions b to c have intensities linearly varying between 'hi' and 'lo'.
//      positions c to d (maximum position) have intensity lo.
// Constraints: lo < hi, a < b < c < d.
class Triangle {
 public:
  Triangle(uint16_t a, uint16_t b, uint16_t c, uint16_t d, uint8_t lo, uint8_t hi)
      : a_(a), b_(b), c_(c), d_(d - d % NPIXELS), step_(d_ / NPIXELS),
        lo_(lo), hi_(hi), hi_lo_(hi - lo), origin_(0), forward_(false) {}

  void swapDirection() { forward_ = !forward_; }

  void adjustOrigin(unsigned long delta) {
    if (forward_) {
      origin_ += d_;
      origin_ -= (delta % d_);
    } else {
      origin_ += delta;
    }
    origin_ %= d_;
  }

  uint8_t intensity(uint16_t pixel) {
    uint16_t pos = (uint32_t(pixel) * step_ + origin_) % d_;
    return valueAtPosition(pos);
  }

  uint16_t d() { return d_; }

 private:
  uint8_t valueAtPosition(uint16_t pos) {
    if (pos <= a_ || pos >= c_) {
      return lo_;
    }
    uint32_t offset;
    uint32_t range;
    if (pos <= b_) {
      offset = pos - a_;
      range = b_ - a_;
    } else {
      offset = c_ - pos;
      range = c_ - b_;
    }
  
    // Floating point version:
    // return     int(float(offset) / float(range) * (hi - lo) + lo)
    // ==         int(float(offset * (hi - lo)) / float(range) + lo)
    // Should look in graphic gems, or similar, for ideas on fast integer math.
  
    uint32_t numerator = offset * hi_lo_;
    uint32_t quotientX256 = (numerator << 8) / range;
    uint32_t quotient = quotientX256 >> 8;
    if (quotient < hi_lo_ && quotientX256 & 0x80) {
      // Round up.
      quotient++;
    }
    return quotient + lo_;
  }

  unsigned long origin_;
  const uint16_t a_, b_, c_, d_, step_;
  const uint8_t lo_, hi_, hi_lo_;
  bool forward_;
};

Triangle red_tri(0, 1000, 2500, 10000, 0, 50);
Triangle green_tri(0, 400, 1000, 5000, 0, 50);
Triangle blue_tri(0, 2500, 4000, 15000, 0, 50);

void colorChase() {
  unsigned long time = millis();
  unsigned long end = time + 40L * 1000L;
  unsigned long last_time = time;
  target.clear();
  int reduce = 0;
  while (true) {
    time = millis();
    if (time != last_time) {
      if (10 == random(0, red_tri.d())) red_tri.swapDirection();
      if (10 == random(0, green_tri.d())) green_tri.swapDirection();
      if (10 == random(0, blue_tri.d())) blue_tri.swapDirection();
      unsigned long dt = time - last_time;
      red_tri.adjustOrigin(dt);
      green_tri.adjustOrigin(dt);
      blue_tri.adjustOrigin(dt);
      last_time = time;
      for (uint16_t pixel = 0; pixel < NPIXELS; ++pixel) {
        int r = red_tri.intensity(pixel);
        int g = green_tri.intensity(pixel);
        int b = blue_tri.intensity(pixel);
        target.setPixelColor(pixel, r, g, b);
      }
      if (time > end) {
        ++reduce;
        if (integerFade(&target, reduce)) return;
        end += 20;
      }
    }
    moveToTarget();
    strip.show();
  }
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

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<30; j++) {  //do 30 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip.show();
     
      delay(wait);
     
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

