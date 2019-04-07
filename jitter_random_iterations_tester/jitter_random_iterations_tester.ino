// Print the values produced by JitterRandom::random32(N) for a range of
// different values of N (number of TCNT1L values that are hashed to produce
// the desired value).
// The results are formatted as Python calls to a RecordCounts function,
// allowing the output to be copied as is into eval_jitter_random_iterations.py,
// a Python program for evaluating the values.

#include <Arduino.h>
#include <inttypes.h>

#include "jitter_random.h"
#include "debug.h"
#include "test.h"
#include "time.h"

int min_num_register_reads = 1;
int max_num_register_reads = 64;
int num_register_reads;

void setup() {
  Serial.begin(9600);
  delay(1000);
  Serial.println("# Start");

  // We start from the max, which will cause the first row of output to take
  // longer to output. This will make it easier to copy-and-paste the results
  // as whole groups.
  num_register_reads = max_num_register_reads;
}

void loop() {
  bool start_over = false;
  auto start_time = jamessynge::ArdTime::Now();
  if (num_register_reads > max_num_register_reads) {
    num_register_reads = min_num_register_reads;
    start_over = true;
  } else if (num_register_reads < min_num_register_reads) {
    num_register_reads = max_num_register_reads;
    start_over = true;
  }

  // First collect the results, then print them. This makes it easier to copy
  // and paste the results as whole roles from the Arduino IDE Serial Monitor.
  // Of course, it would be better to use another program to read the results
  // from the tty.
  const int kNumValues = 32;
  uint32_t values[kNumValues];
  for (int i = 0; i < kNumValues; ++i) {
    values[i] = JitterRandom::random32(num_register_reads);
  }

  if (start_over) {
    Serial.print("# Started over at ");
    Serial.println(start_time);
  }

  Serial.print("RecordRandom32(");
  Serial.print(num_register_reads);
  Serial.print(", [0x");
  for (int i = 0; i < kNumValues; ++i) {
    if (i > 0) {
      Serial.print(", 0x");
    } 
    Serial.print(values[i], HEX);
  }
  Serial.println("])");
  --num_register_reads;
}
