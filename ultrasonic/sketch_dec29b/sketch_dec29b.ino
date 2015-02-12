#include <ColorLCDShield.h>

LCDShield lcd;  // Creates an LCDShield, named lcd

//*******************************************************
//				Button Pin Definitions
//*******************************************************
#define	kSwitch1_PIN	3
#define	kSwitch2_PIN	4
#define	kSwitch3_PIN	5

char char_buffer[16];

#define MAX_SAMPLES 20
int samples[MAX_SAMPLES];
int num_valid_samples = 0;
int last_sample = 0;
int loopDelay = 300;

void drawValue(int value) {
  sprintf(char_buffer, "%6d", value);
//  lcd.clear(WHITE);  // clear the screen
  lcd.setStr(char_buffer, 2, 20, BLUE, WHITE);
}

void handleButtons(int s1, int s2, int s3) {
  if (s1) { // if button 1 is pressed then ....
    loopDelay = 1000;
    lcd.setStr("     Slow  ", 100, 0, BLUE, WHITE);
  } else if (s2) {
    loopDelay = 250;
    lcd.setStr("     Medium", 100, 0, BLUE, WHITE);
  } else if (s3) {
    loopDelay = 50;
    lcd.setStr("     Fast  ", 100, 0, BLUE, WHITE);
  }
}


void checkInputs() {
  int s1, s2, s3; // Make integers for all the swiches
  s1 = !digitalRead(kSwitch1_PIN);
  s2 = !digitalRead(kSwitch2_PIN);
  s3 = !digitalRead(kSwitch3_PIN);

  handleButtons(s1, s2, s3);
}

void setup() {
//  lcd.init(EPSON);  // Initializes lcd, using an EPSON driver
  lcd.init(PHILLIPS);  // Initializes lcd, using an PHILLIPS driver
  lcd.contrast(40);  // 40's usually a good contrast value
  lcd.clear(WHITE);

  handleButtons(0, 1, 0);
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

