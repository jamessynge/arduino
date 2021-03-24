#include "module3.h"

#include <Arduino.h>
#include <EEPROM.h>

#include "module2.h"

void loop() {
  Serial.write(EEPROM[1] + 0);
  loop_helper();
}