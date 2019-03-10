#include "addresses.h"

#include "analog_random.h"
#include "eeprom_io.h"

namespace {
const char kName[] = "addrs";

// Pick a MAC address for the Ethernet interface. Since the Arduino
// doesn't have its own MAC address assigned at the factory, we must
// pick one. Read more about the issue here:
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
// So, we ensure the low-nibble of the first byte is one of these 8
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
bool pickMACAddress(byte mac[6]) {
  for (int i = 0; i < 6; ++i) {
    int r = AnalogRandom.randomByte();
    if (r < 0) {
      return false;
    }
    mac[i] = r;
  }
  // Make sure this is in the local administered space.
  mac[0] |= 2;
  // And not a multicast address.
  mac[0] &= ~1;
  return true;
}

}  // namespace

void printMACAddress(byte mac[6]) {
  Serial.print(mac[0], HEX);
  for (int i = 1; i < 6; ++i) {
    Serial.print("-");
    Serial.print(mac[i], HEX);
  }
}

void Addresses::save() {
  saveStructToEEPROM(kName, *this);
}

bool Addresses::load() {
  return readStructFromEEPROM(kName, this);
}

void Addresses::generateAddresses() {
  pickMACAddress(mac);

  // A link-local address is in the range 169.254.1.0 to 169.254.254.255,
  // inclusive. Learn more: https://tools.ietf.org/html/rfc3927
  // Also known as Automatic Private IP Addressing.
  int c;
  do {
    c = AnalogRandom.randomByte();
  } while (!(c >= 1 && c <= 254));
  int d;
  while ((d = AnalogRandom.randomByte()) < 0);
  ip = IPAddress(169, 254, c, d);
}
