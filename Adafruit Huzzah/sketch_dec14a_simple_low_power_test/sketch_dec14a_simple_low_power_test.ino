#define RED_LED 0   // Red LED on Adafruit HUZZAH Feather; no pull-up resistor
#define BLUE_LED 2  // Blue LED on Adafruit HUZZAH Feather; has a pull-up resistor
void setup() {
  Serial.begin(115200);

//  delay(100);
  Serial.println();

  Serial.print("Time: ");
  Serial.println(millis());

  pinMode(RED_LED, INPUT);
  
//  pinMode(RED_LED, OUTPUT);
//  for (int i = 0; i < 5; ++i) {
//    digitalWrite(RED_LED, LOW);
//    delay(400);
//    digitalWrite(RED_LED, HIGH);
//    delay(100);
//  }
//  digitalWrite(RED_LED, HIGH);
////  pinMode(RED_LED, INPUT);

  pinMode(BLUE_LED, OUTPUT);
  for (int i = 0; i < 5; ++i) {
    digitalWrite(BLUE_LED, LOW);
    delay(200);
    digitalWrite(BLUE_LED, HIGH);
    delay(50);
  }
  pinMode(BLUE_LED, INPUT_PULLUP); 

  Serial.println("setup done");
}

void loop() {
  Serial.println("Sleep.... 2");
  delay(100);
  Serial.println("Sleep.... 1");
  delay(100);
  if (true) {
    pinMode(RED_LED, OUTPUT);
//    digitalWrite(RED_LED, LOW);
//    delay(10);
    digitalWrite(RED_LED, HIGH);
    delay(10);
  } else {
    pinMode(RED_LED, INPUT);
    delay(1);
  }
  ESP.deepSleep(10000000);
  delay(100);
  Serial.println("Woke up....");
}
