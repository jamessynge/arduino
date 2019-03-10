#ifndef SENSOR_ETHER_SERVER_ANALOG_RANDOM_H
#define SENSOR_ETHER_SERVER_ANALOG_RANDOM_H

#include <inttypes.h>

class AnalogRandomClass {
  public:
    // Return a random bit (0 or 1), determined by reading the analog pins.
    // Returns -1 if unable to find enough randomness within readLimit reads
    // of analog pins.
    int randomBit(int readLimit=100);

    // Returns an 8-bit random value, produced by calling randomBit
    // 8 times. Returns -1 if there was not enough randomness available
    // for any of the bits.
    int randomByte(int perBitReadLimit=100);

    // Returns an unsigned 32-bit random value, produced by calling randomBit
    // 32 times. zero is returned if there was not enough randomness available
    // for any of the bits.
    uint32_t random32(int perBitReadLimit=100);

    // Set the seed for the Arduino random number generator using the analog
    // pins as our source of randomness. Returns true if able to get sufficient
    // randomness, false otherwise.
    bool seedArduinoRNG();

  private:
    // We cycle through the analog pins; this is the next one to read.
    int next_pin_ = 0;
};

extern AnalogRandomClass AnalogRandom;

#endif  // SENSOR_ETHER_SERVER_ANALOG_RANDOM_H