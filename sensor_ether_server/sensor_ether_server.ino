/*
Author: James Synge
Date: March, 2019

This is a demo of using the Arduino Ethernet and EthernetBonjour libraries,
along with randomly generating the required Ethernet MAC address and using a
randomly generated IP address if there is no DHCP server on the LAN. This is
adapted from the Arduino example:

     Ethernet > BarometricPressureWebServer

The sketch prints a lot of status to the Serial port to help the reader
understand what is going on at each step.

The sketch uses the EEPROM library to store the generated Ethernet address and a
randomly generated link-local IP address. The latter is only used if a DHCP
server is unavailable. By storing these values, the sketch uses the same values
each time it connects to the LAN, which makes working with it easier. Read more
about these in addresses.cpp.

analog_random.h/.cpp demonstrate how one can use the analog pins as source
of random bits.

eeprom_io.h/.cpp demonstrate how to write a structure to EEPROM and later
safely read it back, with a name and a checksum used to ensure that the
correct structure is being read.

TO USE:

1) Connect your Arduino to your computer (for uploading the sketch),
   and connect the Arduino Ethernet shield (or similar) to your local
   Ethernet. If necessary, connect a power supply to the Arduino (i.e.
   USB isn't enough for powering the Ethernet shield).
2) Open the Arduino IDE, select the correct type of board in the Tools
   menu, and the correct port to which your Arduino is attached.
3) Open the Serial Monitor, set it to 9600 baud.
4) Load this sketch into your Arduino IDE, compile and upload to your
   Arduino.
5) Read the messages in the Serial Monitor showing what the sketch is doing.
   For example:

14:29:35.255 -> Using MAC address: 52-C4-58-43-37-4D
14:29:35.288 -> Finding IP address via DHCP... assigned address: 192.168.86.48
14:29:35.952 -> Ethernet hardware: W5100
14:29:35.985 -> Link status: Unknown (only W5200 and W5500 can do this)
14:29:38.276 -> Advertising name: sensor_ether_server

6) Locate the board on your network by its mDNS name. For example:

  $ ping sensor_ether_server.local
  PING sensor_ether_server.local (192.168.86.48) 56(84) bytes of data.
  64 bytes from wiznet43374d.lan (192.168.86.48): icmp_seq=1 ttl=128 time=0.559 ms

7) Read from the HTTP server provided by the sketch:

  $ curl sensor_ether_server.local
  Temperature: 34.47 degrees C<br />
  Pressure: 953.67 Pa<br />

NOTE: Due to the size of the libraries, and all the print statements, this
sketch uses most of the memory in a 32KB flash. If you are adapting it for
working with your actual sensors, you may want to drop the print statements.

TODO:
* Determine if there is a way to detect whether our generated addresses
  conflict with other devices on the network.
* Figure out why D13 is blinking fast when running this sketch, instead of
  blinking once a second as requested. Some library must be affecting it.

*/

// This is the Arduino "standard" Ethernet 2.0.0 library (or later).
#include <Ethernet.h>

// I've forked this library just so that I'm sure I can find it:
// https://github.com/jamessynge/EthernetBonjour
#include <EthernetBonjour.h>

#include "addresses.h"
#include "analog_random.h"
#include "eeprom_io.h"

// Name we'll advertise using mDNS (Apple's Bonjour protocol).
const char* kMulticastDnsName = "sensor_ether_server";

// Keep track of whether we're using DHCP or not. If we are, then we need
// to renew our lease on the allocated address periodically.
bool using_dhcp = true;

// Initialize the Ethernet server library with the port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);

// Fake sensor values.
float temperature;
float pressure;

