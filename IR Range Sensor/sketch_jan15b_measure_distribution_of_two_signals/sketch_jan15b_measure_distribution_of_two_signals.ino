// Goal is to measure the distribution of two IR sensor signals,
// while holding the range constant.  Pressing a button on the
// LCDShield will reset the collection, measurement will occur
// over next second, then results will be printed to serial port.

#include <ColorLCDShield.h>

extern void serial_printf(const char *fmt, ...);

const int LOW_PIN = 0; //analog pin 0
const int HIGH_PIN = 5; //analog pin 5

LCDShield lcd;  // Creates an LCDShield, named lcd

char char_buffer[16];

void lcdSetInt(int value, int x, int y, int fColor, int bColor) {
  sprintf(char_buffer, "%3d", value);
  lcd.setStr(char_buffer, x, y, fColor, bColor);
}

const uint16_t MAX_READINGS_COUNT = 5000;
uint16_t lowReadings[256];
uint16_t highReadings[256];
boolean doMeasure = false;
int analogSamples = 10;

void resetMeasurements() {
  for (int ndx = 0; ndx < 256; ++ndx) {
    lowReadings[ndx] = 0;
    highReadings[ndx] = 0;
  }
}

void printSeparator() {
  Serial.println("############################################################");
}

void startMeasuring() {
  printSeparator();
  serial_printf("Starting measurements, analogReads per reading = %d\n", analogSamples);
  serial_printf("Waiting for any counter to reach %u\n", MAX_READINGS_COUNT);
  resetMeasurements();
  doMeasure = true;
}

boolean recordMeasurements(const int lowReading, const int highReading) {
  if (doMeasure) {
    lowReadings[lowReading] += 1;
    highReadings[highReading] += 1;
    if (lowReadings[lowReading] == MAX_READINGS_COUNT) {
      serial_printf("lowReadings[%d] reached limit of %u\n", lowReading, lowReadings[lowReading]);
      doMeasure = false;
    }
    if (highReadings[highReading] == MAX_READINGS_COUNT) {
      serial_printf("highReadings[%d] reached limit of %u\n", highReading, highReadings[highReading]);
      doMeasure = false;
    }
  }
  return doMeasure;
}

void printMeasurements() {
  serial_printf("\nanalogReads per reading = %d\nCounters:\n", analogSamples);
  for (int ndx = 0; ndx < 256; ++ndx) {
    serial_printf("%d,%u,%u\n", ndx, lowReadings[ndx], highReadings[ndx]);
  }
  printSeparator();
}

//*******************************************************
//				Button Pin Definitions
//*******************************************************
#define	kSwitch1_PIN	3
#define	kSwitch2_PIN	4
#define	kSwitch3_PIN	5

void setup() {
//  lcd.init(EPSON);  // Initializes lcd, using an EPSON driver
  lcd.init(PHILLIPS);  // Initializes lcd, using an PHILLIPS driver
  lcd.contrast(40);  // 40's usually a good contrast value
  lcd.clear(RED);

  Serial.begin(9600);

  delay(1000);
  
  serial_printf("s1 = %d\n", readButton(kSwitch1_PIN));
  serial_printf("s2 = %d\n", readButton(kSwitch2_PIN));
  serial_printf("s3 = %d\n", readButton(kSwitch3_PIN));

  serial_printf("Ready\n");
  startMeasuring();
}

boolean readButton(int pin) {
  if (!digitalRead(pin)) {
    // Button is pressed.  Wait until it is released.
    while (!digitalRead(pin))
      ;
    return true;
  } else {
    return false;
  }
}

boolean firstTime = true;

void loop() {
  if (readButton(kSwitch1_PIN)) {
    if (doMeasure) {
      doMeasure = false;
      printMeasurements();
    } else {
      startMeasuring();
    }
  }
  
  if (doMeasure) {
    int32_t lowPinSum = 0, highPinSum = 0;
    for (int ndx = 0; ndx < analogSamples; ++ndx) {
      lowPinSum += analogRead(LOW_PIN);
      highPinSum += analogRead(HIGH_PIN);
    }
    const int sumToMeasurement = analogSamples * 4;  // (4 == 1024 / 256)
    int lowPinAvg = lowPinSum / sumToMeasurement;
    int highPinAvg = highPinSum / sumToMeasurement;
    if (!recordMeasurements(lowPinAvg, highPinAvg)) {
      printMeasurements();
      if (firstTime && analogSamples > 1) {
        analogSamples = max(1, analogSamples - 2);
        startMeasuring();
      } else {
        firstTime = false;
      }
    }
  } else {
    boolean s2 = readButton(kSwitch2_PIN);
    boolean s3 = readButton(kSwitch3_PIN);

    if (s2) {
      analogSamples = max(1, analogSamples - 1);
      serial_printf("analogReads per reading = %d\n", analogSamples);
    } else if (s3) {
      analogSamples = min(30, analogSamples + 1);
      serial_printf("analogReads per reading = %d\n", analogSamples);
    }
  }

}
