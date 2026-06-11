// 03_RTC_Test - Stage 3 verification for the 2-Part ProMini EEprom Logger
// -------------------------------------------------------------------------
// PURPOSE: register-level check of the DS3231 RTC after the two modules are
// joined. Tests four things the logger depends on:
//   1. power-loss (OSF) flag state    - has the clock lost Vbat power?
//   2. internal temperature register  - sanity check the chip is alive
//   3. clock ticking                  - seconds must advance
//   4. ALARM fired on ProMini pin D2  - the SQW->D2 jumper that wakes the
//                                       logger; without it the build is dead
//
// BOARD SETTINGS:  Arduino Pro or Pro Mini / ATmega328P (3.3V, 8MHz)
// SERIAL MONITOR:  500000 baud   (set line ending to 'No line ending')
//
// SERIAL COMMANDS:
//   T  set RTC from this sketch's compile time & clear the power-loss flag
//   A  re-run the 10-second alarm test
//   R  re-run the full test sequence
//
// EXPECTED: all four lines of the summary print PASS. A fresh RTC (or one
// whose coin cell was out) shows OSF FAIL until you send 'T' - that is
// normal: set the clock, then expect PASS on re-test.
// No libraries required beyond Wire.

#include <Wire.h>

#define DS3231_ADDRESS     0x68
#define DS3231_CONTROL_REG 0x0E
#define DS3231_STATUS_REG  0x0F
#define DS3231_TMP_UP_REG  0x11
#define RTC_ALARM_PIN      2

volatile bool alarmFlag = false;
bool oscOK = false, tempOK = false, tickOK = false, alarmOK = false;

// ---------- low-level register helpers ----------
uint8_t readRegister(uint8_t reg) {
  Wire.beginTransmission(DS3231_ADDRESS);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(DS3231_ADDRESS, 1);
  return Wire.read();
}

void writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(DS3231_ADDRESS);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

uint8_t bcd2bin(uint8_t v) { return (v >> 4) * 10 + (v & 0x0F); }
uint8_t bin2bcd(uint8_t v) { return ((v / 10) << 4) | (v % 10); }

void rtc_d2_alarm_ISR() { alarmFlag = true; }

// ---------- individual tests ----------
void testOSF() {
  uint8_t status = readRegister(DS3231_STATUS_REG);
  Serial.print(F("[1] Power-loss flag (OSF): "));
  if (status & 0b10000000) {
    Serial.println(F("SET -> clock stopped at some point. Send 'T' to set time & clear."));
    oscOK = false;
  } else {
    Serial.println(F("clear -> PASS"));
    oscOK = true;
  }
}

void testTemperature() {
  int8_t  tMSB = (int8_t)readRegister(DS3231_TMP_UP_REG);
  uint8_t tLSB = readRegister(DS3231_TMP_UP_REG + 1);
  float degC = tMSB + ((tLSB >> 6) * 0.25);
  Serial.print(F("[2] RTC temperature: "));
  Serial.print(degC, 2); Serial.print(F(" C  "));
  tempOK = (degC > -10.0 && degC < 50.0);   // plausible on a workbench
  Serial.println(tempOK ? F("-> PASS") : F("-> FAIL (implausible reading)"));
}

void printTime() {
  uint8_t sec  = bcd2bin(readRegister(0x00) & 0x7F);
  uint8_t min  = bcd2bin(readRegister(0x01));
  uint8_t hour = bcd2bin(readRegister(0x02) & 0x3F);
  uint8_t day  = bcd2bin(readRegister(0x04));
  uint8_t mon  = bcd2bin(readRegister(0x05) & 0x1F);
  uint16_t yr  = 2000 + bcd2bin(readRegister(0x06));
  Serial.print(yr);  Serial.print(F("/"));
  Serial.print(mon); Serial.print(F("/"));
  Serial.print(day); Serial.print(F(" "));
  Serial.print(hour); Serial.print(F(":"));
  Serial.print(min);  Serial.print(F(":"));
  Serial.println(sec);
}

void testTicking() {
  Serial.println(F("[3] Clock tick test (3 readings, 1s apart):"));
  uint8_t first = bcd2bin(readRegister(0x00) & 0x7F);
  uint8_t last = first;
  for (uint8_t i = 0; i < 3; i++) {
    Serial.print(F("    ")); printTime();
    delay(1100);
    last = bcd2bin(readRegister(0x00) & 0x7F);
  }
  tickOK = (last != first);
  Serial.println(tickOK ? F("    seconds advanced -> PASS")
                        : F("    seconds FROZEN -> FAIL (oscillator not running?)"));
}