void setup() {
  // As described on the freetronics website, there is a delay between the reset
  // of the EtherTen board and the time when the Ethernet chip is allowed to
  // operate. Do some other stuff first so that we don't try initializing the
  // Ethernet chip too soon.
  //
  //       https://www.freetronics.com.au/pages/usb-power-and-reset
  unsigned long startTime = millis();

  // SOLELY for this demo, which is without real hardware, initialize the random
  // number generator's seed, which we'll use to generate fake sensor readings.
  seedRNG();

  // Load the addresses saved to EEPROM, if they were previously saved. If they
  // were not successfully loaded, then generate them.
  Addresses addresses;
  bool loadedAddresses = addresses.load();
  bool generatedAddresses = !loadedAddresses && addresses.generateAddresses();
  if (generatedAddresses) {
    // It *MAY* help you identify devices on your network as using this software
    // if they have the same "Organizationally Unique Identifier" (the first 3
    // bytes of the MAC address). Let's do that here, using values from an
    // online "locally administered mac address generator".
    addresses.mac[0] = 0x52;
    addresses.mac[1] = 0xC4;
    addresses.mac[2] = 0x58;
    addresses.save();
  }

  // Open serial communications and wait for host to start reading, but not
  // forever. This provides some time for the Arduino IDE Serial Monitor to
  // start reading before too many messages (or any) have been printed. Note
  // that many Arduinos reset when the host computer connects to the serial
  // device, so when debugging it will usually be the case that the device
  // is ready right away. And if not debugging (i.e. embedded somewhere with
  // nobody connected to the serial device), then any time we spend here is
  // just wasted... except it allows us to burn the time until the Ethernet
  // chip is available.
  Serial.begin(9600);
  unsigned long waitUntil = startTime + 200;  // Allow up to 200ms from boot.
  while (!Serial && (millis() < waitUntil)) {
    // Blink fast so that the person looking at the board can tell
    // the difference between this blink rate and some of the others.
    blink(50);
    delay(1);
  }

  if (!loadedAddresses) {
    if (!generatedAddresses) {
      announceFailure("Unable to load or generate MAC and IP addresses!");
    }
    Serial.println("Generated random MAC and IP addresses.");
  }

  Serial.print("MAC address: ");
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
      announceFailure("Sorry, can't run without Ethernet. :(");

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
      Serial.print("unfamiliar id: 0x");
      Serial.println(hwStatus, HEX);
      break;
  }

  printChangedLinkStatus();

  if (EthernetBonjour.begin(kMulticastDnsName)) {
    Serial.print("Advertising name: ");
    Serial.println(kMulticastDnsName);
  } else {
    Serial.println("EthernetBonjour.begin FAILED!");
  }

  // Choose initial values for the fake sensors.
  readFakeSensors();

  // Start listening for clients
  server.begin();
}

