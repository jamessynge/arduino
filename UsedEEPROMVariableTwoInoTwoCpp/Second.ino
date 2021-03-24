#include <Arduino.h>
#include <EEPROM.h>

#include "module1.h"
#include "module2.h"

void loop() {
  Serial.write('1' + 0);
  loop_helper();
}
