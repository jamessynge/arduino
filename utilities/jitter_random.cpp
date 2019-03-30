// This is based on TrueRandomSeed.ino by Walter Anderson, copied from:
//   https://sites.google.com/site/astudyofentropy/project-definition/timer-jitter-entropy-sources/entropy-library/arduino-random-seed
// See also:
//   https://sites.google.com/site/astudyofentropy/project-definition/timer-jitter-entropy-sources/entropy-library
//
// Original Copyright 2014 by Walter Anderson, wandrson01 at gmail dot com

#include "jitter_random.h"

#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/atomic.h>

namespace {

volatile uint32_t rand;  // These two variables can be reused in your program after the
volatile int8_t nrot;    // function CreateTrulyRandomSeed()executes in the setup() 
                         // function.
volatile uint32_t TCNT1L_values[64];
volatile uint32_t TCNT1H_values[64];
volatile int values_cursor=0;

// Interrupt service routine, records the jitter in TCNT1L.
ISR(WDT_vect)
{
  nrot--;
  rand = rand << 8;

  auto lvalue = TCNT1L;
  auto hvalue = TCNT1H;

  rand = rand ^ lvalue;
  if (values_cursor < 64) {
    TCNT1L_values[values_cursor] = lvalue;
    TCNT1H_values[values_cursor] = hvalue;
    ++values_cursor;
  }
}

}  // namespace
 
uint32_t JitterRandom::random32()
{
  rand = 0;
  nrot = 32; // Must be at least 4, but more increased the uniformity of the produced 
             // seeds entropy.
  values_cursor = 0;
  
  // The following five lines of code turn on the watch dog timer interrupt to create
  // the rand value
  cli();                                             
  MCUSR = 0;                                         
  _WD_CONTROL_REG |= (1<<_WD_CHANGE_BIT) | (1<<WDE); 
  _WD_CONTROL_REG = (1<<WDIE);                       
  sei();                                             
 
  while (nrot > 0);  // wait here until rand is created
 
  // The following five lines turn off the watch dog timer interrupt
  cli();                                             
  MCUSR = 0;                                         
  _WD_CONTROL_REG |= (1<<_WD_CHANGE_BIT) | (0<<WDE); 
  _WD_CONTROL_REG = (0<< WDIE);                      
  sei();

  Serial.print("JitterRandom::random32 - sizeof TCNT1L=");
  Serial.println(sizeof(TCNT1L));








  return rand;                                         
}
