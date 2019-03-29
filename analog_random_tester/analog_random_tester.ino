#include <Arduino.h>
#include <inttypes.h>

#include "analog_random.h"
#include "debug.h"
#include "test.h"

// Returns a random bit (0 or 1), or -1 if it can't produce such a bit.
using RandomBitFn = int(*)();

long GetRandomBits(RandomBitFn next_bit, int nbits) {
  long result = 0;
  while (nbits-- > 0) {
    result <<= 1;
    int r = next_bit();
    switch (r) {
      case -1:
        Serial.println();
        Serial.println("Unable to get enough randomness!");
        return -1;
      case 0:
        break;
      case 1:
        result |= 1;
        break;
      default:
        Serial.println();
        Serial.print("Unexpected value from next_bit function: ");
        Serial.print(r, DEC);
        Serial.print(" (0x");
        Serial.print(r, HEX);
        Serial.println(")");
        return -1;
    }
  }
  return result;
}

template <typename BUCKET_TYPE, int NBITS>
void test_analog_random(RandomBitFn next_bit, const unsigned long bucket_max, const unsigned long report_interval) {
  constexpr int NUM_BUCKETS = 1 << NBITS;
  BUCKET_TYPE buckets[NUM_BUCKETS];

  // Clear buckets.
  Serial.println("Clearing buckets:");
  for (int i = 0; i < NUM_BUCKETS; ++i) {
    buckets[i] = 0;
  }

  // Fill buckets until one would overflow.
  Serial.println("Filling buckets:");
  unsigned long count = 0;
  unsigned long reports = 0;
  while (true) {
    ++count;
    if (count >= report_interval) {
      Serial.print(".");
      count = 0;
      ++reports;
      if (reports > 80) {
        Serial.println();
        reports = 0;
      }
    }
    long r = GetRandomBits(next_bit, NBITS);
    if (r < 0) {
      ASSERT(r == -1);
      Serial.println();
      // Giving up on this experiment.
      return;
    }
    ASSERT(r < NUM_BUCKETS);
    BUCKET_TYPE v = buckets[r];
    if (v >= bucket_max) {
      ASSERT(v == bucket_max);
      break;
    }
    ++v;
    if (v >= bucket_max) {
      ASSERT(v == bucket_max);
      Serial.print("*");
    }
    buckets[r] = v;
  }
  Serial.println();
  Serial.println();
  Serial.println("Filled at least one bucket");

  // Report on bucket fullness.
  for (int i = 0; i < NUM_BUCKETS; ++i) {
    Serial.print("bucket[");
    Serial.print(i);
    Serial.print("] = ");
    Serial.println(buckets[i]);
  }

  Serial.println();
  Serial.println();
}

static AnalogRandom rng;

int AnalogRandomRandomBit() {
  return rng.randomBit();
}

int PseudoRandomBit() {
  return static_cast<int>(random(2));
}

void setup() {
//  Serial.begin(115200);
  Serial.begin(9600);
  delay(500);
  if (!rng.seedArduinoRNG()) {
    Serial.println("UNABLE TO SEED THE RANDOM NUMBER GENERATOR!");
  }
}

void loop() {
  // Collect a bunch of analog values at various time spacings and print
  // to the serial port (this is the reason for boosting the serial speed
  // to 115200... i.e. lots of data).
  // This will enable analysis on a real computer.
  const int kPins[] = {A0, A1, A2, A3, A4, A5};
  const unsigned int kDelayMicros[] = {1, 10, 25, 50, 100, 175, 250, 500, 750, 1000};
  for (const int pin : kPins) {
    for (const unsigned int readDelayMicros : kDelayMicros) {
      Serial.println("#######################################################");
      Serial.print("# pin=");
      Serial.println(pin);
      Serial.print("# delay microseconds=");
      Serial.println(readDelayMicros);
      for (int i = 0; i < 200; ++i) {
        int a = analogRead(pin);
        Serial.println(a);
        delayMicroseconds(readDelayMicros);
      }
    }
  }
  for (const unsigned int readDelayMicros : kDelayMicros) {
    Serial.println("# *****************************************************");
    Serial.print("# delay microseconds=");
    Serial.println(readDelayMicros);
    for (int i = 0; i < 200; ++i) {
      bool first = true;
      for (const int pin : kPins) {
        if (first) {
          first = false;
        } else {
          Serial.print(",");
        }
        int a = analogRead(pin);
        Serial.print(a);
        delayMicroseconds(readDelayMicros);
      }
      Serial.println();
    }
  }
}

void loop_old() {
  #define ARRAY_ELEMS(a) ((sizeof a) / (sizeof a[0]))
  static const int kNBitsChoices[] = {1, 2, 3, 4, 5};
  static const long kBucketMaxChoices[] = {256, 1024, 4096};
  constexpr auto kNumNbitsChoices = ARRAY_ELEMS(kNBitsChoices);
  constexpr auto kNumBucketMaxChoices = ARRAY_ELEMS(kBucketMaxChoices);
  static int nbits_index = 0;
  static int bucket_max_index = 0;
  static bool use_analog_random = false;

  const auto nbits = kNBitsChoices[nbits_index];
  const auto bucket_max = kBucketMaxChoices[bucket_max_index];
  const auto next_bit = use_analog_random ? &AnalogRandomRandomBit : &PseudoRandomBit;

  Serial.println();
  Serial.println("###########################################################");
  Serial.print("RNG=");
  Serial.print(use_analog_random ? "AnalogRandomRandomBit" : "PseudoRandomBit");
  Serial.print(", nbits=");
  Serial.print(nbits);
  Serial.print(", bucket_max=");
  Serial.println(bucket_max);

  switch (nbits) {
      case 1:
        test_analog_random<uint16_t, 1>(next_bit, bucket_max, 500);
        break;
      case 2:
        test_analog_random<uint16_t, 2>(next_bit, bucket_max, 500);
        break;
      case 3:
        test_analog_random<uint16_t, 3>(next_bit, bucket_max, 500);
        break;
      case 4:
        test_analog_random<uint16_t, 4>(next_bit, bucket_max, 500);
        break;
      case 5:
        test_analog_random<uint16_t, 5>(next_bit, bucket_max, 500);
        break;
      default:
        Serial.print("Don't know how to handle nbits=");
        Serial.println(nbits);
        // do something
  }

  if (!use_analog_random) {
    use_analog_random = true;
  } else {
    use_analog_random = false;
    ++bucket_max_index;
    if (bucket_max_index >= kNumBucketMaxChoices) {
      bucket_max_index = 0;
      ++nbits_index;
      if (nbits_index >= kNumNbitsChoices) {
        nbits_index = 0;
      }
    }
  }
}
