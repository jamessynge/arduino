#include <Adafruit_NeoPixel.h>

#define PIN 6

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(24, PIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

void setup() {
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}

void loop() {
  fadingChase2();
  theaterChase(strip.Color(  0,   0, 127), 50); // Blue

  fadingChase();
  theaterChase(strip.Color(127,   0,   0), 50); // Red


  // Send a theater pixel chase in...
//  theaterChase(strip.Color(127, 127, 127), 50); // White
//  theaterChase(strip.Color(  0,   0, 127), 50); // Blue

//  theaterChaseRainbow(50);


/*
  // Some example procedures showing how to display to the pixels:
  colorWipe(strip.Color(255, 0, 0), 50); // Red
  colorWipe(strip.Color(0, 255, 0), 50); // Green
  colorWipe(strip.Color(0, 0, 255), 50); // Blue
  rainbow(20);
  rainbowCycle(20); */
}

void drawFade6(int lead, uint8_t r, uint8_t g, uint8_t b) {
  int np = strip.numPixels();
  strip.setPixelColor(lead % np, r, g, b);
  r >>= 1;
  g >>= 1;
  b >>= 1;
  lead = (lead + np - 1) % np;
  strip.setPixelColor(lead % np, r, g, b);
  r -= r >> 2;
  g >>= 1;
  b >>= 1;
  lead = (lead + np - 1) % np;
  strip.setPixelColor(lead % np, r, g, b);
  r >>= 1;
  g >>= 1;
  b >>= 1;
  lead = (lead + np - 1) % np;
  strip.setPixelColor(lead % np, r, g, b);
  r -= r >> 2;
  g >>= 1;
  b >>= 1;
  lead = (lead + np - 1) % np;
  strip.setPixelColor(lead % np, r, g, b);
  r >>= 1;
  g >>= 1;
  b >>= 1;
  lead = (lead + np - 1) % np;
  strip.setPixelColor(lead % np, r, g, b);
  r -= r >> 2;
  g >>= 1;
  b >>= 1;
  lead = (lead + np - 1) % np;
  strip.setPixelColor(lead % np, r, g, b);
  r >>= 1;
  g >>= 1;
  b >>= 1;
  lead = (lead + np - 1) % np;
  strip.setPixelColor(lead % np, r, g, b);
  r -= r >> 2;
  g >>= 1;
  b >>= 1;
  lead = (lead + np - 1) % np;
  strip.setPixelColor(lead % np, r, g, b);
  lead = (lead + np - 1) % np;
  strip.setPixelColor(lead % np, 0, 0, 0);
}

void integerFade(uint8_t reduce) {
  int np = strip.numPixels();
  uint8_t* ptr = strip.getPixels();
  uint8_t* end = ptr + (3 * np);
  while (ptr < end) {
    uint8_t c = *ptr;
    if (c < reduce) {
      *ptr = 0;
    } else {
      *ptr = c - reduce;
    }
    ptr++;
  }
}

void fadingChase() {
  int np = strip.numPixels();
  int np2 = np / 2;
  int waitFor = 50;
  while (waitFor > 0) {
    for (int i = 0; i < np; i++) {
      drawFade6(i, 200, 100, 200);
      drawFade6(i + np2, 200, 100, 200);
      strip.show();
      delay(waitFor);
    }
    waitFor--;
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
    integerFade(1);

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

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
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

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
        for (int i=0; i < strip.numPixels(); i=i+3) {
          strip.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
        }
        strip.show();
       
        delay(wait);
       
        for (int i=0; i < strip.numPixels(); i=i+3) {
          strip.setPixelColor(i+q, 0);        //turn every third pixel off
        }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if(WheelPos < 170) {
    WheelPos -= 85;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}

