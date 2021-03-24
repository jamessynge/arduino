#include "module2.h"

#include <EEPROM.h>

void loop_helper() {
  Serial.write(EEPROM[3] + 0);
}
