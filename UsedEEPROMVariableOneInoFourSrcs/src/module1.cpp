#include "module1.h"

#include <EEPROM.h>

void setup_helper() {
  Serial.write(EEPROM[2] + 0);
}
