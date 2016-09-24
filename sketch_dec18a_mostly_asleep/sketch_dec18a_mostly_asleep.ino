// Keeps module asleep most of the time to allow battery to charge faster... hopefully.

#include <ESP8266WiFi.h>

void setup() {
  delay(100);
  Serial.begin(115200);
  delay(100);
  Serial.println();
  Serial.println("Good night");
  ESP.deepSleep(0xffffffff);
  delay(100);
}

void loop() {
}
