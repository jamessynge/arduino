#ifndef SENSOR_ETHER_SERVER_ANALOG_RANDOM_H
#define SENSOR_ETHER_SERVER_ANALOG_RANDOM_H

#include <inttypes.h>

class AnalogRandomClass {
  public:
    // Return a random bit (0 or 1), determined by reading the analog pins.
    // Returns -1 if unable to find enough randomness within readLimit reads
    // of analog pins.
    int randomBit(int readLimit=100);

    // Returns an unsigned 32-bit random value, produced by calling randomBit
    // at least 32 times. zero is return if there was not enough randomness
    // available for any of the pins.
    uint32_t random32(int perBitReadLimit=100);

    // Set the seed for the Arduino random number generator using the analog
    // pins as our source of randomness. Returns true if able to get sufficient
    // randomness, false otherwise.
    bool seedArduinoRNG();

  private:
    // static int convertAnalogPinNum(int pinNum);
    // static int readAnalogPinNum(int pinNum);

    int next_pin_ = 0;
};

extern AnalogRandomClass AnalogRandom;

#endif  // SENSOR_ETHER_SERVER_ANALOG_RANDOM_H