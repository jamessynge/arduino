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

// int AnalogRNGClass::convertAnalogPinNum(int pinNum) {
//   if (pinNum >= kNumAnalogPins) {
//     return -1;
//   }
//   int pinId = kAnalogPinTable[pinNum];
//   return pinId;
// }

// int AnalogRNGClass::readAnalogPinNum(int pinNum) {
//   int pinId = convertAnalogPinNum(pinNum);
//   if (pinId < 0) {
//     return -1;
//   }
//   int v = analogRead(pinId);
//   return v;
// }

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



// void seedRNG() {

// #define DEBUG_SEED_RNG
// #ifdef DEBUG_SEED_RNG
//   unsigned long startMicros = micros();
// #endif  // DEBUG_SEED_RNG

//   // randomSeed takes an unsigned long, which which determines the number
//   // of bits we need.
//   int neededBits = 8 * sizeof(unsigned long);

//   // Initialize with a previously generated randomly value just in case we
//   // can't get all the bits we need (i.e. the analog pins are too stable).
//   unsigned long seed = 0x43a61ac0;

//   // ATmega's can perform analogRead about 10K times per second, so let's
//   // limit to that number of analog reads so this doesn't take too long.
//   int loopLimit = 5000;  // Two reads per loop.

//   // A lookup table from integer (pin number) to the corresponding analog pin
//   // identifier (i.e. A0 should not be assumed to be zero). Using the macro
//   // NUM_ANALOG_INPUTS to determine how many analog pins are available, and
//   // hence build the table's values.

// #ifndef NUM_ANALOG_INPUTS
// #error Expected NUM_ANALOG_INPUTS to be a C preprocessor macro.
// #endif

//   int pinTable[] = {
//     A0, A1, A2,
// #if NUM_ANALOG_INPUTS > 3
//     A3,
// #endif
// #if NUM_ANALOG_INPUTS > 4
//     A4,
// #endif
// #if NUM_ANALOG_INPUTS > 5
//     A5,
// #endif
// #if NUM_ANALOG_INPUTS > 6
//     A6,
// #endif
// #if NUM_ANALOG_INPUTS > 7
//     A7,
// #endif
// #if NUM_ANALOG_INPUTS > 8
//     A8,
// #endif
// #if NUM_ANALOG_INPUTS > 9
//     A9,
// #endif
// #if NUM_ANALOG_INPUTS > 10
//     A10,
// #endif
// #if NUM_ANALOG_INPUTS > 11
//     A11,
// #endif
// #if NUM_ANALOG_INPUTS > 12
//     A12,
// #endif
// #if NUM_ANALOG_INPUTS > 13
//     A13,
// #endif
// #if NUM_ANALOG_INPUTS > 14
//     A14,
// #endif
//   };
//   int numPins = sizeof pinTable / sizeof pinTable[0];
// #ifdef DEBUG_SEED_RNG
//   Serial.print("seedRNG: numPins=");
//   Serial.print(numPins);
//   Serial.print("   pinTable=[");
//   for (int i = 0; i < numPins; ++i) {
//     if (i > 0) {
//       Serial.print(", ");
//     }
//     Serial.print(pinTable[i], DEC);
//   }
//   Serial.println("]");
// #endif  // DEBUG_SEED_RNG

//   while ((neededBits > 0) && (loopLimit-- > 0)) {
//     int pinNum = loopLimit % numPins;
//     int pinId = pinTable[pinNum];
//     int bit0= analogRead(pinId) & 1;
//     int bit1= analogRead(pinId) & 1;
//     if (bit1!=bit0) {
//       seed = (seed<<1) | bit1;
//       --neededBits;
//     }
//   }

// #ifdef DEBUG_SEED_RNG
//   unsigned long endMicros = micros();
//   unsigned long elapsedMicros = endMicros - startMicros;
//   Serial.print("seedRNG: neededBits=");
//   Serial.print(neededBits);
//   Serial.print("   loopLimit=");
//   Serial.print(loopLimit);
//   Serial.print("   elapsedMicros=");
//   Serial.print(elapsedMicros);
//   Serial.print("   seed=0x");
//   Serial.print(seed, HEX);
//   Serial.println();
// #endif  // DEBUG_SEED_RNG

//   randomSeed(seed);
// }
