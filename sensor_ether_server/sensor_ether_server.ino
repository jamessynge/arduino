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

#include "addresses.h"
#include "analog_random.h"
#include "eeprom_io.h"

// // If we can't find a DHCP server to assign an address to us, we'll
// // fallback to this static address.
// IPAddress static_ip(192, 168, 86, 251);
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
    // Blink fast so that the person looking at the board can tell
    // the difference between this blink rate and some of the others.
    blink(200);
  }

  Addresses addresses;
  if (!addresses.load()) {
    Serial.println("Was NOT able to load Addresses.");
    addresses.generateAddresses();

    // It *MAY* you identify devices on your network as using this software if
    // they share a "Organizationally Unique Identifier" (the first 3 bytes of
    // the MAC address). Let's do that here that, using values generated using
    // a value from an online "locally administered mac address generator".
    addresses.mac[0] = 0x52;
    addresses.mac[1] = 0xC4;
    addresses.mac[2] = 0x58;

    addresses.save();
  }
  Serial.print("Using MAC address: ");
  printMACAddress(addresses.mac);
  Serial.println();

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

  Serial.print("Finding IP address via DHCP... ");
  if (Ethernet.begin(addresses.mac)) {
    using_dhcp = true;
    Serial.print("assigned address: ");
    Serial.println(Ethernet.localIP());
  } else {
    using_dhcp = false;
    Serial.print("no response! Using fixed address: ");
    Serial.println(addresses.ip);
    Ethernet.setLocalIP(addresses.ip);

    // The link-local address range must not be divided into smaller
    // subnets, so we set our subnet mask accordingly:
    IPAddress subnet(255, 255, 0, 0);
    Ethernet.setSubnetMask(subnet);

    // Assume that the gateway is on the same subnet, at address 1 within
    // the subnet. This code will work with many subnets, not just a /16.
    IPAddress gateway = addresses.ip;
    gateway[0] &= subnet[0];
    gateway[1] &= subnet[1];
    gateway[2] &= subnet[2];
    gateway[3] &= subnet[3];
    gateway[3] |= 1;
    Ethernet.setGatewayIP(gateway);
  }

  Serial.print("Ethernet hardware: ");
  auto hwStatus = Ethernet.hardwareStatus();
  switch(hwStatus) {
    case EthernetNoHardware:
      Serial.println("NOT FOUND!");
      while (true) {
        Serial.println("Sorry, can't run without Ethernet. :(");
        delay(1000); // do nothing, no point running without Ethernet hardware.
      }

    case EthernetW5100:
      Serial.println("W5100");
      break;

    case EthernetW5200:
      Serial.println("W5200");
      break;

    case EthernetW5500:
      Serial.println("W5500");
      break;

    default:
      Serial.print("unfamiliar device detected, id: 0x");
      Serial.println(hwStatus, HEX);
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
      Serial.println("Unknown (only W5200 and W5500 can do this)");
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
