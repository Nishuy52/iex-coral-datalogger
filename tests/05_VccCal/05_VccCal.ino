// 05_VccCal - Stage 6 calibration helper for the 2-Part ProMini EEprom Logger
// -----------------------------------------------------------------------------
// PURPOSE: calibrates the InternalReferenceConstant that the main logger code
// uses to convert ADC readings of the 1.1V internal bandgap into rail
// millivolts. Every 328p's bandgap is slightly different (1.0-1.2V), so an
// uncalibrated logger can misreport the coin cell voltage by 100mV or more -
// enough to make the low-battery shutdown fire early or late.
//
// PROCEDURE (logger powered from the UART adapter, no coin cell needed):
//   1. upload, open serial monitor at 500000 baud
//   2. measure the actual voltage between VCC and GND with your multimeter
//   3. type the measured value in MILLIVOLTS (e.g. 3302) and press send
//   4. the sketch computes the corrected constant and switches to it live -
//      the reported mV should now match your meter within +/-20mV
//   5. WRITE THE CONSTANT on the unit's QC checklist, then enter it in the
//      main logger sketch via start menu option [10] after flashing
//
// BOARD SETTINGS:  Arduino Pro or Pro Mini / ATmega328P (3.3V, 8MHz)
// SERIAL MONITOR:  500000 baud  (line ending: Newline)
//
// Default constant = 1126400 (1100mV * 1024). Changing it by ~400 moves the
// reported value by ~1mV. Typical calibrated range: 1000000 - 1228800.
// No libraries required.

long referenceConstant = 1126400L;   // default: assumes bandgap is exactly 1100mV
uint16_t reportedmV = 0;

uint16_t readRailmV() {
  // ADMUX: REFS0=1 (AVcc reference), MUX3..0=1110 (1.1V bandgap input)
  ADMUX = bit(REFS0) | bit(MUX3) | bit(MUX2) | bit(MUX1);
  delay(3);                                    // let the bandgap settle
  ADCSRA |= bit(ADSC); while (ADCSRA & bit(ADSC));   // throw-away reading
  uint32_t sum = 0;
  for (uint8_t i = 0; i < 8; i++) {
    ADCSRA |= bit(ADSC); while (ADCSRA & bit(ADSC));
    sum += ADC;
  }
  return (uint16_t)(referenceConstant / (sum >> 3));
}

void setup() {
  Serial.begin(500000);
  Serial.println();
  Serial.println(F("====================================================="));
  Serial.println(F(" 05_VccCal - InternalReferenceConstant calibration"));
  Serial.println(F(" 1. measure VCC-to-GND with your multimeter"));
  Serial.println(F(" 2. type the value in mV (e.g. 3302) + send"));
  Serial.println(F(" 3. record the printed constant on the QC checklist"));
  Serial.println(F("    and enter it via main-sketch menu option [10]"));
  Serial.println(F("====================================================="));
}

void loop() {
  reportedmV = readRailmV();
  Serial.print(F("constant "));
  Serial.print(referenceConstant);
  Serial.print(F("  ->  rail = "));
  Serial.print(reportedmV);
  Serial.println(F(" mV"));

  if (Serial.available()) {
    long measured = Serial.parseInt();
    while (Serial.available()) Serial.read();   // discard line ending
    if (measured > 1700 && measured < 3700) {   // plausible rail voltage only
      referenceConstant = (long)((float)referenceConstant * measured / reportedmV);
      Serial.println();
      Serial.print(F(">>> your meter: ")); Serial.print(measured);
      Serial.print(F(" mV   new constant: "));
      Serial.println(referenceConstant);
      Serial.println(F(">>> RECORD THIS CONSTANT on the unit's checklist!"));
      Serial.println(F(">>> reported mV below should now match your meter:"));
      Serial.println();
    } else if (measured != 0) {
      Serial.println(F("?? expected millivolts between 1700 and 3700, e.g. 3302"));
    }
  }
  delay(1000);
}
