#ifndef SENSOR_ETHER_SERVER_ADDRESSES_H
#define SENSOR_ETHER_SERVER_ADDRESSES_H

#include "Ethernet.h"

void printMACAddress(byte mac[6]);

struct Addresses {
  void save();
  bool load();

  // We generate both a MAC address (always needed) and an IPv4 address (only
  // needed if DHCP is not working) so that we will always use the same IP
  // address when DHCP isn't able to provide one. Note that there isn't support
  // for detecting conflicts with other users of the same link-local address.
  void generateAddresses();

  IPAddress ip;
  byte mac[6];
};

#endif  // SENSOR_ETHER_SERVER_ADDRESSES_H
