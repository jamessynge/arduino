#include "SensorAndLED.h"
#include <EEPROM.h>

#define CALIBRATION_PERIOD 500  // milliseconds
#define CALIBRATION_BLINK_PERIOD 100
#define ANNOUNCE_INTERVAL 500

void SensorAndLED::init() {
  pinMode(sensor_pin, INPUT);
  pinMode(led_pin, OUTPUT);
  next_toggle = 0;
  toggle_period = 0;
  threshold = 1024; // To avoid being triggered before ready.
#if DEBUG
  next_announce = 0;
#endif
  ledOff();
}

void SensorAndLED::ledOff() {
  digitalWrite(led_pin, LOW);
  led_state = false;
}

void SensorAndLED::ledOn() {
  digitalWrite(led_pin, HIGH);
  led_state = true;
}

void SensorAndLED::toggleLed() {
  if (led_state) {
    ledOff();
  } else {
    ledOn();
  }
}

void SensorAndLED::startBlinking(const long now_millis, const int toggle_period) {
  if (this->toggle_period == toggle_period) {
    return;
  }
  ledOn();
  this->toggle_period = toggle_period;
  this->next_toggle += toggle_period;
}

void SensorAndLED::stopBlinking() {
  ledOff();
  toggle_period = 0;
  next_toggle = 0;
}

void SensorAndLED::updateLed(const long now_millis) {
  if (toggle_period) {
    if (next_toggle <= now_millis) {
      toggleLed();
      next_toggle += toggle_period;
    }
  }
}

void SensorAndLED::calibrate() {
  threshold = 0;

  unsigned long now = millis();
  const unsigned long end_millis = now + CALIBRATION_PERIOD;

  startBlinking(now, CALIBRATION_BLINK_PERIOD);

  while (true) {
    threshold = max(threshold, analogRead(sensor_pin));
    now = millis();
    if (now >= end_millis) {
      break;
    }
    updateLed(now);
  }

  stopBlinking();
}

boolean SensorAndLED::isSensorTriggered(const int num_reads, const int tolerance) {
  const int value = readSensor(num_reads);
  const int adjusted_threshold = threshold + tolerance;
#if DEBUG
  unsigned long now = millis();
  if (next_announce == 0) {
    next_announce = now + ANNOUNCE_INTERVAL;
  } else if (now >= next_announce) {
    DLOG("isSensorTriggered '%c': value=%d, margin=%d\n", tag, value, adjusted_threshold - value);
    next_announce += ANNOUNCE_INTERVAL;
  }
#endif
  return value >= adjusted_threshold;
}

int SensorAndLED::readSensor(int num_reads) const {
  int value = 1023;
  do {
    value = min(value, analogRead(sensor_pin));
  } while (--num_reads > 0);
  return value;
}

int SensorAndLED::writeThreshold(int addr) const {
  DLOG("writeThreshold(%d) '%c' threshold %d\n", addr, tag, threshold);
  EEPROM.write(addr++, tag);
  EEPROM.write(addr++, threshold >> 8);
  EEPROM.write(addr++, threshold & 0xff);
  return addr;
}

// Read the threshold for this sensor (Tag Byte, High Byte, Low Byte).
// Returns the addr beyond the threshold if the data is value, or -1 if not.
int SensorAndLED::readThreshold(int addr) {
  DLOG("readThreshold(%d)\n", addr);
  threshold = 1024;  // Prevent alerts when unable to read.
  if (EEPROM.read(addr++) == tag) {
    int value = EEPROM.read(addr++) << 8;
    value |= EEPROM.read(addr++);
    if (0 < value && value < 1023) {
      threshold = value;
      DLOG("readThreshold(%d) '%c' -> %d\n", addr, tag, threshold);
      return addr + 3;
    }
  }
  DLOG("readThreshold(%d) -> INVALID\n", addr);
  return -1;
}

