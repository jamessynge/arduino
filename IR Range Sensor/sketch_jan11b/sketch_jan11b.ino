const int LOW_PIN = 0; //analog pin 0
const int HIGH_PIN = 5; //analog pin 5

void setup() {
  Serial.begin(9600);
}

extern 
void serial_printf(const char *fmt, ... ) ;


int average_sampler(const int pin, const int times) {
  int32_t sum = 0;
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

int min_sampler(const int pin, const int times) {
  int best = 10000;
  for (int ndx = 0; ndx < times; ++ndx) {
    int val = analogRead(pin);
    if (best > val) {
      best = val;
    }
  }
  return best;
}

int last = -1;

// Sharp GP2Y0A02YK IR Range Sensor.
// Datasheet: http://sharp-world.com/products/device/lineup/data/pdf/datasheet/gp2y0a02_e.pdf
// 
// From: http://www.robotshop.com/PDF/Sharp_GP2Y0A02YK_Ranger.pdf
// An curve fit formula is used to approximate distance as a function of voltage:
//
//             A + B*V
//     D = ---------------
//         1 + C*X + D*X^2
// 
// where
//   D   = Distance (cm)
//   V   =  Voltage (V)
//   A   =  0.008 271
//   B   =  939.6
//   C   = -3.398
//   D   =  17.339

class GP2Y0A02YK {
 private:
  static const float A = 0.0082712905f;
  static const float B = 939.57652f;
  static const float C = -3.3978697f;
  static const float D = 17.339222f;
 public:
  static float millivoltsToCentimeters(const int16_t mV) {
    float V = mV / 1000.0f;
    return (A + B * V) / (1.0 + C * V + D * V * V);
  }
  static float analogReadingToCentimeters(const int16_t ar) {
    float V = ar * 5.0f / 1023.0f;
    return (A + B * V) / (1.0 + C * V + D * V * V);
  }
  static float voltsToCentimeters(const float V) {
    return (A + B * V) / (1.0 + C * V + D * V * V);
  }
};

const int analogSamples = 20;
const float sumToVolts = 5.0f / (1023.0f * analogSamples);

void loop() {
  int32_t lowPinSum = 0, highPinSum = 0;
  for (int ndx = 0; ndx < analogSamples; ++ndx) {
    lowPinSum += analogRead(LOW_PIN);
    highPinSum += analogRead(HIGH_PIN);
  }
  float lowPinVolts = lowPinSum * sumToVolts;
  float highPinVolts = highPinSum * sumToVolts;
  int lowCm = (int)GP2Y0A02YK::voltsToCentimeters(lowPinVolts);
  int highCm = (int)GP2Y0A02YK::voltsToCentimeters(highPinVolts);

  serial_printf(" lowPinSum=%05ld    lowMillivolts=%4d    lowCm=%d\n",  lowPinSum,  (int)(lowPinVolts*1000),  lowCm);
  serial_printf("highPinSum=%05ld   highMillivolts=%4d   highCm=%d\n", highPinSum, (int)(highPinVolts*1000), highCm);

  delay(200);
}
