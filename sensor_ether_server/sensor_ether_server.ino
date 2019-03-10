/*

This is adapted from the Arduino example:

     Ethernet > BarometricPressureWebServer

It demonstrates how to work with the Arduino Ethernet and EthernetBonjour
libraries, with lots of printed status to help the reader understand what
is going on at each step.

The sketch uses the EEPROM library to store its randomly generated
Ethernet address and, if necessary, its randomly generated IP address.

An Ethernet shield (or an freetronics EtherTen board, which I've used to test
this code) does not have its own MAC address (the unique identifier on the
local Ethernet used to distinguish between the packets sent by this device
from the packets sent by other devices on the Ethernet). Fortunately the
design of MAC addresses allows for both globally unique addresses (i.e.
assigned at the factory, unique world-wide) and locally unique addresses.
This code will generate an address and store it in EEPROM.
  


   TODO: Add use of EEPROM library, storing a generated MAC address
   on first run, and if DHCP fails, storing a generated IP address.
   Ideally we'd have some means of detecting the IP addresses in use
   on the local network segment (e.g. by using Promiscuous mode), but
   it doesn't appear that the Arduino Ethernet library supports this. 

   Author: James Synge
*/

// This is the Arduino "standard" Ethernet 2.0.0 library (or later).
#include <Ethernet.h>

// I've forked this library just so that I'm sure I can find it:
// https://github.com/jamessynge/EthernetBonjour
#include <EthernetBonjour.h>

#include "analog_random.h"

// Assign a MAC address for the Ethernet controller. Since the Arduino
// doesn't have its own MAC address assigned at the factory, you must
// pick one. Read more:
//
//     https://serverfault.com/a/40720
//     https://en.wikipedia.org/wiki/MAC_address#Universal_vs._local
//
// Quoting:
//
//     Universally administered and locally administered addresses are
//     distinguished by setting the second least significant bit of the
//     most significant byte of the address. If the bit is 0, the address
//     is universally administered. If it is 1, the address is locally
//     administered. In the example address 02-00-00-00-00-01 the most
//     significant byte is 02h. The binary is 00000010 and the second
//     least significant bit is 1. Therefore, it is a locally
//     administered address. The bit is 0 in all OUIs.
//
// So, ensure the low-nibble of the first byte is one of these 8
// hexidecimal values:
//
//    2  == 0010b
//    3  == 0011b
//    6  == 0110b
//    7  == 0111b
//    a  == 1010b
//    b  == 1011b
//    e  == 1110b
//    f  == 1111b
//
// There are websites that will generate these for you. Search for:
//
//    locally administered mac address generator
//
// TODO(james): Consider storing the address in the Arduino's EEPROM,
// generating a random address if it isn't there when first started.
// That would allow for this sketch to be copied many times, and used
// without editing.

// Fill in your address here:
byte mac[] = {
  0x52, 0xC4, 0x58, 0xC7, 0x2E, 0x81
};

// If we can't find a DHCP server to assign an address to us, we'll
// fallback to this static address.
IPAddress static_ip(192, 168, 86, 251);
bool using_dhcp = true;

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);

float temperature;
float pressure;
long lastReadingTime = 0;

