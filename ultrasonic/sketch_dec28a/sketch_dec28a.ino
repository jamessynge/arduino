#include <ColorLCDShield.h>

LCDShield lcd;  // Creates an LCDShield, named lcd

void setup()
{
//  lcd.init(EPSON);  // Initializes lcd, using an EPSON driver
  lcd.init(PHILLIPS);  // Initializes lcd, using an PHILLIPS driver
  lcd.contrast(40);  // 40's usually a good contrast value
//  lcd.clear(WHITE);  // oooh, teal!
//  lcd.printLogo();

  lcd.clear(WHITE);  // clear the screen
  lcd.setStr("Just say no", 2, 20, SLATE, WHITE);
  lcd.setStr("to addition", 110, 20, SLATE, WHITE);
  lcd.setCircle(66, 66, 45, RED);  // Circle in the mid, 55 radius
  lcd.setCircle(66, 66, 44, RED);  // Circle in the mid, 54 radius
  lcd.setRect(55, 34, 77, 98, 1, BLACK);
  lcd.setRect(34, 55, 98, 77, 1, BLACK);
  lcd.setLine(34, 34, 98, 98, RED);
  lcd.setLine(33, 34, 97, 98, RED);
  lcd.setLine(35, 34, 99, 98, RED);  
}

void loop()
{
}

