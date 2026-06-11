// 04_EEprom_Test - Stage 3 verification for the 2-Part ProMini EEprom Logger
// ----------------------------------------------------------------------------
// PURPOSE: proves every byte of the 4k EEprom on the RTC module (AT24C32 at
// 0x57) can be written and read back. This is the chip that stores all your
// sensor data, so a single bad byte means lost records in the field.
//
//   Pass 1: writes a known pattern to all 4096 bytes
//   Pass 2: reads everything back and counts mismatches
//   Pass 3: erases the chip back to 0xFF (factory state)
//
// *** DESTRUCTIVE: this overwrites any logged data on the EEprom! ***
// Run it during assembly/QC only, never after a deployment you care about.
// The sketch waits for you to type Y in the serial monitor before starting.
//
// BOARD SETTINGS:  Arduino Pro or Pro Mini / ATmega328P (3.3V, 8MHz)
// SERIAL MONITOR:  500000 baud
//
// EXPECTED: "EEPROM TEST: PASS  (0 mismatches)" in roughly 15-30 seconds.
// To test an [OPTIONAL] 32k module instead: set EEPROM_ADDR 0x50 and
// EEPROM_BYTES 32768 below.
// No libraries required beyond Wire.

#include <Wire.h>

#define EEPROM_ADDR  0x57     // 4k AT24C32 on the RTC module
#define EEPROM_BYTES 4096UL   // 32k module: use 0x50 and 32768UL
#define CHUNK        16       // bytes per I2C write: fits the 32-byte Wire
                              // buffer (2 addr bytes + 16 data) and divides
                              // evenly into the AT24C32's 32-byte pages so
                              // no write ever crosses a page boundary

uint16_t mismatches = 0;

// wait for the EEprom's internal self-timed write cycle (~5-10ms) to finish:
// the chip won't ACK its address until done
bool ackPoll() {
  for (uint8_t tries = 0; tries < 50; tries++) {
    Wire.beginTransmission(EEPROM_ADDR);
    if (Wire.endTransmission() == 0) return true;
    delay(1);
  }
  return false;
}

bool writeChunk(uint16_t memAddr, bool erase) {
  Wire.beginTransmission(EEPROM_ADDR);
  Wire.write(highByte(memAddr));
  Wire.write(lowByte(memAddr));
  for (uint8_t i = 0; i < CHUNK; i++) {
    uint16_t a = memAddr + i;
    Wire.write(erase ? 0xFF : (uint8_t)((a * 7 + 13) & 0xFF));
  }
  if (Wire.endTransmission() != 0) return false;
  return ackPoll();
}

void readChunk(uint16_t memAddr, uint8_t *buf) {
  Wire.beginTransmission(EEPROM_ADDR);
  Wire.write(highByte(memAddr));
  Wire.write(lowByte(memAddr));
  Wire.endTransmission();
  Wire.requestFrom((uint8_t)EEPROM_ADDR, (uint8_t)CHUNK);
  for (uint8_t i = 0; i < CHUNK; i++) buf[i] = Wire.read();
}

void progressDot(uint32_t addr) {
  if (addr % 512 == 0) { Serial.print(F(".")); }   // one dot per 512 bytes
}

void setup() {
  Serial.begin(500000);
  Wire.begin();
  Serial.println();
  Serial.println(F("====================================================="));
  Serial.println(F(" 04_EEprom_Test - full write/read/erase check"));
  Serial.print  (F(" Target: 0x")); Serial.print(EEPROM_ADDR, HEX);
  Serial.print  (F(", ")); Serial.print(EEPROM_BYTES); Serial.println(F(" bytes"));
  Serial.println(F(" ** DESTRUCTIVE: overwrites all logged data! **"));
  Serial.println(F(" Type Y in the serial monitor to start."));
  Serial.println(F("====================================================="));

  Wire.beginTransmission(EEPROM_ADDR);
  if (Wire.endTransmission() != 0) {
    Serial.println(F("FATAL: EEprom not responding! Run 02_I2C_Scanner first."));
    while (true) { delay(1000); }
  }

  while (true) {                                   // wait for consent
    if (Serial.available()) {
      char c = Serial.read();
      if (c == 'Y' || c == 'y') break;
    }
  }

  uint32_t startMs = millis();

  Serial.print(F("\nPass 1 - writing pattern "));
  for (uint32_t addr = 0; addr < EEPROM_BYTES; addr += CHUNK) {
    if (!writeChunk(addr, false)) {
      Serial.print(F("\nWRITE ERROR at 0x")); Serial.println(addr, HEX);
      mismatches++;
    }
    progressDot(addr);
  }

  Serial.print(F("\nPass 2 - verifying      "));
  uint8_t buf[CHUNK];
  for (uint32_t addr = 0; addr < EEPROM_BYTES; addr += CHUNK) {
    readChunk(addr, buf);
    for (uint8_t i = 0; i < CHUNK; i++) {
      uint8_t expected = (uint8_t)(((addr + i) * 7 + 13) & 0xFF);
      if (buf[i] != expected) {
        if (mismatches < 10) {                     // report first few only
          Serial.print(F("\nMISMATCH at 0x")); Serial.print(addr + i, HEX);
          Serial.print(F(" wrote 0x"));  Serial.print(expected, HEX);
          Serial.print(F(" read 0x"));   Serial.print(buf[i], HEX);
        }
        mismatches++;
      }
    }
    progressDot(addr);
  }

  Serial.print(F("\nPass 3 - erasing to 0xFF"));
  for (uint32_t addr = 0; addr < EEPROM_BYTES; addr += CHUNK) {
    writeChunk(addr, true);
    progressDot(addr);
  }

  uint32_t elapsed = (millis() - startMs) / 1000;
  Serial.println();
  Serial.println(F("\n========= SUMMARY ========="));
  Serial.print(F(" EEPROM TEST: "));
  if (mismatches == 0) { Serial.print(F("PASS")); } else { Serial.print(F("FAIL")); }
  Serial.print(F("  (")); Serial.print(mismatches); Serial.println(F(" mismatches)"));
  Serial.print(F(" ")); Serial.print(EEPROM_BYTES); Serial.print(F(" bytes in "));
  Serial.print(elapsed); Serial.println(F("s"));
  Serial.println(F("==========================="));
}

void loop() { }   // single-shot test: press reset to run again