void printChangedLinkStatus() {
  static bool first = true;
  static EthernetLinkStatus last_status = Unknown;

  if (Ethernet.hardwareStatus() == EthernetW5100) {
    // No point in checking, a W5100 can't report link status.
    return;
  }

  EthernetLinkStatus status = Ethernet.linkStatus();
  if (!first && status == last_status) {
    return;
  }
  first = false;
  last_status = status;
  Serial.print("Link status: ");
  switch (status) {
    case Unknown:
      Serial.println("Unknown");
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

  // If we're using an IP address assigned via DHCP, renew the lease
  // periodically. The library will do so at the appropriate interval if we
  // call it often enough.
  if (using_dhcp) {
    switch (Ethernet.maintain()) {
      case 1: // Renew failed
      case 3: // Rebind failed
        Serial.println("WARNING! lost our DHCP assigned address!");
        // MIGHT want to just return at this point, since the rest won't work.
        // Or for a very robust product, use a digital output pin connected to
        // the RESET pin, and force the Arduino to reset if it loses its lease.
        // It can then start over requesting an address via DHCP, or fallback
        // to its randomly generated link-local address.
    }
  }

  // If we've received an mDNS query for our name, respond. This must be called
  // often in order for the mDNS feature to work, ideally once per loop.
  EthernetBonjour.run();

  readFakeSensors();
  blink(1000);

  // listen for incoming Ethernet connections:
  listenForEthernetClients();
}

// Something is odd about the behavior here. If I just run blink(1000) and
// nothing else, all is well, but if I run the rest of the demo too, then D13
// blinks really fast, an is off most of the time. Not sure why yet.
void blink(unsigned long interval) {
  static unsigned long lastBlink = 0;
  static bool ledIsOn = false;

  unsigned long nextBlink = lastBlink + interval;
  unsigned long now = millis();

//#define DEBUG_BLINK
#ifdef DEBUG_BLINK
  Serial.print("interval=");
  Serial.print(interval);
  Serial.print(", lastBlink=");
  Serial.print(lastBlink);
  Serial.print(", nextBlink=");
  Serial.print(nextBlink);
  Serial.print(", now=");
  Serial.print(now);
#endif  // DEBUG_BLINK

  if (nextBlink < lastBlink) {
    // Wrapped around.
    if (now >= lastBlink) {
#ifdef DEBUG_BLINK
      Serial.println("   millis hasn't wrapped yet, so not time.");
#endif  // DEBUG_BLINK
      return;
    } else if (now < nextBlink) {
#ifdef DEBUG_BLINK
      Serial.println("   Wrapped, but not time yet.");
#endif  // DEBUG_BLINK
      return;
    }
  } else if (now < lastBlink) {
    // Clock has wrapped around, so past due for blinking!
  } else if (now < nextBlink) {
    // Not time yet.
#ifdef DEBUG_BLINK
    Serial.println("   Not time yet.");
#endif  // DEBUG_BLINK
    return;
  }
  int pinValue = ledIsOn ? LOW : HIGH;
  digitalWrite(LED_BUILTIN, pinValue);
  ledIsOn = !ledIsOn;
  lastBlink = now;
#ifdef DEBUG_BLINK
  Serial.print("    BLINK!   ledIsOn=");
  Serial.print(ledIsOn);
  Serial.print("pinValue=");
  Serial.println(pinValue);
#endif  // DEBUG_BLINK
}

void listenForEthernetClients() {
  // Is there a client that connected since we last checked?
  EthernetClient client = server.available();
  if (!client) {
    return;
  }
  Serial.println("Got a client!!");
  // We take an EXTREMELY simple approach to "parsing" (consuming) the HTTP
  // request: we read until we get a blank line, as every HTTP request header
  // block ends with \r\n\r\n (i.e. two line endings). We assume that the
  // request is not a PUT or POST, i.e. has no body, and we certainly don't
  // try to read it.
  boolean currentLineIsBlank = true;
  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
      // As a debugging aid, print every character in the HTTP request header.
      Serial.print(c);
      // If you've gotten to the end of the line (received a newline
      // character) and the line is blank, the HTTP request has ended,
      // so you can send a reply.
      if (c == '\n' && currentLineIsBlank) {
        // Send a standard http response header
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html");
        client.println();
        // Print the current readings, in HTML format:
        client.print("Temperature: ");
        client.print(temperature);
        client.print(" degrees C");
        client.println("<br/>");
        client.print("Pressure: " + String(pressure));
        client.print(" Pa");
        client.println("<br/>");
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
  // Give the web browser time to receive the data.
  // NOT SURE WHY THIS IS HERE. Maybe the Ethernet chip will drop pending data
  // if we close too early?
  delay(1);
  // Close the TCP connection.
  client.stop();
}

void seedRNG() {
  AnalogRandom rng;
  for (int loop = 0; loop < 10; ++loop) {
    if (rng.seedArduinoRNG()) {
      break;
    }
  }
}

void readFakeSensors() {
  static bool first = true;
  static unsigned long lastReadingTime = 0;

  unsigned long now = millis();
  if (!first) {
    // Check for a reading no more than once a second.
    if (lastReadingTime > now) {
      // The clock has wrapped. Not attempting perfect spacing here.
      lastReadingTime = 0;
    }
    unsigned long elapsed = now - lastReadingTime;
    if (elapsed < 1000) {
      // Too soon.
      return;
    }
  } 
  lastReadingTime = now;

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

void announceFailure(const char* message) {
  while (true) {
    Serial.println(message);
    delay(1000); // do nothing, no point running without Ethernet hardware.
  }
}