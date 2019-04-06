#include <Arduino.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
//#include <util/atomic.h>

#include "time.h"

using jamessynge::ArdTime;
using jamessynge::ArdDuration;
using jamessynge::ArdTimeParts;
using jamessynge::Milliseconds;
using jamessynge::Seconds;
using jamessynge::Minutes;
using jamessynge::Hours;

volatile uint32_t num_interrupts_remaining = 0;
volatile uint32_t tcnt1l_counts[256];

// Interrupt service routine, records the jitter in TCNT1L.
ISR(WDT_vect)
{
  if (num_interrupts_remaining > 0) {
    ++tcnt1l_counts[TCNT1L];
    --num_interrupts_remaining;
  }
}

class DurationEstimator {
 public:
  DurationEstimator(ArdDuration elapsed, uint32_t completed_operations)
    : elapsed_(elapsed), completed_operations_(completed_operations) {}

  DurationEstimator operator=(const DurationEstimator& other) {
    elapsed_ = other.elapsed_;
    completed_operations_ = other.completed_operations_;
  }

  ArdDuration estimate_duration_for(uint32_t operations_to_go) const {
    double ratio = double(operations_to_go) / double(completed_operations_);
    return elapsed_ * ratio;
  }

  uint32_t completed_operations() const { return completed_operations_; }

 private:
  ArdDuration elapsed_;
  uint32_t completed_operations_;
};

ArdTime start_counting_time;
ArdTime next_heartbeat_time;

// Basis for initial estimates, from a prior run.
DurationEstimator duration_estimator(Seconds(261), 16384UL);

// Number of samples used to compute estimated_watchdog_duration:
uint32_t estimated_watchdog_duration_basis_samples = 1;
ArdDuration last_duration = Milliseconds(16);

uint32_t start_time_ms = 0;
uint32_t estimated_wd_time_ms = 0;

ArdDuration estimate_duration(ArdDuration elapsed_dur,
                              uint32_t collected_samples,
                              uint32_t samples_to_go) {
  double ratio = double(samples_to_go) / double(collected_samples);
  return elapsed_dur * ratio;
}

ArdDuration choose_next_heartbeat_interval(
    ArdDuration estimated_remaining_dur) {
  ArdDuration next_interval;
  if (estimated_remaining_dur >= Hours(12)) {
    next_interval = Hours(6);
  } else if (estimated_remaining_dur >= Minutes(90)) {
    next_interval = Hours(1);
  } else if (estimated_remaining_dur >= Minutes(30)) {
    next_interval = Minutes(20);
  } else {
    next_interval = Minutes(5);
  }
  return next_interval;
}

void print_heart_beat(ArdTime now, uint32_t total_samples,
                      uint32_t samples_to_go) {
  auto collected_samples = total_samples - samples_to_go;
  auto elapsed_dur = now - start_counting_time;

  // If we've done enough work, prefer a new basis for estimates.
  if (collected_samples * 10 >= duration_estimator.completed_operations()) {
    duration_estimator = DurationEstimator(elapsed_dur, collected_samples);
  }

  auto estimated_remaining_dur =
      duration_estimator.estimate_duration_for(samples_to_go);

  Serial.print("# Counted ");
  Serial.print(collected_samples);
  Serial.print(" samples over period ");
  Serial.print(elapsed_dur);
  if (samples_to_go > 0) {
    Serial.print("; ");
    Serial.print(samples_to_go);
    Serial.print(" to go; estimated time remaining: ");
    Serial.print(estimated_remaining_dur);
  }
  Serial.println();

  next_heartbeat_time += choose_next_heartbeat_interval(estimated_remaining_dur);
}

void start_counting(uint32_t limit) {
  Serial.println("###########################################################");
  Serial.print("# Time since boot/rollover: ");
  Serial.println(ArdTime::Now());
  Serial.print("# Counting ");
  Serial.print(limit);
  Serial.println(" TCNT1L values");

  if (estimated_wd_time_ms > 0) {
    auto seconds = (limit * estimated_wd_time_ms + 500UL) / 1000UL;
    Serial.print("# Estimated seconds to compute: ");
    Serial.println(seconds);
  }
  auto estimated_total_dur =
      duration_estimator.estimate_duration_for(limit);
  Serial.print("# Estimated time to compute: ");
  Serial.println(estimated_total_dur);

  // Reset state used by the interrupt.
  num_interrupts_remaining = limit;
  for (int i = 0; i < 256; ++i) {
    tcnt1l_counts[i] = 0;
  }

  // Allow time for Serial to flush.
  delay(50);
  start_time_ms = millis();
  start_counting_time = ArdTime::Now();
  next_heartbeat_time = start_counting_time +
                        choose_next_heartbeat_interval(estimated_total_dur);

  // The following five lines of code turn on the watch dog timer interrupt,
  // triggering the calling of the interrupt service routine.
  cli();
  MCUSR = 0;
  _WD_CONTROL_REG |= (1<<_WD_CHANGE_BIT) | (1<<WDE);
  _WD_CONTROL_REG = (1<<WDIE);
  sei();
}

void stop_and_report_counts(uint32_t limit) {
  uint32_t elapsed_ms = millis() - start_time_ms;
  auto end_time = ArdTime::Now();

  // The following five lines turn off the watch dog timer interrupt
  cli();
  MCUSR = 0;
  _WD_CONTROL_REG |= (1<<_WD_CHANGE_BIT) | (0<<WDE);
  _WD_CONTROL_REG = (0<< WDIE);
  sei();

  Serial.print("# Time since boot/rollover: ");
  Serial.println(ArdTime::Now());
  print_heart_beat(end_time, limit, 0);

  Serial.print("# Elapsed seconds: ");
  Serial.println((elapsed_ms + 500UL) / 1000UL);
  // Round up when estimating the time it will take.
  estimated_wd_time_ms = (elapsed_ms + limit - 1) / limit;

  // Output in a form that can be copied into python.
  Serial.print("# Reporting population counts for ");
  Serial.print(limit);
  Serial.println(" reads from TCNT1L.");
  Serial.print("RecordCounts(");
  Serial.print(limit);
  Serial.print(", [");
  for (int i = 0; i < 256; ++i) {
    if (i % 16 == 0) {
      Serial.println();
      Serial.print("    ");
    }
    Serial.print(tcnt1l_counts[i]);
    Serial.print(", ");
  }
  Serial.println();
  Serial.println("])");
  Serial.println();
}

uint32_t current_limit = 0;

void setup() {
  Serial.begin(9600);

//  current_limit = 2048;
  current_limit = 16777216;
  // current_limit = 32;
  start_counting(current_limit);
}

void loop() {
  if (num_interrupts_remaining > 0) {
    auto now = ArdTime::Now();
    if (now >= next_heartbeat_time) {
      print_heart_beat(now, current_limit, num_interrupts_remaining);
    }
    return;
  }

  stop_and_report_counts(current_limit);
  current_limit *= 2;
  start_counting(current_limit);
}
