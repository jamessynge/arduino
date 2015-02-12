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
extern void serialPrintf(const char *fmt, ...);

#define DEBUG false
#if DEBUG
#define DLOG(...) serialPrintf(__VA_ARGS__)
#else
#define DLOG(...) 0
#endif

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

class SensorAndLED {
 public:
  SensorAndLED(byte sensor_pin, byte led_pin, char tag)
      : sensor_pin(sensor_pin),
        led_pin(led_pin),
        tag(tag) {
    init();
  }

  void init() {
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

  void ledOff() {
    digitalWrite(led_pin, LOW);
    led_state = false;
  }

  void ledOn() {
    digitalWrite(led_pin, HIGH);
    led_state = true;
  }

  void toggleLed() {
    if (led_state) {
      ledOff();
    } else {
      ledOn();
    }
  }

  void startBlinking(const long now_millis, int toggle_period) {
    if (this->toggle_period == toggle_period) {
      return;
    }
    ledOn();
    this->toggle_period = toggle_period;
    this->next_toggle += toggle_period;
  }

  void stopBlinking() {
    ledOff();
    toggle_period = 0;
    next_toggle = 0;
  }

  void updateLed(const long now_millis) {
    if (toggle_period) {
      if (next_toggle <= now_millis) {
        toggleLed();
        next_toggle += toggle_period;
      }
    }
  }

  void calibrate() {
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

  boolean isSensorTriggered(const int num_reads, const int tolerance) {
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

  int readSensor(int num_reads) {
    int value = 1023;
    do {
      value = min(value, analogRead(sensor_pin));
    } while (--num_reads > 0);
    return value;
  }

  int writeThreshold(int addr) {
    DLOG("writeThreshold(%d) '%c' threshold %d\n", addr, tag, threshold);
    EEPROM.write(addr++, tag);
    EEPROM.write(addr++, threshold >> 8);
    EEPROM.write(addr++, threshold & 0xff);
    return addr;
  }

  // Read the threshold for this sensor (Tag Byte, High Byte, Low Byte).
  // Returns the addr beyond the threshold if the data is value, or -1 if not.
  int readThreshold(int addr) {
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

 private:
  unsigned long next_toggle;
#if DEBUG
  unsigned long next_announce;
#endif
  int toggle_period;
  int threshold;
  boolean led_state;
  const byte sensor_pin;
  const byte led_pin;
  const char tag;
};

SensorAndLED lowSensor(LOW_SENSOR_PIN, LOW_LED_PIN, 'L');
SensorAndLED mediumSensor(MEDIUM_SENSOR_PIN, MEDIUM_LED_PIN, 'M');
SensorAndLED highSensor(HIGH_SENSOR_PIN, HIGH_LED_PIN, 'H');

boolean initializeFromEEPROM() {
  if (EEPROM.read(0) != 'D' ||
      EEPROM.read(1) != 'o' ||
      EEPROM.read(2) != 'g') {
    return false;
  }
  int addr = 3;

  addr = lowSensor.readThreshold(addr);
  if (addr < 0) return false;

  addr = mediumSensor.readThreshold(addr);
  if (addr < 0) return false;

  addr = highSensor.readThreshold(addr);
  if (addr < 0) return false;

  return EEPROM.read(addr) == 0;
}

void saveToEEPROM() {
  DLOG("Writing to EEPROM: %d, %d\n", low_value, high_value);
  int addr = 0;
  EEPROM.write(addr++, 'X');  // Mark as invalid until done.
  EEPROM.write(addr++, 'o');
  EEPROM.write(addr++, 'g');
  addr = lowSensor.writeThreshold(addr);
  addr = mediumSensor.writeThreshold(addr);
  addr = highSensor.writeThreshold(addr);
  EEPROM.write(addr++, 0);
  EEPROM.write(0, 'D');       // Finally valid.
}

void initSensorsAndLEDs() {
  lowSensor.init();
  mediumSensor.init();
  highSensor.init();
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
    highSensor.toggleLed();
    delay(500);
  }
  highSensor.ledOff();

  // Now calibrate each sensor (get the maximum voltage we see over an interval).
  // Leave the corresponding LED on after it is calibrated.
  lowSensor.calibrate();
  lowSensor.ledOn();

  mediumSensor.calibrate();
  mediumSensor.ledOn();

  highSensor.calibrate();
  highSensor.ledOn();

  // Remember the calibration values.
  saveToEEPROM();

  do_calibrate = false;
  DLOG("reboot()\n");
  reboot();
}

typedef enum StateEnum {
  STATE_NORMAL = 0,
  STATE_HIGH_TRIGGERED,
  STATE_MOVING_UP_LOW,
  STATE_MOVING_UP_LOW_THEN_MEDIUM,
  STATE_MOVING_UP_MEDIUM,
  STATE_ESCAPED,  // Low then Medium, then Medium only, then nothing.
  STATE_MOVING_DOWN_MEDIUM,
  STATE_MOVING_DOWN_MEDIUM_THEN_LOW,
  STATE_MOVING_DOWN_LOW,

  NUM_STATES
} StateEnumT;

StateEnumT g_current_state;
struct State;

typedef StateEnumT (*TransitionFunc)(
    const unsigned long now,
    const boolean low_triggered,
    const boolean medium_triggered);

typedef void (*EnterStateFunc)(const unsigned long now);

struct State {
  State(const char* name, TransitionFunc trans, EnterStateFunc enter) : trans(trans), enter(enter) {}
  const char* name;
  const TransitionFunc trans;
  const EnterStateFunc enter;
};

State* g_states[(int)NUM_STATES];

#define DEF_STATE(ID, TRANS, ENTER) \
  State g_##STATE_##ID(#ID, TRANS, ENTER); \
  g_states[STATE_##ID] = &g_##STATE_##ID;

// Nothing happening so far.
StateEnumT normal_trans(
    const unsigned long now,
    const boolean low_triggered,
    const boolean medium_triggered,
    const boolean high_triggered)
{
  if (
  return NULL;
}

void enter_normal() {
  // Just in case an alert is active.
  deactivateAlert();
}


void setup() {
  DLOG("setup() entry\n");

  // Turn on the high LED, then turn off when ready to run.
  initSensorsAndLEDs();
  highSensor.ledOn();

  // Make sure the outputs are "off".
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PWM_PIN, OUTPUT);
  deactivateAlert();

  // Assumes hooked up to 3.3v.
  analogReference(EXTERNAL);

  // First analogRead is a bit slower, so do it now.
  lowSensor.readSensor(1);
  mediumSensor.readSensor(1);
  highSensor.readSensor(1);

  // Read the sensor thresholds from EEPROM.
  if (!initializeFromEEPROM()) {
    calibrate();  // Does not return.
  }

  // Calibration is complete.
  // Prepare to loop.
  pinMode(CALIBRATION_INT, INPUT);
  attachInterrupt(CALIBRATION_INT, calibrationButtonPressed, FALLING);

  ignore_until = 0;
  activate_after = 0;
  deactivate_after = 0;
  g_current_state = STATE_NORMAL;

  delay(50);
  do_calibrate = false;

  highSensor.ledOff();

  DLOG("setup() exit\n");
}

void loop() {
  if (do_calibrate) {
    DLOG("calibration button pressed\n");
    calibrate();  // Never returns.
  }

  // Read the sensors.
  const boolean low_triggered = lowSensor.isSensorTriggered(NUM_ANALOG_SAMPLES, THRESHOLD_OFFSET);
  const boolean medium_triggered = mediumSensor.isSensorTriggered(NUM_ANALOG_SAMPLES, THRESHOLD_OFFSET);
  const boolean high_triggered = highSensor.isSensorTriggered(NUM_ANALOG_SAMPLES, THRESHOLD_OFFSET);
  const unsigned long now = millis();

  // Because the high sensor always transitions us to STATE_HIGH_TRIGGERED, we'll handle it here.
  // Has the high sensor been triggered?
  StateEnum next_state = g_current_state;
  if (high_triggered) {
    next_state = STATE_HIGH_TRIGGERED;
  } else {
    
      

const unsigned long current_millis = millis();

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

