#include "misc.h"

#include <stdarg.h>
#include <avr/wdt.h>

// See http://wiblocks.luciani.org/docs/app-notes/software-reset.html
void reboot() {
  wdt_disable();  
  wdt_enable(WDTO_15MS);
  while (1) {}
}

void serialPrintf(const char *fmt, ... ) {
  static boolean initialized = false;
  if (!initialized) {
    Serial.begin(9600);
    initialized = true;
    Serial.println("Serial.begin called");
  }
  char tmp[128]; // resulting string limited to 128 chars
  va_list args;
  va_start (args, fmt );
  vsnprintf(tmp, 128, fmt, args);
  va_end (args);
  Serial.print(tmp);
  delay(100);
}

