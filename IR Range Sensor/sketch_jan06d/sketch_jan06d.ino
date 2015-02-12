int sensorPin = 5; //analog pin 5

void setup(){
  Serial.begin(9600);
}

int average_sampler(const int pin, const int times) {
  int sum = 0;
  for (int ndx = 0; ndx < times; ++ndx) {
    sum += analogReadMillivolts(pin);
  }
  return sum / times;
}

int max_sampler(const int pin, const int times) {
  int best = -1;
  for (int ndx = 0; ndx < times; ++ndx) {
    int val = analogReadMillivolts(pin);
    if (best < val) {
      best = val;
    }
  }
  return best;
}

/*
// Based on http://www.robotshop.com/PDF/Sharp_GP2Y0A02YK_Ranger.pdf

// reading is 0 to 1023 (0 to Vcc), though for the GP2Y0A02YK
// it appears that the max is around 500.
int readingToRange(const int reading) {
  float voltage = 

Private Function VoltageToRange( _
    ByVal V As Single) As Single
' Returns distance in units of cm.
    Const A As Single = 0.0082712905
    Const B As Single = 939.57652
    Const C As Single = -3.3978697
    Const D As Single = 17.339222
    ' Curve fit.
    VoltageToRange = (A + B * V) / (1! + C * V + D * V * V)
End Function
*/

int last = -1;

void loop(){
  int val = average_sampler(sensorPin, 10);
/*
  // print out a graphic representation of the result
  Serial.print(" ");
  for (x=0;x<(inches/5);x++)
  {
    Serial.print(".");
  }
  Serial.println("|");  
  */
  
  if (abs(last - val) > 5) {
    Serial.println(val);
    last = val;
    delay(50);
  }
}
