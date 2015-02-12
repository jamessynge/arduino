/*
Inputs:
A0 - Low IR Sensor (GP2Y0A02YK, smoothed with RC circuit)
A5 - High IR Sensor (GP2Y0A02YK, smoothed with RC circuit)
D2 - Pushbutton, for triggering calibration.

Outputs:

D4 - Trigger Relay Shield so that Dog Repeller/No Bark device emits sound
D6 - PWM Output to piezoelectric buzzer
D7 - Low IR Sensor detecting something now
D12 - High IR Sensor detected something recently

EEPROM:

Using memory to remember calibration values of the two sensors, i.e. their
voltages (which won't be the same) when nothing is present.  An increase
in voltage of more than X% will be deemed evidence of something being present.

RAM:

latest low and high sensor readings.
was calibration button pressed; set in ISR, cleared at end of calibration().

-------------

Three basic modes of operation:

1) Normal:
   setup() finds reasonable values in EEPROM, exits so that loop can run.
   loop() read inputs to detect movement, and check for calibration button.

2) setup() does not find reasonable values in EEPROM, runs calibration before
   exiting, which puts the circuit into normal mode.

3) calibration() flashes an led slowly to indicate that it is preparing to
   calibrate (gives user time to get out of the way), then flashes led quickly
   while taking lots of measurements from sensors (over the course of several
   seconds).  Higher voltages indicate closer, so choose something like a voltage
   such that 95% of readings are lower.  Store the two voltages in EEPROM for
   the next time the program starts.  After calibration, reset detection timers.

-------------

loop() 

*/

#include <EEPROM.h>
extern void serialPrintf(const char *fmt, ...);

#define DEBUG true
#if DEBUG
#define DLOG(...) serialPrintf(__VA_ARGS__)
#else
#define DLOG(...) 0
#endif

const int LOW_SENSOR_PIN = 0;   // Analog pin 0, GP2Y0A02YK, smoothed with RC circuit
const int HIGH_SENSOR_PIN = 5;  // Analog pin 5
const int CALIBRATION_INT = 0;  // Pusbutton on D2, Arduino interrupt 0

const int RELAY_PIN = 4;
const int BUZZER_PWM_PIN = 6;
const int LOW_LED_PIN = 7;
const int HIGH_LED_PIN = 12;

// Variables initialized in setup, not changed after that,
// hence capitalized like constants.
const int THRESHOLD_OFFSET = 20;  // 3.3V / 1024 * 20 = 64mV
int LOW_SENSOR_THRESHOLD;
int HIGH_SENSOR_THRESHOLD;
boolean IS_CALIBRATED;

int readSensorCalibration(boolean read_low) {
  DLOG("readSensorCalibration(%d)\n", read_low);

  const int addr0 = read_low ? 4 : 7;
  int value = EEPROM.read(addr0) << 8 |  EEPROM.read(addr0 + 1);
  if (value < 0 || value >= 1024) {
    value = 0;
  }
  DLOG("readSensorCalibration(%d) -> %d\n", read_low, value);
  return value;
}

boolean isEEPROMInitialized() {
  boolean result =
      EEPROM.read(0) == 'D' &&
      EEPROM.read(1) == 'o' &&
      EEPROM.read(2) == 'g' &&
      EEPROM.read(3) == 'L' &&
      EEPROM.read(6) == 'H' &&
      EEPROM.read(9) == 0;
  DLOG("isEEPROMInitialized() -> %d\n", result);
  return result;
}

volatile boolean do_calibrate = false;
boolean low_led = false;
boolean high_led = false;
  
void calibrationButtonPressed() {
  do_calibrate = true;
}

void updateLedPins() {
  digitalWrite(LOW_LED_PIN, low_led);
  digitalWrite(HIGH_LED_PIN, high_led);
}

// calibrate never returns, triggers a reboot.
void calibrate() {
  DLOG("calibration()\n");

  // Clear the EEPROM bytes we use.
  for (int addr = 0; addr < 10; ++addr) {
    EEPROM.write(addr, 0);
  }

  // Wait for a bit (5 seconds) to allow the user to get out of the way.
  // Toggle the two leds.
  low_led = !high_led;
  for (int ndx = 0; ndx < 10; ++ndx) {
    low_led = !low_led;
    high_led = !high_led;
    updateLedPins();
    delay(500);
  }

  // Now get the maximum voltage we see over 2 seconds.
  // Blink the high LED fast.
  DLOG("Finding current maximum values\n");
  low_led = false;
  high_led = true;
  updateLedPins();

  const unsigned long end_millis = millis() + 2000;
  unsigned long blink_millis = millis() + 100;
  int low_value = 0;
  int high_value = 0;

  while (true) {
    low_value = max(low_value, analogRead(LOW_SENSOR_PIN));
    high_value = max(high_value, analogRead(HIGH_SENSOR_PIN));
    const int current_millis = millis();
    if (current_millis >= end_millis) {
      break;
    } else if (current_millis > blink_millis) {
      blink_millis += 100;
      high_led = !high_led;
      updateLedPins();
    }
  }

  // Remember the maximum values.
  DLOG("Writing to EEPROM: %d, %d\n", low_value, high_value);
  EEPROM.write(4, low_value >> 8);
  EEPROM.write(5, low_value & 0xff);
  EEPROM.write(7, high_value >> 8);
  EEPROM.write(8, high_value & 0xff);
  EEPROM.write(0, 'D');
  EEPROM.write(1, 'o');
  EEPROM.write(2, 'g');
  EEPROM.write(3, 'L');
  EEPROM.write(6, 'H');
  EEPROM.write(9, 0);

  do_calibrate = false;
  DLOG("reboot()\n");
  reboot();
}

