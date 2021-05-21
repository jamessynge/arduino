// Include the AccelStepper library:
#include <AccelStepper.h>

#define kNoSuchPin 255

// Suggested pin selection for next revision.

#define kLedChannel1PwmPin 2               // OC3B
#define kLedChannel2PwmPin 3               // OC3C
#define kLedChannel3PwmPin 5               // OC3A
#define kLedChannel4PwmPin 6               // OC4A
#define kLedChannel1EnabledPin kNoSuchPin  //
#define kLedChannel2EnabledPin PIN_A1      //              was 10
#define kLedChannel3EnabledPin PIN_A2      //              was 11
#define kLedChannel4EnabledPin PIN_A3      //              was 12
#define kCoverEnabledPin PIN_A4            //              was 13
#define kCoverMotorStepPin 16              // TXD2         was 3
#define kCoverMotorDirectionPin 17         // RXD2         was 4
#define kCoverOpenLimitPin 18              // TXD1, INT3   was 20
#define kCoverCloseLimitPin 19             // RXD1, INT2   was 21

// Define stepper motor connections and motor interface type. Motor interface type must be set to 1 when using a driver:
#define motorInterfaceType 1

// Create a new instance of the AccelStepper class:
AccelStepper stepper = AccelStepper(motorInterfaceType, kCoverMotorStepPin, kCoverMotorDirectionPin);

#define kOpenPosition 0
#define kClosedPosition 108300
#define kStepsPerSecond 10000

void setEnabledPinMode(int pin, int mode) {
  if (pin != kNoSuchPin) {
    pinMode(pin, mode);
  }
}

void setup() {
  // Setup serial, wait for it to be ready so that our logging messages can be
  // read. Note that the baud rate is meaningful on boards that do true serial,
  // while those microcontrollers with builtin USB likely don't rate limit
  // because there isn't a need.
  Serial.begin(57600);

  // Wait for serial port to connect, or at least some minimum amount of time
  // (TBD), else the initial output gets lost. Note that this isn't true for all
  // Arduino-like boards: some reset when the Serial Monitor connects, so we
  // almost always get the initial output.
  while (!Serial) {
  }

  // Set the maximum speed in steps per second:
  stepper.setMaxSpeed(20000);

  pinMode(kLedChannel1PwmPin, OUTPUT);
  pinMode(kLedChannel2PwmPin, OUTPUT);
  pinMode(kLedChannel3PwmPin, OUTPUT);
  pinMode(kLedChannel4PwmPin, OUTPUT);

  pinMode(kCoverMotorStepPin, OUTPUT);
  pinMode(kCoverMotorDirectionPin, OUTPUT);

  pinMode(kCoverOpenLimitPin, INPUT_PULLUP);
  pinMode(kCoverCloseLimitPin, INPUT_PULLUP);

  setEnabledPinMode(kLedChannel1EnabledPin, INPUT_PULLUP);
  setEnabledPinMode(kLedChannel2EnabledPin, INPUT_PULLUP);
  setEnabledPinMode(kLedChannel3EnabledPin, INPUT_PULLUP);
  setEnabledPinMode(kLedChannel4EnabledPin, INPUT_PULLUP);
  setEnabledPinMode(kCoverEnabledPin, INPUT_PULLUP);
}

bool announceEnabled(const char* name, int enabled_pin) {
  Serial.print(name);
  if (enabled_pin != kNoSuchPin && digitalRead(enabled_pin) != LOW) {
    Serial.println(" is disabled");
    return false;
  } else {
    Serial.println(" is enabled");
    return true;
  }
}

void SweepLed(int led_channel, int pwm_pin, int enabled_pin) {
  Serial.print("LED ");
  Serial.print(led_channel);
  if (announceEnabled("", enabled_pin)) {
    // fade in from min to max in increments of 1 ADU:
    for (int fadeValue = 0 ; fadeValue <= 255; fadeValue += 1) {
      // sets the value (range from 0 to 255):
      analogWrite(pwm_pin, fadeValue);
      // wait for 2 milliseconds to see the dimming effect
      delay(2);
    }

    // fade out from max to min in increments of 1 ADU:
    for (int fadeValue = 255 ; fadeValue >= 0; fadeValue -= 1) {
      // sets the value (range from 0 to 255):
      analogWrite(pwm_pin, fadeValue);
      // wait for 2 milliseconds to see the dimming effect
      delay(2);
    }
  }
  delay(200);
}


void SweepCover(int stop_pin, int32_t stop_position) {
  while (stepper.currentPosition() != stop_position && digitalRead(stop_pin) != LOW) {
    stepper.runSpeed();
  }
}

void loop() {
  SweepLed(1, kLedChannel1PwmPin, kLedChannel1EnabledPin);
  SweepLed(2, kLedChannel2PwmPin, kLedChannel2EnabledPin);
  SweepLed(3, kLedChannel3PwmPin, kLedChannel3EnabledPin);
  SweepLed(4, kLedChannel4PwmPin, kLedChannel4EnabledPin);

  if (announceEnabled("Cover", kCoverEnabledPin)) {
    // Open if not already open.
    if (digitalRead(kCoverOpenLimitPin) != LOW) {
      // Run the motor backward (opening the cover) until the motor reaches the open position,
      // or closes the limit switch (approximately 270 degress). Assume fully closed at the start.
      stepper.setCurrentPosition(kClosedPosition);
      stepper.setSpeed(-kStepsPerSecond);
      SweepCover(kCoverOpenLimitPin, kOpenPosition);
    } else {
      Serial.println("Already open");
    }

    delay(1000);

    // Close if not already closed.
    if (digitalRead(kCoverCloseLimitPin) != LOW) {
      // Run the motor forward (closing the cover) until the motor reaches the closed position,
      // or closes the limit switch (approximately 270 degress). Assume fully open at the start.
      stepper.setCurrentPosition(kOpenPosition);
      stepper.setSpeed(kStepsPerSecond);
      SweepCover(kCoverCloseLimitPin, kClosedPosition);
    } else {
      Serial.println("Already closed");
    }
  }
}
