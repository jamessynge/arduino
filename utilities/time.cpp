#include "time.h"

namespace jamessynge {
namespace internal {
// Functions for internal use by functions in this file.
unsigned long repr(ArdTime t) {
  return t.ms_;
}
long repr(ArdDuration d) {
  return d.ms_;
}

ArdTime repr_to_ard_time(unsigned long ms) {
  return ArdTime(ms);
}
ArdDuration repr_to_ard_duration(long ms) {
  return ArdDuration(ms);
}

}  // namespace internal

using internal::repr;
using internal::repr_to_ard_time;
using internal::repr_to_ard_duration;

namespace {
size_t printWithLeadingZeros(Print& p, unsigned long value, int min_width) {
  size_t result = 0;
  if (value >= 10) {
    result += printWithLeadingZeros(p, value / 10, min_width - 1);
    value = value % 10;
    min_width = 0;
  }
  while (min_width > 1) {
    result += p.print('0');
    --min_width;
  }
  return result + p.print(value, DEC);
}
}  // namespace

ArdTimeParts::ArdTimeParts(unsigned long ms) {
  negative = false;
  milliseconds = ms % 1000;
  ms /= 1000;
  seconds = ms % 60;
  ms /= 60;
  minutes = ms % 60;
  ms /= 60;
  hours = ms % 24;
  ms /= 24;
  days = ms;
};
size_t ArdTimeParts::printTo(Print& p) const {
  size_t result = 0;
  if (negative) {
    result += p.print('-');
  }
  bool first = true;
  if (days > 0) {
    first = false;
    result += p.print(days, DEC);
    result += p.print("d ");
  }
  if (!first || hours > 0) {
    first = false;
    result += printWithLeadingZeros(p, hours, 2);
    result += p.print(":");
  }
  result += printWithLeadingZeros(p, minutes, 2);
  result += p.print(":");
  result += printWithLeadingZeros(p, seconds, 2);
  result += p.print(".");
  result += printWithLeadingZeros(p, milliseconds, 3);
  return result;
}

ArdTime::ArdTime() : ms_(0) {}
ArdTime::ArdTime(unsigned long ms) : ms_(ms) {}
ArdTime& ArdTime::operator=(ArdTime other) {
  ms_ = other.ms_;
  return this;
}
ArdTime& ArdTime::operator+=(ArdDuration d) {
  ms_ += repr(d);
  return this;
}
ArdTimeParts ArdTime::Split() const {
  return ArdTimeParts(ms_);
}
// static
ArdTime ArdTime::Now() {
  return ArdTime(millis());
}
size_t ArdTime::printTo(Print& p) const {
  return Split().printTo(p);
}

ArdDuration::ArdDuration() : ms_(0) {}
ArdDuration::ArdDuration(long ms) : ms_(ms) {}
ArdDuration ArdDuration::operator=(const ArdDuration& other) {
  ms_ = other.ms_;
}
ArdTimeParts ArdDuration::Split() const {
  if (ms_ >= 0) {
    return ArdTimeParts(ms_);
  }
  ArdTimeParts p(-ms_);
  p.negative = true;
  return p;
}
size_t ArdDuration::printTo(Print& p) const {
  return Split().printTo(p);
}

ArdDuration operator-(ArdTime a, ArdTime b) {
  auto a_ms = repr(a);
  auto b_ms = repr(b);
  if (a_ms >= b_ms) {
    return repr_to_ard_duration(a_ms - b_ms);
  } else {
    return repr_to_ard_duration(-(b_ms - a_ms));
  }
}
bool operator>=(ArdTime a, ArdTime b) {
  return repr(a) >= repr(b);
}
ArdTime operator-(ArdTime t, ArdDuration d) {
  auto t_ms = repr(t);
  auto d_ms = repr(d);
  return repr_to_ard_time(t_ms - d_ms);
}
ArdTime operator+(ArdTime t, ArdDuration d) {
  auto t_ms = repr(t);
  auto d_ms = repr(d);
  return repr_to_ard_time(t_ms + d_ms);
}
ArdDuration operator-(ArdDuration a, ArdDuration b) {
  auto a_ms = repr(a);
  auto b_ms = repr(b);
  return repr_to_ard_duration(a_ms - b_ms);
}
ArdDuration operator+(ArdDuration a, ArdDuration b) {
  auto a_ms = repr(a);
  auto b_ms = repr(b);
  return repr_to_ard_duration(a_ms + b_ms);
}
ArdDuration operator/(ArdDuration dur, long div) {
  auto dur_ms = repr(dur);
  return repr_to_ard_duration(dur_ms / div);
}
ArdDuration operator*(ArdDuration dur, long mul) {
  auto dur_ms = repr(dur);
  return repr_to_ard_duration(dur_ms * mul);
}
ArdDuration operator/(ArdDuration dur, double div) {
  auto dur_ms = repr(dur);
  return repr_to_ard_duration(dur_ms / div);
}
ArdDuration operator*(ArdDuration dur, double mul) {
  auto dur_ms = repr(dur);
  return repr_to_ard_duration(dur_ms * mul);
}
bool operator>=(ArdDuration a, ArdDuration b) {
  auto a_ms = repr(a);
  auto b_ms = repr(b);
  return a_ms >= b_ms;
}
bool operator>(ArdDuration a, ArdDuration b) {
  auto a_ms = repr(a);
  auto b_ms = repr(b);
  return a_ms > b_ms;
}
bool operator<=(ArdDuration a, ArdDuration b) {
  auto a_ms = repr(a);
  auto b_ms = repr(b);
  return a_ms <= b_ms;
}
bool operator<(ArdDuration a, ArdDuration b) {
  auto a_ms = repr(a);
  auto b_ms = repr(b);
  return a_ms < b_ms;
}

ArdDuration Milliseconds(long ms) {
  return repr_to_ard_duration(ms);
}
ArdDuration Milliseconds(int ms) {
  return repr_to_ard_duration(static_cast<long>(ms));
}
ArdDuration Milliseconds(double ms) {
  return repr_to_ard_duration(static_cast<long>(round(ms)));
}
ArdDuration Seconds(long seconds) {
  return Milliseconds(seconds * 1000);
}
ArdDuration Seconds(int seconds) {
  return Milliseconds(seconds * 1000L);
}
ArdDuration Seconds(double seconds) {
  return Milliseconds(seconds * 1000);
}
ArdDuration Minutes(int minutes) {
  return Seconds(minutes * 60L);
}
ArdDuration Minutes(long minutes) {
  return Seconds(minutes * 60);
}
ArdDuration Minutes(double minutes) {
  return Seconds(minutes * 60);
}
ArdDuration Hours(int hours) {
  return Minutes(hours * 60L);
}
ArdDuration Hours(double hours) {
  return Minutes(hours * 60);
}

}  // namespace jamessynge
