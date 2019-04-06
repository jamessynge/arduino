#ifndef _WALTER_ANDERSON_JITTER_RANDOM_H_
#define _WALTER_ANDERSON_JITTER_RANDOM_H_

#include <inttypes.h>

class JitterRandom {
  public:
    // Returns an unsigned 32-bit pseudo random value. This is based on reading
    // from a timer counter register (one bytes) at un-even intervals relative
    // to that counter, and hashing a sequence of those timer register values.
    // The more we read from the register the better, because the underlying
    // values aren't necessarily well distributed. To get at least 32 bits, we
    // need to read at least 6 times, but more reads will be better.
    static uint32_t random32(int num_register_reads=32);
};

#endif  // _WALTER_ANDERSON_JITTER_RANDOM_H_