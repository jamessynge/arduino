#ifndef SENSOR_ETHER_SERVER_ADDRESSES_H
#define SENSOR_ETHER_SERVER_ADDRESSES_H

#include "Ethernet.h"

void printMACAddress(byte mac[6]);

// Represents the addresses that must be generated to work with the Ethernet
// library.
struct Addresses {
  // Save this struct's fields to EEPROM.
  void save();

  // Restore this struct's fields from EEPROM. Returns true if successful,
  // false otherwse.
  bool load();

  // We generate both a MAC address (always needed) and an IPv4 address (only
  // needed if DHCP is not working) so that we will always use the same IP
  // address when DHCP isn't able to provide one. Note that there isn't support
  // for detecting conflicts with other users of the same link-local address.
  // Returns true if successful, false if not enough randomness was available
  // using AnalogRandom.
  bool generateAddresses();

  IPAddress ip;
  byte mac[6];
};

#endif  // SENSOR_ETHER_SERVER_ADDRESSES_H
