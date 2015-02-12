#ifndef _SENSOR_AND_LED_H_
#define _SENSOR_AND_LED_H_

#include <stdint.h>

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
  bool isSensorTriggered(const int num_reads, const int tolerance);
  int readSensor(int num_reads) const;

  // Write the threshold for this sensor (Tag Byte, High Byte, Low Byte)
  // to EEPROM starting at addr.
  int writeThreshold(int addr) const;

  // Read the threshold for this sensor (Tag Byte, High Byte, Low Byte).
  // Returns the addr beyond the threshold if the data is value, or -1 if not.
  int readThreshold(int addr);

 private:
  unsigned long next_toggle;
#if DEBUG
  unsigned long next_announce;
#endif
  int toggle_period;
  int threshold;
  bool led_state;
  const char sensor_pin;
  const char led_pin;
  const char tag;
};

#endif  // _SENSOR_AND_LED_H_