void testAlarm() {
  Serial.println(F("[4] Alarm test: alarm set for 10 seconds from now..."));
  // Alarm1 'seconds match' mode: A1M1=0, A1M2..A1M4=1
  uint8_t nowSec    = bcd2bin(readRegister(0x00) & 0x7F);
  uint8_t targetSec = (nowSec + 10) % 60;
  writeRegister(0x07, bin2bcd(targetSec));   // A1M1=0: match seconds
  writeRegister(0x08, 0b10000000);           // A1M2=1: ignore minutes
  writeRegister(0x09, 0b10000000);           // A1M3=1: ignore hours
  writeRegister(0x0A, 0b10000000);           // A1M4=1: ignore day/date

  uint8_t status = readRegister(DS3231_STATUS_REG);
  writeRegister(DS3231_STATUS_REG, status & 0b11111100);     // clear A1F & A2F
  writeRegister(DS3231_CONTROL_REG, 0b00000101);             // INTCN=1, A1IE=1

  alarmFlag = false;
  pinMode(RTC_ALARM_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RTC_ALARM_PIN), rtc_d2_alarm_ISR, FALLING);

  uint32_t start = millis();
  while (!alarmFlag && (millis() - start) < 15000UL) { delay(50); }
  detachInterrupt(digitalPinToInterrupt(RTC_ALARM_PIN));

  alarmOK = alarmFlag;
  if (alarmOK) {
    Serial.print(F("    D2 interrupt received after "));
    Serial.print((millis() - start) / 1000.0, 1);
    Serial.println(F("s -> PASS"));
  } else {
    Serial.println(F("    TIMEOUT, no D2 interrupt -> FAIL (check SQW->D2 jumper)"));
  }
  writeRegister(DS3231_CONTROL_REG, 0b00000100);             // alarm interrupts off
  status = readRegister(DS3231_STATUS_REG);
  writeRegister(DS3231_STATUS_REG, status & 0b11111100);     // clear alarm flags
}

void setTimeFromCompiler() {
  // __DATE__ = "Jun 11 2026", __TIME__ = "12:34:56"
  const char monthNames[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
  char mStr[4] = { __DATE__[0], __DATE__[1], __DATE__[2], 0 };
  uint8_t month = (strstr(monthNames, mStr) - monthNames) / 3 + 1;
  uint8_t day   = atoi(__DATE__ + 4);
  uint8_t year  = atoi(__DATE__ + 9);   // last two digits
  uint8_t hour  = atoi(__TIME__);
  uint8_t min   = atoi(__TIME__ + 3);
  uint8_t sec   = atoi(__TIME__ + 6);

  writeRegister(0x00, bin2bcd(sec));
  writeRegister(0x01, bin2bcd(min));
  writeRegister(0x02, bin2bcd(hour));   // 24h mode
  writeRegister(0x04, bin2bcd(day));
  writeRegister(0x05, bin2bcd(month));
  writeRegister(0x06, bin2bcd(year));

  uint8_t status = readRegister(DS3231_STATUS_REG);
  writeRegister(DS3231_STATUS_REG, status & 0b01111111);  // clear OSF
  Serial.print(F("RTC set to compile time: ")); printTime();
  Serial.println(F("NOTE: compile time lags real time by upload duration."));
  Serial.println(F("      Use the main logger's menu [2] for an accurate set."));
}

void runAllTests() {
  Serial.println(F("\n--- running test sequence ---"));
  testOSF();
  testTemperature();
  testTicking();
  testAlarm();
  Serial.println(F("\n========= SUMMARY ========="));
  Serial.print(F(" OSF/power : ")); Serial.println(oscOK  ? F("PASS") : F("FAIL"));
  Serial.print(F(" Temp      : ")); Serial.println(tempOK ? F("PASS") : F("FAIL"));
  Serial.print(F(" Ticking   : ")); Serial.println(tickOK ? F("PASS") : F("FAIL"));
  Serial.print(F(" D2 Alarm  : ")); Serial.println(alarmOK? F("PASS") : F("FAIL"));
  Serial.println(F("==========================="));
  Serial.println(F("Commands: T=set time  A=alarm test  R=re-run all"));
}

void setup() {
  Serial.begin(500000);
  Wire.begin();
  Serial.println();
  Serial.println(F("====================================================="));
  Serial.println(F(" 03_RTC_Test - DS3231 register-level check"));
  Serial.print  (F(" Compiled: ")); Serial.print(F(__DATE__));
  Serial.print  (F(" ")); Serial.println(F(__TIME__));
  Serial.println(F("====================================================="));

  // quick presence check before driving registers
  Wire.beginTransmission(DS3231_ADDRESS);
  if (Wire.endTransmission() != 0) {
    Serial.println(F("FATAL: no device at 0x68! Run 02_I2C_Scanner first."));
    while (true) { delay(1000); }
  }
  runAllTests();
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'T' || c == 't') { setTimeFromCompiler(); runAllTests(); }
    if (c == 'A' || c == 'a') { testAlarm(); }
    if (c == 'R' || c == 'r') { runAllTests(); }
  }
}
