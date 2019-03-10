#include "analog_random.h"

#include "Arduino.h"

AnalogRandomClass AnalogRandom;

static const int kAnalogPinTable[] = {
    A0, A1, A2,
#if NUM_ANALOG_INPUTS > 3
    A3,
#endif
#if NUM_ANALOG_INPUTS > 4
    A4,
#endif
#if NUM_ANALOG_INPUTS > 5
    A5,
#endif
#if NUM_ANALOG_INPUTS > 6
    A6,
#endif
#if NUM_ANALOG_INPUTS > 7
    A7,
#endif
#if NUM_ANALOG_INPUTS > 8
    A8,
#endif
#if NUM_ANALOG_INPUTS > 9
    A9,
#endif
#if NUM_ANALOG_INPUTS > 10
    A10,
#endif
#if NUM_ANALOG_INPUTS > 11
    A11,
#endif
#if NUM_ANALOG_INPUTS > 12
    A12,
#endif
#if NUM_ANALOG_INPUTS > 13
    A13,
#endif
#if NUM_ANALOG_INPUTS > 14
    A14,
#endif
};

constexpr int kNumAnalogPins = sizeof kAnalogPinTable / sizeof kAnalogPinTable[0];

int AnalogRandomClass::randomBit(int readLimit) {
  // This is based on:
  //   http://www.utopiamechanicus.com/article/better-arduino-random-numbers/
  // which in turn is based on earlier work, at least back to Alan Turing.
  // The idea is that we can "debias" a biased source of numbers (e.g. an
  // unfair coin) by taking two readings at a time rather than one, each
  // reading producing 1 bit. If the values are:
  //   0,0: Try again
  //   0,1: Output a 1
  //   1,0, Output a 0
  //   1,1: Try again.
  // I first read about this in either C/C++ Users Journal or Dr. Dobbs,
  // but haven't found the original reference.
  // We use the analog pins as our source of biased readings. We'll cycle
  // through all of the available analog pins just in case some are well
  // grounded, so not producing any noise. Note though that for any one
  // pair of values, we read the same one pin.
  do {
    int pinNum = next_pin_++ % kNumAnalogPins;
    if (next_pin_ >= kNumAnalogPins) {
      next_pin_ = next_pin_ % kNumAnalogPins;
    }
    int pinId = kAnalogPinTable[pinNum];
    int bit0 = analogRead(pinId) & 1;
    int bit1 = analogRead(pinId) & 1;
    if (bit1 != bit0) {
      return bit1;
    }
    readLimit -= 1;
  } while (readLimit > 0);
  return -1;
}

int AnalogRandomClass::randomByte(int perBitReadLimit) {
  int result = 0;
  for (int i = 0; i < 8; ++i) {
    int bit = randomBit(perBitReadLimit);
    if (bit < 0) {
      return -1;
    }
    result = (result << 1) | bit;
  }
  return result;
}

uint32_t AnalogRandomClass::random32(int perBitReadLimit) {
  uint32_t result = 0;
  for (int i = 0; i < 32; ++i) {
    int bit = randomBit(perBitReadLimit);
    if (bit < 0) {
      return 0;
    }
    result = (result << 1) | bit;
  }
  return result;
}

bool AnalogRandomClass::seedArduinoRNG() {
  uint32_t seed = random32();
  if (seed != 0) {
    randomSeed(seed);
    return true;
  } else {
    return false;
  }
}
