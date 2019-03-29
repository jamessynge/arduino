#ifndef SENSOR_ETHER_SERVER_ADDRESSES_H
#define SENSOR_ETHER_SERVER_ADDRESSES_H

#include "Arduino.h"

#include <inttypes.h>

#include "Ethernet.h"
#include "eeprom_io.h"

void printMACAddress(byte mac[6]);


// Organizationally Unique Identifier: first 3 bytes of a MAC address that is
// NOT a globally unique address. The 
struct OuiPrefix : Printable {
  // Ctor will ensure that the bit marking this as an OUI is set, and that
  // the multicast address bit is cleared.
  OuiPrefix();
  OuiPrefix(uint8_t a, uint8_t b, uint8_t c);
  size_t printTo(Print&) const override;  

  byte bytes[3];
};

struct MacAddress : Printable {
  bool generateAddress(const OuiPrefix* oui_prefix=nullptr);
  size_t printTo(Print&) const override;

  // Saves to the specified address in the EEPROM; returns the address after
  // the saved MAC address.
  int save(int toAddress, eeprom_io::Crc32* crc) const;

  // Reads from the specified address in the EEPROM; returns the address after
  // the restored MAC address.
  int read(int fromAddress, eeprom_io::Crc32* crc);

  // Returns true if the first 3 bytes match the specified prefix.
  bool hasOuiPrefix(const OuiPrefix& oui_prefix) const;

  bool operator==(const MacAddress& other) const;

  byte mac[6];
};

class SaveableIPAddress : public IPAddress {
public:
  // Inherit the base class constructors.
  using IPAddress::IPAddress;

  // Saves to the specified address in the EEPROM; returns the address after
  // the saved value.
  int save(int toAddress, eeprom_io::Crc32* crc) const;

  // Reads from the specified address in the EEPROM; returns the address after
  // the restored value.
  int read(int fromAddress, eeprom_io::Crc32* crc);
};

// The pair of addresses (MAC and IP) that we must provide to the Ethernet
// library.
struct Addresses : Printable {
  // Load the saved addresses, which must have the oui_prefix if specified;
  // if unable to load them (not stored or wrong prefix), generate addresses
  // and store them in the EEPROM.
  bool loadOrGenAndSave(const OuiPrefix* oui_prefix);

  // Save this struct's fields to EEPROM at address 0.
  void save() const;

  // Restore this struct's fields from EEPROM, starting at address 0.
  // Returns true if successful (i.e. the named and CRC matched),
  // false otherwse.
  bool load(const OuiPrefix* oui_prefix);

  // We generate both a MAC address (always needed) and an IPv4 address (only
  // used if DHCP is not working) so that we will always use the same IP
  // address when DHCP isn't able to provide one. Note that there isn't support
  // for detecting conflicts with other users of the same link-local address.
  // Returns true if successful, false if not enough randomness was available.
  bool generateAddresses(const OuiPrefix* oui_prefix);

  // Print the addresses, preceded by a prefix (if provided) and followed by a
  // newline.
  void println(const char* prefix=nullptr) const;
  size_t printTo(Print&) const override;

  bool operator==(const Addresses& other) const;

  SaveableIPAddress ip;
  MacAddress mac;
};

#endif  // SENSOR_ETHER_SERVER_ADDRESSES_H
