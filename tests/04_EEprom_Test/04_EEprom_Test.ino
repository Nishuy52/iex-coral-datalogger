// 04_EEprom_Test - Stage 3 verification for the 2-Part ProMini EEprom Logger
// ----------------------------------------------------------------------------
// PURPOSE: proves every byte of each I2C EEprom can be written and read back.
// These chips store all your sensor data, so a single bad byte means lost
// records in the field. The test runs on:
//   - the 4k AT24C32 on the RTC module (0x57) - ALWAYS tested
//   - an external EEprom on the bus (0x50)    - tested IF present (Appendix B)
//
// For each chip:
//   Pass 1: writes a known pattern to every byte
//   Pass 2: reads it all back and counts mismatches
//   Pass 3: erases the chip back to 0xFF (factory state)
//
// *** DESTRUCTIVE: this overwrites any logged data on the EEprom(s)! ***
// Run it during assembly/QC only, never after a deployment you care about.
// The sketch waits for you to type Y in the serial monitor before starting.
//
// BOARD SETTINGS:  Arduino Pro or Pro Mini / ATmega328P (3.3V, 8MHz)
// SERIAL MONITOR:  500000 baud
//
// SET the external chip size below (presence itself is auto-detected):
//   AT24C256 (32k) -> 32768UL    AT24C512 (64k) -> 65536UL    none -> 0UL
//
// EXPECTED: "ALL EEPROM TESTS: PASS" - a few seconds for the 4k, up to ~1 min
// for a 64k. No libraries required beyond Wire.

#include <Wire.h>

#define INTERNAL_ADDR   0x57       // 4k AT24C32 on the RTC module (always present)
#define INTERNAL_BYTES  4096UL
#define EXTERNAL_ADDR   0x50       // external EEprom module/chip (Appendix B)
#define EXTERNAL_BYTES  32768UL    // <-- SET THIS: 32768UL (32k), 65536UL (64k), or 0UL to skip
#define CHUNK           16         // bytes per I2C write: fits the 32-byte Wire buffer
                                   // (2 addr + 16 data) and divides evenly into both the
                                   // 32-byte (4k) and 64-byte (32k/64k) hardware pages

// wait for the EEprom's internal self-timed write cycle (~5-10ms) to finish:
// the chip won't ACK its address until done
bool ackPoll(uint8_t addr) {
  for (uint8_t tries = 0; tries < 50; tries++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) return true;
    delay(1);
  }
  return false;
}

bool present(uint8_t addr) {
  Wire.beginTransmission(addr);
  return (Wire.endTransmission() == 0);
}

bool writeChunk(uint8_t addr, uint16_t memAddr, bool erase) {
  Wire.beginTransmission(addr);
  Wire.write(highByte(memAddr));
  Wire.write(lowByte(memAddr));
  for (uint8_t i = 0; i < CHUNK; i++) {
    uint16_t a = memAddr + i;
    Wire.write(erase ? 0xFF : (uint8_t)((a * 7 + 13) & 0xFF));
  }
  if (Wire.endTransmission() != 0) return false;
  return ackPoll(addr);
}

void readChunk(uint8_t addr, uint16_t memAddr, uint8_t *buf) {
  Wire.beginTransmission(addr);
  Wire.write(highByte(memAddr));
  Wire.write(lowByte(memAddr));
  Wire.endTransmission();
  Wire.requestFrom(addr, (uint8_t)CHUNK);
  for (uint8_t i = 0; i < CHUNK; i++) buf[i] = Wire.read();
}

