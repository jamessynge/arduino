int sensorPin = 5; //analog pin 5

void setup(){
  Serial.begin(9600);
}

int last = -1;

int average_sampler(const int pin, const int times) {
  int sum = 0;
  for (int ndx = 0; ndx < times; ++ndx) {
    sum += analogRead(pin);
  }
  return sum / times;
}

int max_sampler(const int pin, const int times) {
  int best = -1;
  for (int ndx = 0; ndx < times; ++ndx) {
    int val = analogRead(pin);
    if (best < val) {
      best = val;
    }
  }
  return best;
}


void loop(){
  int val = max_sampler(sensorPin, 10);
  

  // print out a graphic representation of the result
  Serial.print(" ");
  for (x=0;x<(inches/5);x++)
  {
    Serial.print(".");
  }
  Serial.println("|");  
  
  
  if (abs(last - val) > 5) {
    Serial.println(val);
    delay(50);
  }
  last = val;
}
