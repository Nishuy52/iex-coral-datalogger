// 02_I2C_Scanner - Stage 3 & Stage 7 verification for the 2-Part ProMini Logger
// ------------------------------------------------------------------------------
// PURPOSE: confirms the I2C bus wiring between the ProMini and the RTC module
// (A4->SDA, A5->SCL crossover, VCC, GND) and later that the deployment sensors
// (BMP280, BH1750) are connected and answering.
//
// BOARD SETTINGS:  Arduino Pro or Pro Mini / ATmega328P (3.3V, 8MHz)
// SERIAL MONITOR:  500000 baud
//
// EXPECTED OUTPUT after Stage 3 (logger core only):
//     0x57  EEprom 4k (RTC module)      <- REQUIRED
//     0x68  DS3231 RTC                  <- REQUIRED
//     CORE CHECK: PASS
// EXPECTED after Stage 7 adds the sensors:
//     0x23  BH1750 light sensor
//     0x76  BMP280 pressure/temp sensor
//
// If CORE CHECK: FAIL -> re-check the four solder joints between the boards.
// Remember SDA/SCL must CROSS OVER between the ProMini and the RTC module.
// No libraries required beyond Wire.

#include <Wire.h>

const char* deviceName(byte addr) {
  switch (addr) {
    case 0x23: return "BH1750 light sensor [deploy sensor]";
    case 0x5C: return "BH1750 light sensor (ADDR pin high)";
    case 0x50: return "32k/64k EEprom module [optional]";
    case 0x57: return "EEprom 4k (RTC module) ** REQUIRED **";
    case 0x68: return "DS3231 RTC             ** REQUIRED **";
    case 0x76: return "BMP280 pressure/temp [deploy sensor]";
    case 0x77: return "BMP280 (SDO pin high)";
    case 0x3C: case 0x3D: return "SSD1306 OLED [optional]";
    case 0x40: return "Si7051 temp [optional]";
    case 0x44: case 0x45: return "SHT3x humidity [optional]";
    default:   return "unknown device";
  }
}

void setup() {
  Serial.begin(500000);
  Wire.begin();
  Serial.println();
  Serial.println(F("====================================================="));
  Serial.println(F(" 02_I2C_Scanner - bus wiring check (rescans every 3s)"));
  Serial.println(F(" CORE CHECK requires BOTH 0x68 (RTC) and 0x57 (EEprom)"));
  Serial.println(F("====================================================="));
}

void loop() {
  byte found = 0;
  bool haveRTC = false, haveEEprom = false, haveBMP = false, haveBH = false;

  Serial.println(F("\nScanning 0x08-0x77 ..."));
  for (byte addr = 0x08; addr <= 0x77; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      found++;
      Serial.print(F("  0x"));
      if (addr < 16) Serial.print(F("0"));
      Serial.print(addr, HEX);
      Serial.print(F("  "));
      Serial.println(deviceName(addr));
      if (addr == 0x68) haveRTC = true;
      if (addr == 0x57) haveEEprom = true;
      if (addr == 0x76 || addr == 0x77) haveBMP = true;
      if (addr == 0x23 || addr == 0x5C) haveBH = true;
    }
    delay(2);
  }

  if (found == 0) Serial.println(F("  -- no devices found! check VCC/GND/SDA/SCL joints --"));

  Serial.print(F("CORE CHECK:   "));
  Serial.println((haveRTC && haveEEprom) ? F("PASS") : F("FAIL  <- fix before continuing!"));
  Serial.print(F("SENSOR CHECK: BMP280 "));
  Serial.print(haveBMP ? F("found") : F("absent"));
  Serial.print(F(", BH1750 "));
  Serial.print(haveBH ? F("found") : F("absent"));
  Serial.println(F("   (absent = PASS until Stage 7)"));

  delay(3000);
}