// returns the mismatch count for this chip (0 = PASS)
uint16_t testEeprom(uint8_t addr, uint32_t bytes, const __FlashStringHelper *label) {
  uint16_t mismatches = 0;
  uint8_t buf[CHUNK];
  uint32_t startMs = millis();

  Serial.print(F("\n--- Testing ")); Serial.print(label);
  Serial.print(F(" at 0x")); Serial.print(addr, HEX);
  Serial.print(F(", ")); Serial.print(bytes); Serial.println(F(" bytes ---"));

  Serial.print(F("Pass 1 - writing pattern "));
  for (uint32_t a = 0; a < bytes; a += CHUNK) {
    if (!writeChunk(addr, (uint16_t)a, false)) {
      Serial.print(F("\nWRITE ERROR at 0x")); Serial.println(a, HEX);
      mismatches++;
    }
    if (a % 512 == 0) Serial.print(F("."));
  }

  Serial.print(F("\nPass 2 - verifying      "));
  for (uint32_t a = 0; a < bytes; a += CHUNK) {
    readChunk(addr, (uint16_t)a, buf);
    for (uint8_t i = 0; i < CHUNK; i++) {
      uint8_t expected = (uint8_t)(((a + i) * 7 + 13) & 0xFF);
      if (buf[i] != expected) {
        if (mismatches < 10) {                       // report first few only
          Serial.print(F("\nMISMATCH at 0x")); Serial.print(a + i, HEX);
          Serial.print(F(" wrote 0x"));  Serial.print(expected, HEX);
          Serial.print(F(" read 0x"));   Serial.print(buf[i], HEX);
        }
        mismatches++;
      }
    }
    if (a % 512 == 0) Serial.print(F("."));
  }

  Serial.print(F("\nPass 3 - erasing to 0xFF"));
  for (uint32_t a = 0; a < bytes; a += CHUNK) {
    writeChunk(addr, (uint16_t)a, true);
    if (a % 512 == 0) Serial.print(F("."));
  }

  uint32_t elapsed = (millis() - startMs) / 1000;
  Serial.print(F("\n  result: "));
  Serial.print(mismatches == 0 ? F("PASS") : F("FAIL"));
  Serial.print(F("  (")); Serial.print(mismatches); Serial.print(F(" mismatches, "));
  Serial.print(elapsed); Serial.println(F("s)"));
  return mismatches;
}

void setup() {
  Serial.begin(500000);
  Wire.begin();
  Serial.println();
  Serial.println(F("====================================================="));
  Serial.println(F(" 04_EEprom_Test - full write/read/erase check"));
  Serial.println(F(" ** DESTRUCTIVE: overwrites all logged data! **"));
  Serial.println(F(" Type Y in the serial monitor to start."));
  Serial.println(F("====================================================="));

  bool haveInternal = present(INTERNAL_ADDR);
  bool haveExternal = (EXTERNAL_BYTES > 0) && present(EXTERNAL_ADDR);

  Serial.print(F(" 0x")); Serial.print(INTERNAL_ADDR, HEX);
  Serial.print(F(" (4k RTC module): "));
  Serial.println(haveInternal ? F("found") : F("NOT FOUND - run 02_I2C_Scanner!"));
  Serial.print(F(" 0x")); Serial.print(EXTERNAL_ADDR, HEX);
  Serial.print(F(" (external EEprom): "));
  if (EXTERNAL_BYTES == 0) Serial.println(F("skipped (EXTERNAL_BYTES = 0)"));
  else Serial.println(haveExternal ? F("found") : F("absent (none fitted = OK)"));

  if (!haveInternal && !haveExternal) {
    Serial.println(F("\nFATAL: no EEprom responding! Check wiring."));
    while (true) { delay(1000); }
  }

  while (true) {                                     // wait for consent
    if (Serial.available()) {
      char c = Serial.read();
      if (c == 'Y' || c == 'y') break;
    }
  }

  uint16_t total = 0;
  if (haveInternal) total += testEeprom(INTERNAL_ADDR, INTERNAL_BYTES, F("4k RTC EEprom"));
  if (haveExternal) total += testEeprom(EXTERNAL_ADDR, EXTERNAL_BYTES, F("external EEprom"));

  Serial.println(F("\n========= SUMMARY ========="));
  Serial.print(F(" ALL EEPROM TESTS: "));
  Serial.println(total == 0 ? F("PASS") : F("FAIL"));
  Serial.println(F("==========================="));
}

void loop() { }   // single-shot test: press reset to run again
