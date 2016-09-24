void setup() {
  Serial.begin(115200);
  pinMode(0, OUTPUT);  // Red LED on Adafruit HUZZAH Feather
  pinMode(2, OUTPUT);  // Blue LED on Adafruit HUZZAH Feather
  digitalWrite(0, LOW);
  digitalWrite(2, LOW);

  delay(100);
  Serial.println();
  Serial.println("setup done");
}

void loop() {
  Serial.println("Sleep.... 2");
  delay(100);
  Serial.println("Sleep.... 1");
  delay(100);
  ESP.deepSleep(20000000);
  delay(100);
  Serial.println("Woke up....");
}
