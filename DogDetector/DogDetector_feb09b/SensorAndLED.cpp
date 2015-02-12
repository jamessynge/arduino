#include "SensorAndLED.h"

#include <EEPROM.h>
#include <Arduino.h>

#include "misc.h"

#define CALIBRATION_PERIOD 500  // milliseconds
#define CALIBRATION_BLINK_PERIOD 100
#define ANNOUNCE_INTERVAL 2000

void SensorAndLED::init() {
  pinMode(sensor_pin, INPUT);
  pinMode(led_pin, OUTPUT);

  next_toggle = 0;
  last_transition = 0;
  toggle_period = 0;
  threshold = 1024; // To avoid being triggered before ready.
  last_state = false;

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
  if (toggle_period) {
    ledOff();
    toggle_period = 0;
    next_toggle = 0;
  }
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

  DLOG("calibrate sensor '%c', pin %d -> %d\n", tag, sensor_pin, threshold);
}

SensorReading SensorAndLED::readSensor(const long now_ms, const int num_reads, const int tolerance) {
  const int value = readSensor(num_reads);
  const int adjusted_threshold = threshold + tolerance;
  SensorReading result;
  result.is_triggered = value >= adjusted_threshold;
  result.is_changed = result.is_triggered != last_state;
  unsigned long duration_ms = min(now_ms - last_transition, 0xffff);
  result.duration_ms = static_cast<uint16_t>(duration_ms);
  if (result.is_changed) {
    last_state = result.is_triggered;
    last_transition = now_ms;
  }

#if DEBUG
  unsigned long now = millis();
  if (next_announce == 0) {
    next_announce = now + ANNOUNCE_INTERVAL;
  } else if (now >= next_announce || result.is_changed) {
    DLOG("isSensorTriggered '%c': pin=%d, value=%d, margin=%d, is_triggered=%d, is_changed=%d, duration_ms=%u\n",
         tag, sensor_pin, value, adjusted_threshold - value, result.is_triggered, result.is_changed, result.duration_ms);
    next_announce += ANNOUNCE_INTERVAL;
  }
#endif
  return result;
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
int SensorAndLED::readThreshold(const int addr) {
  DLOG("readThreshold(%d) for tag '%c'\n", addr, tag);
  threshold = 1024;  // Prevent alerts when unable to read.
  char saved_tag = EEPROM.read(addr);
  if (saved_tag == tag) {
    int value = EEPROM.read(addr+1) << 8;
    value |= EEPROM.read(addr+2);
    if (0 < value && value < 1023) {
      threshold = value;
      DLOG("readThreshold(%d) '%c' -> %d\n", addr, tag, threshold);
      return addr+3;
    }
  }
  DLOG("readThreshold(%d) -> INVALID (saved_tag '%c')\n", addr, saved_tag);
  return -1;
}