void setup() {
  DLOG("setup() entry\n");

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PWM_PIN, OUTPUT);
  pinMode(LOW_LED_PIN, OUTPUT);
  pinMode(HIGH_LED_PIN, OUTPUT);

  pinMode(CALIBRATION_INT, INPUT);

  // Turn on both LEDs, then turn off when ready to run.
  low_led = true;
  high_led = true;
  updateLedPins();

  // Assumes hooked up to 3.3v.
  analogReference(EXTERNAL);

  // First analogRead is a bit slower, so do it now.
  analogRead(LOW_SENSOR_PIN);
  analogRead(HIGH_SENSOR_PIN);

  // Get the thresholds for the two sensors.
  LOW_SENSOR_THRESHOLD = readSensorCalibration(true);
  HIGH_SENSOR_THRESHOLD = readSensorCalibration(false);
  IS_CALIBRATED = isEEPROMInitialized() &&
    0 < LOW_SENSOR_THRESHOLD && LOW_SENSOR_THRESHOLD < 1024 &&
    0 < LOW_SENSOR_THRESHOLD && LOW_SENSOR_THRESHOLD < 1024;

  if (!IS_CALIBRATED) {
    calibrate();
  }

  LOW_SENSOR_THRESHOLD += THRESHOLD_OFFSET;
  HIGH_SENSOR_THRESHOLD += THRESHOLD_OFFSET;
  DLOG("Thresholds: low=%d, high=%d\n", LOW_SENSOR_THRESHOLD, HIGH_SENSOR_THRESHOLD);

  // Prepare to loop.
  attachInterrupt(CALIBRATION_INT, calibrationButtonPressed, FALLING);
  delay(50);
  do_calibrate = false;

  low_led = false;
  high_led = false;
  updateLedPins();

  DLOG("setup() exit\n");
}

// Disable for 5 seconds after high sensor has triggered (i.e. a person has passed).
const unsigned long DISABLE_PERIOD = 3000;
const unsigned long ALERT_PERIOD = 1500;
const unsigned long ALERT_DELAY_PERIOD = 500;

unsigned long ignore_until = 0;
unsigned long activate_after = 0;
unsigned long deactivate_after = 0;

void activateAlert() {
  if (deactivate_after) {
    // Already activated.
    return;
  }
  activate_after = 0;
  deactivate_after = millis() + ALERT_PERIOD;
  
  DLOG("activateAlert()\n");
  
  analogWrite(BUZZER_PWM_PIN, 127);
  // TODO
}

void deactivateAlert() {
  DLOG("deactivateAlert()\n");
  deactivate_after = 0;

  analogWrite(BUZZER_PWM_PIN, 0);
  // TODO
}

const int NUM_ANALOG_SAMPLES = 10;

void loop() {
  if (do_calibrate) {
    DLOG("calibration button pressed\n");
    calibrate();  // Never returns.
  }
  // Read the sensors.
  int low_value = 1024;
  int high_value = 1024;
  for (int ndx = 0; ndx < NUM_ANALOG_SAMPLES; ++ndx) {
    low_value = min(low_value, analogRead(LOW_SENSOR_PIN));
    high_value = min(high_value, analogRead(HIGH_SENSOR_PIN));
  }
  // Has the high sensor been triggered?
  const unsigned long current_millis = millis();
#if DEBUG
  #define ANNOUNCE_INTERVAL 500
  static unsigned long next_announce = 0;
  if (next_announce == 0) {
    next_announce = current_millis + ANNOUNCE_INTERVAL;
  } else if (current_millis >= next_announce) {
    DLOG("loop: low=%3d, high=%3d", low_value, high_value);
    DLOG("  margins: low=%3d, high=%3d\n", LOW_SENSOR_THRESHOLD-low_value, HIGH_SENSOR_THRESHOLD-high_value);
    next_announce += ANNOUNCE_INTERVAL;
  }
#endif
  if (high_value > HIGH_SENSOR_THRESHOLD) {
    // Yes.
    if (ignore_until == 0) {
      DLOG("High Sensor Triggered (%d > %d)\n", high_value, HIGH_SENSOR_THRESHOLD);
    }
    high_led = true;
    low_led = false;
    ignore_until = current_millis + DISABLE_PERIOD;
    deactivateAlert();
    activate_after = 0;
    deactivate_after = 0;
  } else if (ignore_until > current_millis) {
    // TODO What?
  } else {
    if (ignore_until) {
      DLOG("Ignore Low Sensor Period Expired\n");
    }
    ignore_until = 0;
    high_led = false;
    if (low_value > LOW_SENSOR_THRESHOLD) {
      low_led = true;
      if (deactivate_after > 0) {
        // Continue the current alert.
        deactivate_after = current_millis + ALERT_PERIOD;
      } else if (activate_after > 0) {
        // No change in when we trigger.
      } else {
        DLOG("Low Sensor Triggered (%d > %d)\n", low_value, LOW_SENSOR_THRESHOLD);
        activate_after = current_millis + ALERT_DELAY_PERIOD;
      }
    } else if (!deactivate_after && !activate_after) {
      if (current_millis > 4000000000L) {
        DLOG("Millisecond clock will soon wrap around; rebooting\n");
        // millis will wrap around soon, so reboot to restart in a controlled fashion.
        reboot();
      }
    }
  }

  if (current_millis >= activate_after && activate_after > 0) {
    activateAlert();
  } else if (current_millis >= deactivate_after && deactivate_after > 0) {
    deactivateAlert();
    low_led = false;
  }

  updateLedPins();

  delay(20);
}

