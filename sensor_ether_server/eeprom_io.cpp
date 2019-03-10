#include "eeprom_io.h"

#include <EEPROM.h>

namespace {

constexpr int kSizeOfCrcValue = static_cast<int>(sizeof(unsigned long));

// Based on https://www.arduino.cc/en/Tutorial/EEPROMCrc:
static const unsigned long kCrcTable[16] = {
  0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
  0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
  0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
  0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
};

class CRC {
public:
  void appendByte(uint8_t v) {
    value_ = kCrcTable[(value_ ^ v) & 0x0f] ^ (value_ >> 4);
    value_ = kCrcTable[(value_ ^ (v >> 4)) & 0x0f] ^ (value_ >> 4);
    value_ = ~value_;
  }

  unsigned long value() { return value_; }

private:
  unsigned long value_ = ~0L;
};

unsigned long crcEEPROMRange(int start, int length) {
  CRC crc;
  for (int offset = 0; offset < length; ++offset) {
    crc.appendByte(EEPROM[start + offset]);
  }
  return crc.value();
}

unsigned long crcBytes(const uint8_t* src, size_t numBytes) {
  CRC crc;
  for (int offset = 0; offset < numBytes; ++offset) {
    crc.appendByte(*src++);
  }
  return crc.value();
}

int putName(const char* name) {
  int numBytes = 0;
  while (*name != 0) {
    EEPROM.put(numBytes++, *name++);
  }
  return numBytes;
}

void putBytes(int address, const uint8_t* src, size_t numBytes) {
  while (numBytes-- > 0) {
    EEPROM.update(address++, *src++);
  }
}

void getBytes(int address, size_t numBytes, uint8_t* dest) {
  while (numBytes-- > 0) {
    *dest++ = EEPROM.read(address++);
  }
}

}  // namespace

void saveBytesToEEPROM(const char* name, const uint8_t* src, size_t numBytes) {
  int nameLen = putName(name);
  unsigned long crc = crcBytes(src, numBytes);
  EEPROM.put(nameLen, crc);
  putBytes(nameLen + kSizeOfCrcValue, src, numBytes);
}

bool readBytesFromEEPROM(const char* name, size_t numBytes, uint8_t* dest) {
  // Confirm the name matches.
  int nameLen = 0;
  while (*name != 0) {
    char c;
    EEPROM.get(nameLen++, c);
    if (c != *name++) {
      // Names don't match.
      return false;
    }
  }
  // Yup, the name matches. Read the CRC value.
  unsigned long stored_crc;
  EEPROM.get(nameLen, stored_crc);
  // Confirm that the stored data has the same CRC.
  int dataStart = nameLen + kSizeOfCrcValue;
  unsigned long computed_crc = crcEEPROMRange(dataStart, numBytes);
  if (stored_crc != computed_crc) {
    // CRC values don't match. Might mean that the struct has changed size,
    // i.e. there might be missing values.
    return false;
  }
  // Looks good, copy numBytes from the EEPROM into dest.
  getBytes(dataStart, numBytes, dest);
  return true;
}