void setup() {
  // As described on the freetronics website, there is a delay between reset
  // of the microcontroller and the ethernet chip. Do some other stuff first
  // so that we don't try initializing the Ethernet chip too soon.
  //
  //       https://www.freetronics.com.au/pages/usb-power-and-reset

  long startTime = millis();

  // Open serial communications and wait for host to start reading, but not
  // forever. This provides some time for the Arduino IDE Serial Monitor to
  // start reading before too many messages (or any) have been printed.
  Serial.begin(9600);
  unsigned long timeLimit = 10 * 1000UL;  // Wait at most 10 seconds.
  while (!Serial && (millis() - startTime) < 10 * 1000UL) {
    // Blink fast so that the person looking at it can tell the difference
    // between this blink rate and some of the others.
    blink(200);
  }

  Serial.println("Setting up Ethernet pins");

  // Tell the Ethernet library which pin to use for CS (Chip Select). SPI
  // buses (as used by the Wiznet W5000 series Ethernet chips) use 3 common
  // signalling lines, but each device (chip) gets its own chip select line
  // (also called slave select) so that each can be activated independently.
  Ethernet.init(10);  // Most Arduino shields
  //Ethernet.init(5);   // MKR ETH shield
  //Ethernet.init(0);   // Teensy 2.0
  //Ethernet.init(20);  // Teensy++ 2.0
  //Ethernet.init(15);  // ESP8266 with Adafruit Featherwing Ethernet
  //Ethernet.init(33);  // ESP32 with Adafruit Featherwing Ethernet

  Serial.println("Setting up Ethernet library");
  if (Ethernet.begin(mac)) {
    using_dhcp = true;
    Serial.print("DHCP successful, assigned address:");
    Serial.println(Ethernet.localIP());
  } else {
    using_dhcp = false;
    Serial.print("Unable to get an IP address via DHCP. Using fixed address: ");
    Serial.println(static_ip);
    Ethernet.setLocalIP(static_ip);

    // Assume that the subnet is a /24, with this mask:
    IPAddress subnet(255, 255, 255, 0);
    Ethernet.setSubnetMask(subnet);

    // Assume that the gateway is on the same subnet, but with the
    // last byte of the address being '1':
    IPAddress gateway = static_ip;
    gateway[3] = 1;
    Ethernet.setGatewayIP(gateway);
  }

  switch(Ethernet.hardwareStatus()) {
    case EthernetNoHardware:
      while (true) {
        Serial.println("Ethernet hardware was not found!");
        Serial.println("Sorry, can't run without it. :(");
        Serial.println();
        delay(1000); // do nothing, no point running without Ethernet hardware
      }

    case EthernetW5100:
      Serial.println("W5100 Ethernet controller detected.");
      break;

    case EthernetW5200:
      Serial.println("W5200 Ethernet controller detected.");
      break;

    case EthernetW5500:
      Serial.println("W5500 Ethernet controller detected.");
      break;

    default:
      Serial.print("Other Ethernet controller detected: ");
      Serial.println(Ethernet.hardwareStatus());
      break;
  }

  printChangedLinkStatus();

  if (EthernetBonjour.begin("rainsensor")) {
    Serial.println("Advertising name 'rainsensor.local'");
  } else {
    Serial.println("EthernetBonjour.begin FAILED!");
  }

  // start listening for clients
  server.begin();

  // Give the sensor and Ethernet shield time to set up:
  // NOT SURE THIS IS NECESSARY AT ALL.
  startTime = millis();
  
  delay(1000);

  // Wait for serial port to connect. Needed for native USB port only.
  while (!Serial && (millis() - startTime) < 10 * 1000UL) {
    blink(200);
  }

  // SOLELY for this demo, without real hardware, initialize the random
  // number generator's seed.
  seedRNG();

  readFakeSensors();
}

unsigned long nextBlink = 0;
bool ledIsOn = false;

void printChangedLinkStatus() {
  static bool first = true;
  static EthernetLinkStatus last_status = Unknown;

  EthernetLinkStatus status = Ethernet.linkStatus();
  if (!first && status == last_status) {
    return;
  }
  first = false;
  last_status = status;
  Serial.print("Link status: ");
  switch (status) {
    case Unknown:
      Serial.println("Unknown. Link status detection is only available with W5200 and W5500.");
      break;
    case LinkON:
      Serial.println("ON");
      break;
    case LinkOFF:
      Serial.println("OFF");
      break;
  }
}

void loop() {
  printChangedLinkStatus();

  // If got address via DHCP, this will renew the lease.
  if (using_dhcp) {
    switch (Ethernet.maintain()) {
      case 1: // Renew failed
      case 3: // Rebind failed
        Serial.println("WARNING! lost our DHCP assigned address!");
        // MIGHT want to just return at this point, since the rest won't work.
    }
  }

  // This actually runs the Bonjour module. YOU HAVE TO CALL THIS PERIODICALLY,
  // OR NOTHING WILL WORK! Preferably, call it once per loop().
  EthernetBonjour.run();

  // check for a reading no more than once a second.
  if (millis() - lastReadingTime > 1000) {
//    Serial.println("We should read from our FAKE sensor!");
    readFakeSensors();
    lastReadingTime = millis();
  }

  blink(1000);

  // listen for incoming Ethernet connections:
  listenForEthernetClients();
}

void blink(unsigned long interval) {
  static unsigned long lastBlink = 0;
  unsigned long nextBlink = lastBlink + interval;
  unsigned long now = millis();

//  Serial.print("lastBlink=");
//  Serial.print(lastBlink);
//  Serial.print(", nextBlink=");
//  Serial.print(nextBlink);
//  Serial.print(", now=");
//  Serial.print(now);

  if (nextBlink < lastBlink) {
    // Wrapped around.
    if (now >= lastBlink) {
      // Serial.println("   millis hasn't wrapped yet, so not time.");
      return;
    } else if (now < nextBlink) {
//      Serial.println("   Wrapped, but not time yet.");
      return;
    }
  } else if (now < lastBlink) {
    // Clock has wrapped around, so past due for blinking!
  } else if (now < nextBlink) {
    // Not time yet.
//    Serial.println("   Not time yet.");
    return;
  }

  digitalWrite(LED_BUILTIN, ledIsOn ? LOW : HIGH);
  ledIsOn = !ledIsOn;
  lastBlink = now;
//  Serial.println("    BLINK!");
}


void listenForEthernetClients() {
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("Got a client!!");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.print(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println();
          // print the current readings, in HTML format:
          client.print("Temperature: ");
          client.print(temperature);
          client.print(" degrees C");
          client.println("<br />");
          client.print("Pressure: " + String(pressure));
          client.print(" Pa");
          client.println("<br />");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
  }
}

