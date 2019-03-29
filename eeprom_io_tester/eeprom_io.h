#ifndef SENSOR_ETHER_SERVER_EEPROM_IO_H
#define SENSOR_ETHER_SERVER_EEPROM_IO_H

// Helpers for writing an object to EEPROM and later reading it back.
//
// We store 3 values:
// 1) CRC of the name and the value (4 bytes)
// 2) Name (without trailing nul), which is used later to confirm that
//    the correct object is being read.
// 3) The object.
//
// There is no check that the data fits in the EEPROM; the caller must
// ensure that.

#include "Arduino.h"
#include <inttypes.h>










// // There is no check that the data fits in the EEPROM; the caller must
// // ensure that.
// void saveBytesToEEPROM(const char* name, const uint8_t* src, size_t numBytes);
// template <class T>
// void saveStructToEEPROM(const char* name, const T& src) {
//   saveBytesToEEPROM(name, reinterpret_cast<const uint8_t*>(&src), sizeof src);
// }

// // 
// bool readBytesFromEEPROM(const char* name, size_t numBytes, uint8_t* dest);
// template <class T>
// bool readStructFromEEPROM(const char* name, T* dest) {
//   return readBytesFromEEPROM(name, sizeof *dest, reinterpret_cast<uint8_t*>(dest));
// }

class Persistable {
public:
  virtual ~Persistable() {}

  virtual void ReadFrom(EEPROMCursor* cursor) = 0;
  virtual void SaveTo(EEPROMCursor* cursor) const = 0;
};

class PersistableWrapper : public Persistable {
public:
  PersistableWrapper(uint8_t* data, size_t numBytes)
      : data_(data), numBytes_(numBytes) {}

  void ReadFrom(EEPROMCursor* cursor) override;
  void SaveTo(EEPROMCursor* cursor) const override;

private:
  uint8_t* data_;
  size_t numBytes_;
};

class EepromIo {
public:
  EepromIo(int address=0) : address_(address) {}

  void set_address(int address) { address_ = address; }
  int address() { return address_; }

private:
  void readBytes(size_t numBytes, uint8_t* dest);
  int address_;
};

class EEPROMCursor : EepromIo {
public:
  EEPROMCursor(int address=0) : EepromIo(address) {}

  using set_address;
  using address;

  // Read a part of an object into *dest. The object must not be polymorphic
  // (i.e. must not have virtual functions).
  template<typename T>
  bool readNamedObject(const char* name, T* dest) {
    PersistableWrapper pw(dest, sizeof(T));
    return readNamedObject(name, &pw);
  }

  template<>
  bool readNamedObject(const char* name, Persistable* dest);



  template<typename T>
  void readPart(T* dest) {

  }

  template<>
  void readPart(Persistable* dest) {
    dest->ReadFrom(this);
  }

Persistable

   T &get( int idx, T &t ){
    EEPtr e = idx;
    uint8_t *ptr = (uint8_t*) &t;
    for( int count = sizeof(T) ; count ; --count, ++e )  *ptr++ = *e;
    return t;
  }
  
  template< typename T > const T &put( int idx, const T &t ){
    EEPtr e = idx;
    const uint8_t *ptr = (const uint8_t*) &t;
    for( int count = sizeof(T) ; count ; --count, ++e )  (*e).update( *ptr++ );
    return t;
  }

private:
  void readBytes(size_t numBytes, uint8_t* dest);
  bool readNamedPersistable(const char* name, Persistable* dest);
  // Returns the length of the name if matches starting at address_;
  // returns -1 if the name doesn't match. name must be non-null.
  int verifyName(const char* name);

  int address_;
};




#endif  // SENSOR_ETHER_SERVER_EEPROM_IO_H
