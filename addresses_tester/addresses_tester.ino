#include <Arduino.h>
#include <EEPROM.h>

#include "addresses.h"
#include "analog_random.h"
#include "eeprom_io.h"
#include "test.h"

void setup()
{
  Serial.begin(9600);
  delay(500);

  // Wipe out the contents of the EEPROM; really, just flip bits in one of
  // the first 14 bytes, will will invalidate the first load; 14 is the number
  // of bytes required to store an IP address, a MAC address and a Crc32.
  {
    AnalogRandom rnd;
    int i = rnd.random32() % 14;
    uint8_t v = EEPROM.read(i);
    EEPROM.write(i, ~v);
  }

  // Should not be able to load at the moment.
  {
    Addresses a0;
    ASSERT_FALSE(a0.load(nullptr));
  }

  // Try again with loadOrGenAndSave(nullptr), i.e. no OUI prefix.
  // This will generate ans save some values.
  Addresses a1;
  ASSERT_TRUE(a1.loadOrGenAndSave(nullptr));

  // Load again, which should produce the same values.
  {
    Addresses a2;
    ASSERT_TRUE(a2.load(nullptr));
    ASSERT_EQ(a1, a2);
  }

  // Now try loading with a different OUI prefix specified. Load will fail
  // because of the mismatch.
  OuiPrefix oui_prefix(0x52, 0xC4, ~a1.mac.mac[2]);
  {
    Addresses a3;
    ASSERT_FALSE(a3.load(&oui_prefix));
  }

  // Try again with loadOrGenAndSave, which will replace the values in
  // the EEPROM.
  Addresses a4;
  ASSERT_TRUE(a4.loadOrGenAndSave(&oui_prefix));
  ASSERT_NE(a1, a4);

  // Now try loading with the same OUI prefix as when we generated.
  {
    Addresses a5;
    ASSERT_TRUE(a5.load(nullptr));
    ASSERT_EQ(a4, a5);
  }

  Serial.println();
  Serial.println("##############################################");
  Serial.println("Test complete");
  Serial.println("##############################################");
  Serial.println();


  // OuiPrefix oui_prefix(0x52, 0xC4, ~addresses.mac.mac[2]);
  // // This shouldn't work because the above generated a MAC address with
  // // a different OUI prefix.
  // if (addresses.load(&oui_prefix)) {
  //   Serial.print("ERROR: loadOrGenAndSave with-prefix worked: ");
  //   addresses.println();
  // }

  // // Try again, but generate and save when there isn't a match.
  // if (addresses.loadOrGenAndSave(&oui_prefix)) {
  //   Serial.print("loadOrGenAndSave(with-prefix) worked: ");
  //   addresses.println();
  // } else {
  //   Serial.println("loadOrGenAndSave(with-prefix) failed");
  // }
  // addresses.loadOrGenAndSave



  // if (!addresses.load()) {
  //   if (!addresses.generateAddresses(oui_prefix)) {
  //     return false;
  //   }
  //   Serial.print("Generated: ");
  //   addresses.println();
  //   addresses.save();
  //   Serial.print("Saved: ");
  //   addresses.println();
  // }

  // Serial.print("MAC: ");
  // // printMACAddress(addresses.mac);
  // Serial.println(addresses.mac);

}

void loop()
{
  
}
