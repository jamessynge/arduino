/*
Compiled for Arduino UNO, with EEPROM declared as static or extern:

Sketch uses 1458 bytes (4%) of program storage space. Maximum is 32256 bytes.
Global variables use 184 bytes (8%) of dynamic memory, leaving 1864 bytes for local variables. Maximum is 2048 bytes.
*/

#include <Arduino.h>
#include <EEPROM.h>

void setup() {
  Serial.begin(9600);
  Serial.write('0' + 0);
}

void loop() {
  Serial.write('1' + 0);
}
