#ifndef SENSOR_ETHER_SERVER_EEPROM_IO_H
#define SENSOR_ETHER_SERVER_EEPROM_IO_H

// Helpers for writing a struct to EEPROM and later reading it back.
// For now limited to a struct at offset zero in the EEPROM.
// We store 3 values:
// 1) Name (without trailing nul), which is used later to confirm that
//    the correct struct is being read.
// 2) CRC of the bytes (or struct) (4 bytes)
// 3) The bytes (or struct).

#include "Arduino.h"
#include <inttypes.h>

// There is no check that the data fits in the EEPROM; the caller must
// ensure that.
void saveBytesToEEPROM(const char* name, const uint8_t* src, size_t numBytes);
template <class T>
void saveStructToEEPROM(const char* name, const T& src) {
  saveBytesToEEPROM(name, reinterpret_cast<const uint8_t*>(&src), sizeof src);
}

// 
bool readBytesFromEEPROM(const char* name, size_t numBytes, uint8_t* dest);
template <class T>
bool readStructFromEEPROM(const char* name, T* dest) {
  return readBytesFromEEPROM(name, sizeof *dest, reinterpret_cast<uint8_t*>(dest));
}

#endif  // SENSOR_ETHER_SERVER_EEPROM_IO_H
