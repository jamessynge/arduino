#include "module1.h"
#include "module2.h"

#include <EEPROM.h>

void setup() {
  Serial.begin(9600);  
  Serial.write(EEPROM[0] + 0);
}