void seedRNG() {
  for (int loop = 0; loop < 10; ++loop) {
    if (AnalogRandom.seedArduinoRNG()) {
      break;
    }
  }


//   // This is based on:
//   //   http://www.utopiamechanicus.com/article/better-arduino-random-numbers/
//   // which in turn is based on earlier work, at least back to Alan Turing.
//   // The idea is that we can "debias" a biased source of numbers (e.g. an
//   // unfair coin) by taking two readings at a time rather than one, each
//   // reading producing 1 bit. If the values are:
//   //   0,0: Try again
//   //   0,1: Output a 1
//   //   1,0, Output a 0
//   //   1,1: Try again.
//   // We use the analog pins as our source of biased readings. We'll cycle
//   // through all of the available analog pins just in case some are well
//   // grounded, so not producing any noise.

// #define DEBUG_SEED_RNG
// #ifdef DEBUG_SEED_RNG
//   unsigned long startMicros = micros();
// #endif  // DEBUG_SEED_RNG

//   // randomSeed takes an unsigned long, which which determines the number
//   // of bits we need.
//   int neededBits = 8 * sizeof(unsigned long);

//   // Initialize with a previously generated randomly value just in case we
//   // can't get all the bits we need (i.e. the analog pins are too stable).
//   unsigned long seed = 0x43a61ac0;

//   // ATmega's can perform analogRead about 10K times per second, so let's
//   // limit to that number of analog reads so this doesn't take too long.
//   int loopLimit = 5000;  // Two reads per loop.

//   // A lookup table from integer (pin number) to the corresponding analog pin
//   // identifier (i.e. A0 should not be assumed to be zero). Using the macro
//   // NUM_ANALOG_INPUTS to determine how many analog pins are available, and
//   // hence build the table's values.

// #ifndef NUM_ANALOG_INPUTS
// #error Expected NUM_ANALOG_INPUTS to be a C preprocessor macro.
// #endif

//   int pinTable[] = {
//     A0, A1, A2,
// #if NUM_ANALOG_INPUTS > 3
//     A3,
// #endif
// #if NUM_ANALOG_INPUTS > 4
//     A4,
// #endif
// #if NUM_ANALOG_INPUTS > 5
//     A5,
// #endif
// #if NUM_ANALOG_INPUTS > 6
//     A6,
// #endif
// #if NUM_ANALOG_INPUTS > 7
//     A7,
// #endif
// #if NUM_ANALOG_INPUTS > 8
//     A8,
// #endif
// #if NUM_ANALOG_INPUTS > 9
//     A9,
// #endif
// #if NUM_ANALOG_INPUTS > 10
//     A10,
// #endif
// #if NUM_ANALOG_INPUTS > 11
//     A11,
// #endif
// #if NUM_ANALOG_INPUTS > 12
//     A12,
// #endif
// #if NUM_ANALOG_INPUTS > 13
//     A13,
// #endif
// #if NUM_ANALOG_INPUTS > 14
//     A14,
// #endif
//   };
//   int numPins = sizeof pinTable / sizeof pinTable[0];
// #ifdef DEBUG_SEED_RNG
//   Serial.print("seedRNG: numPins=");
//   Serial.print(numPins);
//   Serial.print("   pinTable=[");
//   for (int i = 0; i < numPins; ++i) {
//     if (i > 0) {
//       Serial.print(", ");
//     }
//     Serial.print(pinTable[i], DEC);
//   }
//   Serial.println("]");
// #endif  // DEBUG_SEED_RNG

//   while ((neededBits > 0) && (loopLimit-- > 0)) {
//     int pinNum = loopLimit % numPins;
//     int pinId = pinTable[pinNum];
//     int bit0= analogRead(pinId) & 1;
//     int bit1= analogRead(pinId) & 1;
//     if (bit1!=bit0) {
//       seed = (seed<<1) | bit1;
//       --neededBits;
//     }
//   }

// #ifdef DEBUG_SEED_RNG
//   unsigned long endMicros = micros();
//   unsigned long elapsedMicros = endMicros - startMicros;
//   Serial.print("seedRNG: neededBits=");
//   Serial.print(neededBits);
//   Serial.print("   loopLimit=");
//   Serial.print(loopLimit);
//   Serial.print("   elapsedMicros=");
//   Serial.print(elapsedMicros);
//   Serial.print("   seed=0x");
//   Serial.print(seed, HEX);
//   Serial.println();
// #endif  // DEBUG_SEED_RNG

//   randomSeed(seed);
}

void readFakeSensors() {
  static bool first = true;

  float t = random(-400, 1200) / 10.0;
  float p = random(800, 1100);

  if (first) {
    first = false;
    temperature = t;
    pressure = p;
  } else {
    temperature = (temperature * 99 + t) / 100;
    pressure = (pressure * 99 + p) / 100;
  }

//  Serial.print("Temperature: ");
//  Serial.print(temperature);
//  Serial.println(" degrees F");
//  Serial.print("Pressure: ");
//  Serial.print(pressure);
//  Serial.println(" hPa");
//  Serial.println();
}
