// So far, found that the stability of the Maxbotix LV-EZ1 isn't so good (very jumpy).
// But, have since found that this may be due to power supply fluctuations, which
// probably effect both the LV-EZ1, and the Arduino analogRead.
// I've compensated by adding smoothing of the data (see gatherData()), and by
// using non-USB power (which added a large fraction of the power fluctuations).

// This sketch switches from reading the analog output (0-Vcc == 0" to 254"),
// to reading the PWM output which is measuring the time to the first echo,
// so if it is possible to determine the duration accurately, then we can 
// measure the distance.

#include <ColorLCDShield.h>

#define kPWInput 2  // Digital pin for input of PW signal from Maxbotix LV-EZ1
#define kUsec_per_inch 147.0

LCDShield lcd;  // Creates an LCDShield, named lcd

char char_buffer[16];

void lcdSetInt(int value, int x, int y, int fColor, int bColor) {
  sprintf(char_buffer, "%6d", value);
  lcd.setStr(char_buffer, x, y, fColor, bColor);
}

void setup() {
//  lcd.init(EPSON);  // Initializes lcd, using an EPSON driver
  lcd.init(PHILLIPS);  // Initializes lcd, using an PHILLIPS driver
  lcd.contrast(40);  // 40's usually a good contrast value
  lcd.clear(WHITE);

  pinMode(kPWInput, INPUT);
}


void loop() {
  int pulse_length_us = pulseIn(kPWInput, HIGH, 50000);
  int inches = int(pulse_length_us / kUsec_per_inch + 0.5);

  lcdSetInt(inches, 2, 20, BLUE, WHITE);
}

