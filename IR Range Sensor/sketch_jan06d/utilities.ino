#include <stdarg.h>

void serial_printf(char *fmt, ... ) {
  char tmp[128]; // resulting string limited to 128 chars
  va_list args;
  va_start (args, fmt );
  vsnprintf(tmp, 128, fmt, args);
  va_end (args);
  Serial.print(tmp);
}

int16_t readVccMillivolts() {
  // Read 1.1V reference against Vcc
  // Set the reference to Vcc and the measurement to the internal 1.1V reference
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
  return int16_t(result); // Vcc in millivolts
}

// mvcc is Vcc in millivolts.
// Result is millivolts.
int16_t analogReadMillivolts(const int pin, const int16_t mvcc) {
  const int16_t sensor = analogRead(pin);
  const int16_t result = int16_t((sensor * int32_t(mvcc)) / 1023);
  serial_printf("sensor=%4d  mvcc=%4d   result -> %4d\n", sensor, mvcc, result);
/*
  Serial.print("sensor=");
  Serial.print(sensor);
  Serial.print("  mvcc=");
  Serial.print(mvcc);
  Serial.print("  result -> ");
  Serial.println(result);
*/
  return result;
}

// Result is millivolts.
int16_t analogReadMillivolts(const int pin) {
  const int16_t mvcc = readVccMillivolts();
  return analogReadMillivolts(pin, mvcc);
}

