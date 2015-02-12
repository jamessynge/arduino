#include <ColorLCDShield.h>

LCDShield lcd;  // Creates an LCDShield, named lcd

//*******************************************************
//				Button Pin Definitions
//*******************************************************
#define	kSwitch1_PIN	3
#define	kSwitch2_PIN	4
#define	kSwitch3_PIN	5

char buffer[16];
int loopDelay = 300;

void drawValue(int value) {
  sprintf(buffer, "%6d", value);
  lcd.clear(WHITE);  // clear the screen
  lcd.setStr(buffer, 2, 20, BLUE, WHITE);
}

void checkInputs() {
  int s1, s2, s3; // Make integers for all the swiches
  s1 = !digitalRead(kSwitch1_PIN);
  s2 = !digitalRead(kSwitch2_PIN);
  s3 = !digitalRead(kSwitch3_PIN);

  if (s1) { // if button 1 is pressed then ....
    loopDelay = 1000;
    lcd.setStr("Slow  ", 2, 4, BLUE, WHITE);
  } else if (s2) {
    loopDelay = 300;
    lcd.setStr("Medium", 2, 4, BLUE, WHITE);
  } else if (s3) {
     loopDelay = 100;
     lcd.setStr("Fast  ", 2, 4, BLUE, WHITE);
  }
}

void setup() {
//  lcd.init(EPSON);  // Initializes lcd, using an EPSON driver
  lcd.init(PHILLIPS);  // Initializes lcd, using an PHILLIPS driver
  lcd.contrast(40);  // 40's usually a good contrast value
  lcd.clear(WHITE);
}

void loop() {
  int sensor;
  
  // read the analog output of the MaxBotix EZ1 from analog input 0.
  sensor = analogRead(0);

  // convert the sensor reading to inches
  //inches = sensor / 2;

  drawValue(sensor);

  checkInputs();

  // pause before taking the next reading
  delay(loopDelay);  
}

