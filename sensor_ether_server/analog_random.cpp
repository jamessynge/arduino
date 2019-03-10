#include "analog_random.h"

#include "Arduino.h"

// The analog pin identifiers A0, etc., don't have values matching the number
// that is in their name (i.e. A0 != 0). So we build up a table mapping from pin
// number (0, 1, ...) to the corresponding pin identifier (A0, A1, ...).
// As of this writing (March, 2019), the Arduino reference states that the board
// the the greatest number of analog pins has A0 through A14.
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

constexpr int kNumAnalogPins =
    sizeof kAnalogPinTable / sizeof kAnalogPinTable[0];
// We store next_pin_ in a byte, so make sure we don't have too many pins.
static_assert(kNumAnalogPins <= 256, "Too many pins!");

int AnalogRandom::randomBit(int readLimit) {
  // Produce a single random bit by reading from an unreliable source of data,
  // the analog pins of the Arduino. We treat them as a source of biased bits,
  // taking only the low bit of each reading, as it will be the least stable
  // even when reading a pin that is hooked up to a fairly stable voltage). The
  // idea is that we can "debias" a biased source of numbers (e.g. an unfair
  // coin) by taking two readings of the source at a time rather than one, each
  // reading producing 1 bit. Our action table is:
  //
  //   0,0: Try again
  //   0,1: Output a 1
  //   1,0, Output a 0
  //   1,1: Try again.
  //
  // We don't know what is hooked up to our analog pins, so it is possible that
  // there is more correlation on some pins than on others (e.g. a slowly rising
  // voltage, which might result in toggling back and forth between producing a
  // 0 and a 1), so each pair of values is read from a different pins. We
  // cycling through all the pin identifiers in kAnalogPinTable before starting
  // over.
  //
  // This is based on:
  //   http://www.utopiamechanicus.com/article/better-arduino-random-numbers/
  // which in turn is based on earlier work, at least back to Alan Turing.
  // I first read about this in either C/C++ Users Journal or Dr. Dobbs,
  // but haven't found the original reference.

  do {
    // Cycle through the pin numbers.
    int pinNum = next_pin_++ % kNumAnalogPins;
    if (next_pin_ >= kNumAnalogPins) {
      next_pin_ = next_pin_ % kNumAnalogPins;
    }
    // Translate from pin number to pin identifier (e.g. from 3 to A3).
    int pinId = kAnalogPinTable[pinNum];
    // Read the analog pin twice, keeping just the low order bit each time.
    int bit0 = analogRead(pinId) & 1;
    int bit1 = analogRead(pinId) & 1;
    // And if they are different, output bit1.
    if (bit1 != bit0) {
      return bit1;
    }
    // Not different, so we'll try again until we've called analogRead at
    // least readLimit times.
    readLimit -= 2;
  } while (readLimit > 0);
  // Not good, didn't find enough randomness.
  return -1;
}

int AnalogRandom::randomByte(int perBitReadLimit) {
  uint32_t result;
  if (randomBits(8, perBitReadLimit, &result)) {
    return result & 0xff;
  } else {
    return -1;
  }
  // int result = 0;
  // for (int i = 0; i < 8; ++i) {
  //   int bit = randomBit(perBitReadLimit);
  //   if (bit < 0) {
  //     // Not enough randomness found to produce a single bit. Let the
  //     // caller know.
  //     return -1;
  //   }
  //   result = (result << 1) | bit;
  // }
  // return result;
}

uint32_t AnalogRandom::random32(int perBitReadLimit) {
  uint32_t result;
  if (randomBits(8, perBitReadLimit, &result)) {
    return result & 0xff;
  } else {
    return 0;
  }
  // uint32_t result = 0;
  // for (int i = 0; i < 32; ++i) {
  //   int bit = randomBit(perBitReadLimit);
  //   if (bit < 0) {
  //     // Not enough randomness found to produce a single bit. Let the
  //     // caller know.
  //     return 0;
  //   }
  //   result = (result << 1) | bit;
  // }
  // return result;
}

bool AnalogRandom::randomBits(int numBits, int perBitReadLimit, uint32_t* output) {
  uint32_t result = 0;
  for (int i = 0; i < numBits; ++i) {
    int bit = randomBit(perBitReadLimit);
    if (bit < 0) {
      // Not enough randomness found to produce a single bit.
      return false;
    }
    result = (result << 1) | bit;
  }
  *output = result;
  return true;
}



bool AnalogRandom::seedArduinoRNG() {
  uint32_t seed = random32();
  if (seed != 0) {
    randomSeed(seed);
    return true;
  } else {
    return false;
  }
}
