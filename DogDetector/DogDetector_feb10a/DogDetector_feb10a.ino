/*
Inputs:
A0 - Low IR Sensor (GP2Y0A02YK, smoothed with RC circuit)
A2 - Medium IR Sensor (GP2Y0A02YK, smoothed with RC circuit)
A5 - High IR Sensor (GP2Y0A02YK, smoothed with RC circuit)
D2 - Pushbutton, for triggering calibration.

Outputs:

D4 - Trigger Relay Shield so that Dog Repeller/No Bark device emits sound
D6 - PWM Output to piezoelectric buzzer
D7 - Low IR Sensor detecting something now/Dog moving up
D10 - Medium IR Sensor detecting something now/Dog moving down
D12 - High IR Sensor detected something recently/Person moving

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
#include "misc.h"
#include "SensorAndLED.h"

const int LOW_SENSOR_PIN = 0;    // Analog pin 0, GP2Y0A02YK, smoothed with RC circuit
const int MEDIUM_SENSOR_PIN = 2; // Analog pin 2, GP2Y0A02YK, smoothed with RC circuit
const int HIGH_SENSOR_PIN = 5;   // Analog pin 5, GP2Y0A02YK, smoothed with RC circuit
const int CALIBRATION_INT = 0;   // Pusbutton on D2, Arduino interrupt 0

const int RELAY_PIN = 4;
const int BUZZER_PWM_PIN = 6;
const int LOW_LED_PIN = 7;
const int MEDIUM_LED_PIN = 7;
const int HIGH_LED_PIN = 12;

const int LOW_CALIBRATION_ADDR = 4;
const int MEDIUM_CALIBRATION_ADDR = 7;
const int HIGH_CALIBRATION_ADDR = 10;

const int CALIBRATION_PERIOD = 500;  // milliseconds
const int CALIBRATION_BLINK_PERIOD = 100;

// Disable for 5 seconds after high sensor has triggered (i.e. a person has passed).
const unsigned long DISABLE_PERIOD = 3000;
const unsigned long ALERT_PERIOD = 1500;
const unsigned long ALERT_DELAY_PERIOD = 500;

const int NUM_ANALOG_SAMPLES = 10;
const int THRESHOLD_OFFSET = 20;  // 3.3V / 1024 * 20 = 64mV

#define ANNOUNCE_INTERVAL 500

SensorAndLED low_sensor(LOW_SENSOR_PIN, LOW_LED_PIN, 'L');
SensorAndLED medium_sensor(MEDIUM_SENSOR_PIN, MEDIUM_LED_PIN, 'M');
SensorAndLED high_sensor(HIGH_SENSOR_PIN, HIGH_LED_PIN, 'H');

boolean initializeFromEEPROM() {
  if (EEPROM.read(0) != 'D' ||
      EEPROM.read(1) != 'o' ||
      EEPROM.read(2) != 'g') {
    return false;
  }
  int addr = 3;

  addr = low_sensor.readThreshold(addr);
  if (addr < 0) return false;

  addr = medium_sensor.readThreshold(addr);
  if (addr < 0) return false;

  addr = high_sensor.readThreshold(addr);
  if (addr < 0) return false;

  return EEPROM.read(addr) == 0;
}

void saveToEEPROM() {
  DLOG("saveToEEPROM\n");
  int addr = 0;
  EEPROM.write(addr++, 'X');  // Mark as invalid until done.
  EEPROM.write(addr++, 'o');
  EEPROM.write(addr++, 'g');
  addr = low_sensor.writeThreshold(addr);
  addr = medium_sensor.writeThreshold(addr);
  addr = high_sensor.writeThreshold(addr);
  EEPROM.write(addr++, 0);
  EEPROM.write(0, 'D');       // Finally valid.
}

void initSensorsAndLEDs() {
  low_sensor.init();
  medium_sensor.init();
  high_sensor.init();
}

volatile boolean do_calibrate = false;
  
void calibrationButtonPressed() {
  do_calibrate = true;
}

unsigned long ignore_until = 0;
unsigned long activate_after = 0;
unsigned long deactivate_after = 0;

void activateAlert() {
  if (deactivate_after) {
    // Already activated.
    deactivate_after = millis() + ALERT_PERIOD;
    return;
  }
  activate_after = 0;
  deactivate_after = millis() + ALERT_PERIOD;

  DLOG("activateAlert()\n");

  // Make an audible sound using the buzzer and turn on the relay so that the
  // (possibly) connected dog repeller will emit an loud ultrasonic noise.
  digitalWrite(RELAY_PIN, 1);
  pinMode(BUZZER_PWM_PIN, OUTPUT);
  analogWrite(BUZZER_PWM_PIN, 127);
}

void deactivateAlert() {
  DLOG("deactivateAlert()\n");
  deactivate_after = 0;

  // Turn off the relay and buzzer.
  // Was using analogWrite(BUZZER_PWM_PIN, 0), but that kept the buzzer making
  // a very small amount of noise.  Instead change it to an input pin and read
  // from it, one or both of which appears to disable the PWM feature.
  digitalWrite(RELAY_PIN, 0);
  pinMode(BUZZER_PWM_PIN, INPUT);
  digitalRead(BUZZER_PWM_PIN);
}

// calibrate never returns, triggers a reboot.
void calibrate() {
  DLOG("calibration()\n");

  // Just in case an alert is active.
  deactivateAlert();

  // Clear the EEPROM bytes we use.
  for (int addr = 0; addr < 11; ++addr) {
    EEPROM.write(addr, 0);
  }

  // Wait for a bit (5 seconds) to allow the user to get out of the way.
  // Toggle the high LED.
  for (int ndx = 0; ndx < 10; ++ndx) {
    high_sensor.toggleLed();
    delay(500);
  }
  high_sensor.ledOff();

  // Now calibrate each sensor (get the maximum voltage we see over an interval).
  // Leave the corresponding LED on after it is calibrated.
  low_sensor.calibrate();
  low_sensor.ledOn();

  medium_sensor.calibrate();
  medium_sensor.ledOn();

  high_sensor.calibrate();
  high_sensor.ledOn();

  // Remember the calibration values.
  saveToEEPROM();

  do_calibrate = false;
  DLOG("reboot()\n");
  reboot();
}

enum MovementState {
  STATE_NOT_MOVING,

  STATE_MOVING_UP_LOW,
  STATE_MOVING_UP_LOW_AND_MEDIUM,
  STATE_MOVING_UP_MEDIUM,

  STATE_MOVING_DOWN_MEDIUM,
  STATE_MOVING_DOWN_MEDIUM_AND_LOW,
  STATE_MOVING_DOWN_LOW
};

// When was current upwards movement first detected?
unsigned long g_started_up_at_ms;
unsigned long g_last_state_change_ms = 0;
MovementState g_movement_state;

void setMovementState(const int new_state, const unsigned long now_ms) {
  if (new_state != g_movement_state) {
    g_movement_state = (MovementState)new_state;
    g_last_state_change_ms = now_ms;
  }
}

bool isMovingUp(const int state) {
  switch (state) {
   case STATE_MOVING_UP_LOW:
   case STATE_MOVING_UP_LOW_AND_MEDIUM:
   case STATE_MOVING_UP_MEDIUM:
    return true;
  }
  return false;
}

void setup() {
  DLOG("setup() entry\n");

  // Turn on the high LED, then turn off when ready to run.
  initSensorsAndLEDs();
  high_sensor.ledOn();

  pinMode(CALIBRATION_INT, INPUT);

  // Make sure the outputs are "off".
  pinMode(RELAY_PIN, OUTPUT);
  deactivateAlert();

  // Assumes hooked up to 3.3v.
  analogReference(EXTERNAL);

  // First analogRead is a bit slower, so do it now.
  low_sensor.readSensor(1);
  medium_sensor.readSensor(1);
  high_sensor.readSensor(1);

  // Read the sensor thresholds from EEPROM.
  if (!initializeFromEEPROM()) {
    calibrate();  // Does not return.
  }

  // Calibration is complete.
  // Prepare to loop.

  ignore_until = 0;
  activate_after = 0;
  deactivate_after = 0;

  g_started_up_at_ms = 0;
  g_last_state_change_ms = 0;
  g_movement_state = STATE_NOT_MOVING;

  do_calibrate = false;
  attachInterrupt(CALIBRATION_INT, calibrationButtonPressed, FALLING);

  high_sensor.ledOff();

  DLOG("setup() exit\n");
}

#define SENSOR_TOLERANCE_MS 100

void loop() {
  if (do_calibrate) {
    DLOG("calibration button pressed\n");
    calibrate();  // Never returns.
  }

  // Check on timed events.
  const unsigned long now = millis();
  if (ignore_until && ignore_until <= now) {
    DLOG("Ignore Movement Period Expired\n");
    ignore_until = 0;
  }
  if (deactivate_after && deactivate_after <= now) {
    deactivateAlert();
  }

  // Update any blinking leds.
  low_sensor.updateLed(now);
  medium_sensor.updateLed(now);
  high_sensor.updateLed(now);

  // Read the sensors.
  const SensorReading low_reading = low_sensor.readSensor(now, NUM_ANALOG_SAMPLES, THRESHOLD_OFFSET);
  const SensorReading medium_reading = medium_sensor.readSensor(now, NUM_ANALOG_SAMPLES, THRESHOLD_OFFSET);
  const SensorReading high_reading = high_sensor.readSensor(now, NUM_ANALOG_SAMPLES, THRESHOLD_OFFSET);

  // Start or stop blinking the LEDs of recently triggered sensors.
  if (low_reading.isTriggered(SENSOR_TOLERANCE_MS)) {
    low_sensor.startBlinking(now, 100);
  } else {
    low_sensor.stopBlinking();
  }
  if (medium_reading.isTriggered(SENSOR_TOLERANCE_MS)) {
    medium_sensor.startBlinking(now, 100);
  } else {
    medium_sensor.stopBlinking();
  }
  if (low_reading.isTriggered(SENSOR_TOLERANCE_MS)) {
    medium_sensor.startBlinking(now, 100);
  } else {
    medium_sensor.stopBlinking();
  }

  // Ignore the high sensor for the moment, and attempt to determine the direction of movement.
  // Assume that, in general, won't have both the low and medium sensors change "at the same time".
  MovementState next_state = g_movement_state;
  switch (g_movement_state) {
   case STATE_NOT_MOVING:
    if (medium_reading.is_triggered) {
      // Assume moving down (i.e. don't check low_triggered).
      // Can adjust state on next loop to include medium.
      next_state = STATE_MOVING_DOWN_MEDIUM;
    } else if (low_reading.is_triggered) {
      // Assume moving up (i.e. not yet checking how long since the last transition).
      next_state = STATE_MOVING_UP_LOW;
    }
    break;

   case STATE_MOVING_UP_LOW:
    if (medium_reading.is_triggered) {
      // Assuming still moving up (i.e. not checking low_reading).
      next_state = STATE_MOVING_UP_LOW_AND_MEDIUM;
    } else if (!low_reading.isTriggered(SENSOR_TOLERANCE_MS)) {
      // Low sensor no longer triggered, assume detected
      // creature went down below the sensor.
      next_state = STATE_NOT_MOVING;
    }
    break;

   case STATE_MOVING_UP_LOW_AND_MEDIUM:
    if (!low_reading.is_triggered) {
      // Assume creature still moving up.
      next_state = STATE_MOVING_UP_MEDIUM;
    } else if (!medium_reading.is_triggered) {
      // Assume creature is being deterred by the alert.
      next_state = STATE_MOVING_UP_LOW;
    }
    break;

   case STATE_MOVING_UP_MEDIUM:
    if (low_reading.is_triggered) {
      // Assume creature still moving up, but trailing appendage
      // (tail, foot) may have triggered the low sensor.
      next_state = STATE_MOVING_UP_LOW_AND_MEDIUM;
    } else if (!medium_reading.isTriggered(SENSOR_TOLERANCE_MS)) {
      // Could track that a creature has moved above the sensors, if
      // wanted to continue alert for a longer period in that situation.
      next_state = STATE_NOT_MOVING;
    }
    break;

   case STATE_MOVING_DOWN_MEDIUM:
    if (low_reading.is_triggered) {
      // Assuming continuing to move down (i.e. not checking medium_reading).
      next_state = STATE_MOVING_DOWN_MEDIUM_AND_LOW;
    } else if (!medium_reading.isTriggered(SENSOR_TOLERANCE_MS)) {
      // Medium sensor no longer triggered, assume detected creature
      // went back up above the sensor.
      next_state = STATE_NOT_MOVING;
    }
    break;

   case STATE_MOVING_DOWN_MEDIUM_AND_LOW:
    if (!medium_reading.is_triggered) {
      // Assume creature continuing to move down (i.e. not checking low_reading).
      next_state = STATE_MOVING_DOWN_LOW;
    } else if (!low_reading.is_triggered) {
      // Treat as if creature is still moving down, but only triggering the medium sensor.
      next_state = STATE_MOVING_DOWN_MEDIUM;
    }
    break;

   case STATE_MOVING_DOWN_LOW:
    if (medium_reading.is_triggered) {
      // Assume creature still moving down, but trailing appendage
      // (tail, foot) may have triggered the medium sensor.
      next_state = STATE_MOVING_UP_LOW_AND_MEDIUM;
    } else if (!low_reading.isTriggered(SENSOR_TOLERANCE_MS)) {
      // Creature has moved below sensors.
      next_state = STATE_NOT_MOVING;
    }
    break;
  }

  // Is a tall creature passing the sensor?
  if (high_reading.is_triggered) {
    // Yes.
    if (ignore_until == 0) {
      DLOG("High Sensor Triggered\n");
    }
    ignore_until = now + DISABLE_PERIOD;
    deactivateAlert();
    activate_after = 0;
  } else if (ignore_until > now) {
    // Don't do anything.
  } else if (isMovingUp(next_state)) {
    if (deactivate_after) {
      // Alert is active; continue it.
      activateAlert();
    } else if (activate_after) {
      if (medium_reading.is_triggered || activate_after <= now) {
        activateAlert();
      }
    } else {
      DLOG("Movement Up Detected\n");
      activate_after = now + ALERT_DELAY_PERIOD;
    }
  } else if (activate_after) {
    DLOG("Pending Alert Cancelled\n");
  }


  if (now > 4000000000L) {
    // Millisecond clock will wrap around soon. Is now a quiet time to reboot?
    if (!low_reading.isTriggered(20000) &&
        !medium_reading.isTriggered(20000) &&
        !high_reading.isTriggered(20000)) {
      DLOG("Millisecond clock will soon wrap around; rebooting\n");
      reboot();
    }
  }

  setMovementState(next_state, now);

  delay(10);
}

