#include <ColorLCDShield.h>

extern void serial_printf(const char *fmt, ...);

const int LOW_PIN = 0; //analog pin 0
const int HIGH_PIN = 5; //analog pin 5

LCDShield lcd;  // Creates an LCDShield, named lcd

char char_buffer[16];

void lcdSetInt(int value, int x, int y, int fColor, int bColor) {
  sprintf(char_buffer, "%3d", value);
  lcd.setStr(char_buffer, x, y, fColor, bColor);
}

const int BACKGROUND_COLOR = SLATE;
const int GRID_COLOR = WHITE;
const int CURSOR_COLOR = RED;

int currentTime = 0;

void setTime(int newTime) {
  if (newTime >= 132) {
    newTime = 0;
  }
  currentTime = newTime;
  if (newTime % 20 == 0) {
    lcd.setLine(20, newTime, 125, newTime, GRID_COLOR);
  } else {
    // Clear the current time to white, and draw the grids
    lcd.setLine(21, newTime, 124, newTime, BACKGROUND_COLOR);
    lcd.setPixel(GRID_COLOR, 125, newTime);
    lcd.setPixel(GRID_COLOR, 110, newTime);
    lcd.setPixel(GRID_COLOR,  95, newTime);
    lcd.setPixel(GRID_COLOR,  80, newTime);
    lcd.setPixel(GRID_COLOR,  65, newTime);
    lcd.setPixel(GRID_COLOR,  50, newTime);
    lcd.setPixel(GRID_COLOR,  35, newTime);
    lcd.setPixel(GRID_COLOR,  20, newTime);
  }
  // Draw a cursor (a red vertical line) just after the current time.
  newTime = (newTime + 1) % 132;
  lcd.setLine(20, newTime, 125, newTime, CURSOR_COLOR);
}

const int32_t VALUE_RANGE = 121;

void drawValue(int value, int minimum, int maximum, int color) {
  int16_t v2 = int16_t(int32_t(value - minimum) * VALUE_RANGE / maximum);
  int16_t x = 130 - v2;
  lcd.setPixel(color, x, currentTime);
}

void setup() {
  Serial.begin(9600);

//  lcd.init(EPSON);  // Initializes lcd, using an EPSON driver
  lcd.init(PHILLIPS);  // Initializes lcd, using an PHILLIPS driver
  lcd.contrast(40);  // 40's usually a good contrast value
  lcd.clear(BACKGROUND_COLOR);
  
  for (int i = 0; i < 132; ++i) {
    setTime(i);
  }
}

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
    return voltsToCentimeters(V);
  }
  static float analogReadingToCentimeters(const int16_t ar) {
    float V = ar * 5.0f / 1023.0f;
    return voltsToCentimeters(V);
  }
  static float voltsToCentimeters(const float V) {
    return (A + B * V) / (1.0 + C * V + D * V * V);
  }
};

const int analogSamples = 10;
const float sumToVolts = 5.0f / (1023.0f * analogSamples);

int lastLowCm = -1, lastHighCm = -1;

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

  if (lastLowCm >= 0) {
    lowCm = (lowCm + lastLowCm) / 2;
    highCm = (highCm + lastHighCm) / 2;
  }

  lcdSetInt(lowCm, 0, 0, BLACK, YELLOW);
  lcdSetInt(highCm, 0, 80, BLACK, 0x8f8);

  setTime(currentTime + 1);
  drawValue(lowCm, 0, 200, YELLOW);
  drawValue(highCm, 0, 200, GREEN);

  lastLowCm = lowCm;
  lastHighCm = highCm;

  delay(30);
}
