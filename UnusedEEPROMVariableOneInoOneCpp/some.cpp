#include "some.h"

#include <EEPROM.h>

void setup() {
  Serial.begin(9600);
  Serial.write('0' + 0);
}
