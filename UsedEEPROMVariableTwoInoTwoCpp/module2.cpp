#include "module1.h"
#include "module2.h"

#include <EEPROM.h>

void loop() {
  Serial.write(EEPROM[1] + 0);
}
