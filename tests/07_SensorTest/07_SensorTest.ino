// 07_SensorTest - Stage 7 verification for the 2-Part ProMini EEprom Logger
// ----------------------------------------------------------------------------
// PURPOSE: confirms the two deployment sensors respond and produce plausible,
// *changing* readings before the DEPLOY profile is flashed:
//   - BMP280 pressure/temperature sensor (temperature only, per this build)
//   - BH1750 ambient light sensor
//
// Uses the SAME libraries and calls as the main logger sketch, so a PASS here
// means the DEPLOY profile will talk to the sensors identically.
//
// BOARD SETTINGS:  Arduino Pro or Pro Mini / ATmega328P (3.3V, 8MHz)
// SERIAL MONITOR:  500000 baud
// REQUIRES libraries (both via Library Manager):
//   hp_BH1750    [by Stefan Armborst]
//   ForcedBMX280 [by soylentOrange]   (drives the BMP280 in forced mode)
//
// EXPECTED OUTPUT: a reading line every 2 seconds. PASS when:
//   - neither sensor reports MISSING
//   - BMP280 temp is plausible (-10..50C) and RISES when you hold a
//     fingertip on the sensor for ~20s
//   - BH1750 lux DROPS near 0 when you cover it, jumps under a lamp
// Values that are plausible but never change = wiring OK, sensor dead.

#include <Wire.h>
#include <hp_BH1750.h>
#include <forcedBMX280.h>

#define Bh1750_Address 0x23

hp_BH1750 bh1750;
ForcedBME280 climateSensor = ForcedBME280();   // same class the main sketch
                                               // uses for a BMP280 (temp/pressure)
bool bhOK = false, bmpOK = false;

void setup() {
  Serial.begin(500000);
  Wire.begin();
  Serial.println();
  Serial.println(F("====================================================="));
  Serial.println(F(" 07_SensorTest - BMP280 + BH1750 deployment sensors"));
  Serial.println(F(" finger on BMP280 -> temp must rise"));
  Serial.println(F(" cover BH1750     -> lux must drop toward 0"));
  Serial.println(F("====================================================="));

  bhOK = bh1750.begin(Bh1750_Address);
  if (!bhOK) Serial.println(F("BH1750: MISSING at 0x23 - check wiring/address"));

  uint8_t bmpTries = 0;
  while (climateSensor.begin() && bmpTries < 5) { bmpTries++; delay(500); }
  bmpOK = (bmpTries < 5);
  if (bmpOK) {
    Serial.print(F("BMP280: found, chip ID 0x"));
    Serial.println(climateSensor.getChipID(), HEX);   // 0x58 = BMP280, 0x60 = BME280
  } else {
    Serial.println(F("BMP280: MISSING at 0x76 - check wiring/address"));
  }
  Serial.println();
}

void loop() {
  Serial.print(F("BMP280 temp: "));
  if (bmpOK) {
    climateSensor.takeForcedMeasurement();
    delay(50);                                        // conversion time at x1 oversampling
    int32_t t = climateSensor.getTemperatureCelsius();// degC x100, e.g. 2237 = 22.37C
    Serial.print(t / 100); Serial.print(F("."));
    if (abs(t % 100) < 10) Serial.print(F("0"));
    Serial.print(abs(t % 100)); Serial.print(F(" C"));
    if (t < -1000 || t > 5000) Serial.print(F("  << IMPLAUSIBLE"));
  } else {
    Serial.print(F("MISSING"));
  }

  Serial.print(F("   |   BH1750 light: "));
  if (bhOK) {
    bh1750.start(BH1750_QUALITY_LOW, BH1750_MTREG_LOW);  // same mode as main sketch
    delay(50);                                           // low quality converts in ~16ms
    uint16_t raw = bh1750.getRaw();
    float lux = bh1750.calcLux(raw, BH1750_QUALITY_LOW, BH1750_MTREG_LOW);
    Serial.print(lux, 1); Serial.print(F(" lux"));
    if (raw == 65535) Serial.print(F("  << SATURATED (too bright / stuck?)"));
  } else {
    Serial.print(F("MISSING"));
  }

  Serial.println();
  delay(2000);
}
