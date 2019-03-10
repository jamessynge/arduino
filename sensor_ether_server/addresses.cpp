#include "addresses.h"

#include "analog_random.h"
#include "eeprom_io.h"

namespace {
// This is the name used to identify the data stored in the EEPROM.
// Changing the value (e.g. between "addrs" and "Addrs") has the
// effect of invalidating the currently stored values, which can
// be useful if you want to change the space from whicn one or both
// the addresses is allocated.
const char kName[] = "addrs";

// An Arduino Ethernet shield (or an freetronics EtherTen board, which I've used
// to test this code) does not have its own MAC address (the unique identifier
// used to distinguish one Ethernet device from another). Fortunately the design
// of MAC addresses allows for both globally unique addresses (i.e. assigned at
// the factory, unique world-wide) and locally unique addresses. This code will
// generate an address in the range allowed for local administered addresses and
// store it in EEPROM. Note though that there is no support here for probing to
// ensure that the allocated address is free. Read more about the issue here:
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
bool pickMACAddress(byte mac[6], AnalogRandom* rng) {
  for (int i = 0; i < 6; ++i) {
    int r = rng->randomByte();
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

// A link-local address is in the range 169.254.1.0 to 169.254.254.255,
// inclusive. Learn more: https://tools.ietf.org/html/rfc3927
bool pickIPAddress(IPAddress* output, AnalogRandom* rng) {
  auto r = rng->random32();
  if (r == 0) {
    return false;
  }
  r >>= 1;
  int c = (r % 254) + 1;

  r = rng->random32();
  if (r == 0) {
    return false;
  }
  r >>= 1;
  int d = r & 0xff;

  output[0] = 169;
  output[1] = 254;
  output[2] = c;
  output[3] = d;
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

bool Addresses::generateAddresses() {
  AnalogRandom rng;
  if (!pickMACAddress(mac, &rng)) {
    // Unable to find enough randomness.
    return false;
  }
  if (!pickIPAddress(&ip, &rng)) {
    // Unable to find enough randomness.
    return false;
  }
  return true;
}
