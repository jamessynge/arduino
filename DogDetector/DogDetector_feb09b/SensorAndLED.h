#ifndef _SENSOR_AND_LED_H_
#define _SENSOR_AND_LED_H_

#include <stdint.h>

#include "misc.h"

struct SensorReading {
  // Is it currently triggered, or was it triggered in the last tolerance_ms milliseconds?
  bool isTriggered(uint16_t tolerance_ms) const {
    return is_triggered || is_changed || (!is_changed && duration_ms < tolerance_ms);
  }
  
  bool is_triggered;
  bool is_changed;
  // If is_changed, duration_ms is the duration of the last is_triggered state.
  // If !is_changed, duration_ms is the duration of the current is_triggered state.
  // After 65.535 seconds, value is pegged at 0xffff.
  uint16_t duration_ms;
};

class SensorAndLED {
 public:
  SensorAndLED(const char sensor_pin, const char led_pin, const char tag)
      : sensor_pin(sensor_pin),
        led_pin(led_pin),
        tag(tag) {
    init();
  }

  void init();

  void ledOff();

  void ledOn();
  void toggleLed();
  void startBlinking(const long now_millis, const int toggle_period);
  void stopBlinking();
  void updateLed(const long now_millis);
  void calibrate();
  SensorReading readSensor(const long now_ms, const int num_reads, const int tolerance);
  int readSensor(int num_reads) const;

  // Write the threshold for this sensor (Tag Byte, High Byte, Low Byte)
  // to EEPROM starting at addr.  Doesn't check that the value is valid.
  int writeThreshold(int addr) const;

  // Read the threshold for this sensor (Tag Byte, High Byte, Low Byte)
  // from the EEPROM at addr. Returns the addr beyond the threshold if
  // the EEPROM is valid, or -1 if not.
  int readThreshold(int addr);

 private:
  unsigned long next_toggle;
  unsigned long last_transition;
#if DEBUG
  unsigned long next_announce;
#endif
  int toggle_period;
  int threshold;
  bool led_state;
  bool last_state;
  const char sensor_pin;
  const char led_pin;
  const char tag;
};

#endif  // _SENSOR_AND_LED_H_

