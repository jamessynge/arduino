int sensorPin = 5; //analog pin 5

void setup() {
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

int min_sampler(const int pin, const int times) {
  int best = 10000;
  for (int ndx = 0; ndx < times; ++ndx) {
    int val = analogReadMillivolts(pin);
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
};

void loop() {
  int16_t mV = min_sampler(sensorPin, 5);
  float cm = GP2Y0A02YK::millivoltsToCentimeters(mV);
  Serial.print("mV = ");
  Serial.print(mV);
  Serial.print("    cm = ");
  Serial.println(cm);
  delay(100);
}
