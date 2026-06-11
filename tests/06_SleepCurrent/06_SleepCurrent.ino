// 06_SleepCurrent - Stage 6 verification for the 2-Part ProMini EEprom Logger
// ------------------------------------------------------------------------------
// PURPOSE: puts the logger into its deepest possible sleep so you can measure
// the true sleep current with a multimeter. Sleep current is THE number that
// decides whether a CR2032 lasts a year or a week, and it also catches two
// common build problems:
//   - voltage regulator not fully removed  -> ~80uA of back-leakage
//   - clone/fake 328p chip                 -> won't sleep below ~100uA
//
// PROCEDURE:
//   1. upload this sketch over UART, open monitor at 500000 baud, read banner
//   2. close the monitor and UNPLUG THE UART ADAPTER (it back-feeds power)
//   3. put the multimeter in uA range IN SERIES with the coin cell:
//      either between cell and holder contact, or across a power jumper
//   4. the sketch flashes D13 ten times (one per second), then sleeps forever
//   5. wait ~30s after the flashing stops for the reading to settle
//
// TARGETS (logger core only - no BMP280/BH1750/PIR attached):
//   <= 2 uA     PASS - matches the tutorial's expected 1-2uA
//   3 - 10 uA   investigate: flux residue? meter burden? cap leakage?
//   > 100 uA    FAIL - regulator remnant or clone chip
// Record the value on the unit's QC checklist.
//
// BOARD SETTINGS:  Arduino Pro or Pro Mini / ATmega328P (3.3V, 8MHz)
// REQUIRES: LowPower library [by LowPowerLab] via Library Manager
// Press reset (or power-cycle) to run the countdown again.

#include <avr/power.h>
#include <LowPower.h>

void setup() {
  Serial.begin(500000);
  Serial.println();
  Serial.println(F("====================================================="));
  Serial.println(F(" 06_SleepCurrent - deep sleep for uA measurement"));
  Serial.println(F(" 1. UNPLUG this UART adapter now"));
  Serial.println(F(" 2. meter in uA range, in series with the coin cell"));
  Serial.println(F(" 3. D13 flashes 10x, then the unit sleeps forever"));
  Serial.println(F(" TARGET: <=2uA  |  >100uA = clone chip or regulator"));
  Serial.println(F("====================================================="));
  Serial.flush();

  // 10-second countdown so you have time to disconnect and wire the meter
  pinMode(13, OUTPUT);
  for (uint8_t i = 0; i < 10; i++) {
    digitalWrite(13, HIGH); delay(100);
    digitalWrite(13, LOW);  delay(900);
  }

  // ---- shut everything down ----
  // unused pins floating can oscillate and waste power, so enable pullups -
  // EXCEPT the LED pins (a pullup would light the LED through A0's ground)
  // and D13 (its LED+resistor chain to ground would draw through a pullup).
  for (uint8_t pin = 0; pin <= A5; pin++) {
    if (pin == 13 || pin == A0 || pin == A1 || pin == A2) {
      pinMode(pin, INPUT);          // LED paths: leave un-pulled
    } else {
      pinMode(pin, INPUT_PULLUP);   // everything else: pin held high, no current
    }
  }

  ADCSRA = 0;                       // ADC off
  bitSet(ACSR, ACD);                // analog comparator off
  power_all_disable();              // clock-gate every remaining peripheral
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
}

void loop() { }   // never reached
