int sensorPin = 5; //analog pin 5

void setup(){
  Serial.begin(9600);
}

int last = -1;

void loop(){
  int val = analogRead(sensorPin);
  if (abs(last - val) > 5) {
    Serial.println(val);
    delay(50);
  }
  last = val;
}
