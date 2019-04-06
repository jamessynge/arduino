#ifndef _JAMES_SYNGE_TIME_H_
#define _JAMES_SYNGE_TIME_H_

// Types to support Ardunio times and durations. Not generalized to long
// durations; an Arduino's 32-bit millisecond granularity clock can only
// record times up to 49.7 days, so this is focused on durations that are
// generally far shorter than that.

#include <Arduino.h>
#include <inttypes.h>

namespace jamessynge {
class ArdDuration;
class ArdTime;

namespace internal {
// Functions for internal use by functions in this file.
unsigned long repr(ArdTime t);
long repr(ArdDuration d);
ArdTime repr_to_ard_time(unsigned long ms);
ArdDuration repr_to_ard_duration(long ms);
}  // namespace internal

// This is used for ArdTime and ArdDuration. The field `negative` is false
// when produced from a ArdTime.
struct ArdTimeParts : Printable {
  ArdTimeParts(unsigned long ms);
  size_t printTo(Print&) const override;
  uint16_t milliseconds;
  uint8_t seconds;
  uint8_t minutes;
  uint8_t hours;
  uint8_t days;
  bool negative;  // false == positive; true == negative;
};

ArdDuration operator-(ArdTime a, ArdTime b);
bool operator>=(ArdTime a, ArdTime b);
ArdTime operator-(ArdTime t, ArdDuration d);
ArdTime operator+(ArdTime t, ArdDuration d);
ArdDuration operator-(ArdDuration a, ArdDuration b);
ArdDuration operator+(ArdDuration a, ArdDuration b);
ArdDuration operator/(ArdDuration dur, long div);
ArdDuration operator*(ArdDuration dur, long mul);
ArdDuration operator/(ArdDuration dur, double div);
ArdDuration operator*(ArdDuration dur, double mul);
bool operator>=(ArdDuration a, ArdDuration b);
bool operator>(ArdDuration a, ArdDuration b);
bool operator<=(ArdDuration a, ArdDuration b);
bool operator<(ArdDuration a, ArdDuration b);
ArdDuration Milliseconds(int ms);
ArdDuration Milliseconds(long ms);
ArdDuration Milliseconds(double ms);
ArdDuration Seconds(int seconds);
ArdDuration Seconds(long seconds);
ArdDuration Seconds(double seconds);
ArdDuration Minutes(int minutes);
ArdDuration Minutes(long minutes);
ArdDuration Minutes(double minutes);
ArdDuration Hours(int hours);
ArdDuration Hours(long hours);
ArdDuration Hours(double hours);

class ArdTime : public Printable {
 public:
  // Start of epoch, which for an Arduino is when it booted or rolled over.
  ArdTime();  
  ArdTime(const ArdTime&) = default;
  ~ArdTime() = default;

  ArdTime& operator=(ArdTime);
  ArdTime& operator+=(ArdDuration);
  ArdTimeParts Split() const;
  size_t printTo(Print&) const override;

  static ArdTime Now();

 private:
  friend unsigned long internal::repr(ArdTime t);
  friend ArdTime internal::repr_to_ard_time(unsigned long ms);

  ArdTime(unsigned long ms);
  unsigned long ms_;
};

class ArdDuration : public Printable {
 public:
  ArdDuration();
  ArdDuration(const ArdDuration&) = default;
  ArdDuration operator=(const ArdDuration&);
  ~ArdDuration() = default;

  ArdTimeParts Split() const;
  size_t printTo(Print&) const override;

 private:
  friend long internal::repr(ArdDuration d);
  friend ArdDuration internal::repr_to_ard_duration(long ms);

  ArdDuration(long ms);
  long ms_;
};

}  // namespace jamessynge

#endif  // _JAMES_SYNGE_TIME_H_