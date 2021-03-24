#include "module3.h"

#include <Arduino.h>
#include <EEPROM.h>

#include "module1.h"

void setup() {
  Serial.begin(9600);
  Serial.write(EEPROM[0] + 0);
  setup_helper();
}
