int sensorPin = 5; //analog pin 5

int readVccMillivolts() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0);
  #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    ADMUX = _BV(MUX3) | _BV(MUX2);
  #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif  

  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH  
  uint8_t high = ADCH; // unlocks both

  long result = (high<<8) | low;

  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return int(result); // Vcc in millivolts
}

// mvcc is Vcc in millivolts.
// Result is millivolts.
int analogReadMillivolts(const int pin, const int mvcc) {
  const int sensor = analogRead(pin);
  const int result = (sensor * mvcc) / 1023;
  Serial.print("sensor=");
  Serial.print(sensor);
  Serial.print("  mvcc=");
  Serial.print(mvcc);
  Serial.print("  result -> ");
  Serial.println(result);
  return result;
}

// Result is millivolts.
int analogReadMillivolts(const int pin) {
  const int mvcc = readVccMillivolts();
  return analogReadMillivolts(pin, mvcc);
}

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
  int val = max_sampler(sensorPin, 10);
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
