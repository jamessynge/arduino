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
  Serial.begin(57600);
  delay(1000);
  Serial.println("# Start");

  num_register_reads = min_num_register_reads;
}

void loop() {
  if (num_register_reads > max_num_register_reads) {
    num_register_reads = min_num_register_reads;
    Serial.print("# Start over at ");
    Serial.println(jamessynge::ArdTime::Now());
  }

  Serial.print("RecordCounts(");
  Serial.print(num_register_reads);
  Serial.print(", [0x");
  for (int i = 0; i < 20; ++i) {
    if (i > 0) {
      Serial.print(", 0x");
    } 
    Serial.print(JitterRandom::random32(num_register_reads), HEX);
  }
  Serial.println("])");
  ++num_register_reads;
}
