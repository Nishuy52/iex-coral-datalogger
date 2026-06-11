# Design: Batch Build & Verification Documentation for the 2-Part ProMini EEPROM Logger

**Date:** 2026-06-11
**Status:** Approved by user (build batches of the Cave Pearl 2-part coin-cell logger)

## Goal

Produce repeatable build documentation and verification software for batch-building the
2-Part ProMini EEPROM Data Logger (Cave Pearl Project 2022 design), so that each unit in a
batch is assembled, tested, calibrated, and burn-in verified before deployment sensors are
attached.

## User's build configuration

- **Core build (every unit):** ProMini 3.3 V/8 MHz + DS3231 RTC module, CR2032 powered,
  with the tutorial's power modifications (regulator, power LEDs, reset switch, charging
  circuit removed).
- **Included add-ons:** RGB indicator LED on A0–A2; NTC + LDR resistance circuit on D6–D9.
- **Deployment sensors:** BMP280 (temperature only) and BH1750 (lux), attached *after*
  burn-in testing passes.
- **Workflow:** NTC/LDR are used during burn-in of the bare logger; BMP280 + BH1750 added
  afterwards for deployment.
- **Out of scope:** falcon-tube housing (brief optional appendix only), 32k EEPROM upgrade,
  OLED display.

## Sensor-byte profiles (powers-of-2 rule)

The sketch rejects any record size that is not 1, 2, 4, 8, or 16 bytes
(`sensorBytesPerRecord & (sensorBytesPerRecord-1)` check at startup → shutdown).
The repo's committed defines total 6 bytes and will not run. Two named 8-byte profiles fix
this:

| Profile | Defines | Bytes |
|---|---|---|
| **BURN-IN** | `logLowestBattery_1byte` + `logRTC_Temperature_1byte` + `logCurrentBattery_2byte` + `readD7resistorwD6ref_2byte` (NTC) + `readD9resistorwD6ref_2byte` (LDR) | 1+1+2+2+2 = **8** |
| **DEPLOY** | `logLowestBattery_1byte` + `logRTC_Temperature_1byte` + `readD7resistorwD6ref_2byte` (NTC) + `readBh1750_LUX_2byte` + `recordBMEtemp_2byteInt` (BMP280 temp via forcedBMX280) | 1+1+2+2+2 = **8** |

Both yield 512 records in the RTC module's 4k EEPROM (~5.3 days @ 15 min). The LDR define
is disabled at deployment (the part stays soldered; BH1750 covers light). BMP280 pressure
is never enabled. The forcedBMX280 library (maintained) is used for the BMP280 rather than
BMP280_DEV (repo vanished).

## Deliverables

1. **`INSTRUCTIONS.md`** — stage-based build guide written for assembly-line batch work
   (complete each stage across all units before the next stage):
   - Stage 0: BOM (per-unit qty + ×N column), tools, software setup (IDE, LowPower,
     hp_BH1750, forcedBMX280 libraries, UART adapter drivers).
   - Stage 1: ProMini prep — bootloader check FIRST, then remove regulator, power-LED
     resistor, reset switch; solder UART header.
   - Stage 2: RTC module prep — remove charging resistor, power-LED resistor, VCC leg,
     32 kHz pin; bridge VCC to Vbat side.
   - Stage 3: Join modules — I2C A4/A5 crossover, VCC/GND rails, SQW→D2 alarm line, foam
     tape stack.
   - Stage 4: RGB LED on A0(GND)/A1(G)/A2(B), red leg cut; D13 red retained.
   - Stage 5: NTC/LDR circuit — 10k ref on D6, 10k NTC on D7, 300 Ω on D8, LDR on D9,
     104 cap to GND.
   - Stage 6: Calibration + burn-in — VREF calibration, RTC time set, BURN-IN profile
     flash, 24–72 h coin-cell run, data download and inspection.
   - Stage 7: Deployment sensors — BMP280 + BH1750 wiring, DEPLOY profile flash, final QC.
   - Every stage ends with a **VERIFY** box: visual/multimeter checks + which test sketch
     to run + expected output. Optional steps tagged `[OPTIONAL]`.
   - Troubleshooting table (garbled serial, missing I2C devices, high sleep current,
     powers-of-2 error, OSF flag, clone-chip sleep current).

2. **`tests/` folder** — standalone Arduino verification sketches, all at 500000 baud
   (zero UART error at 8 MHz), numbered to match stages:
   - `01_UploadBlink` — bootloader/toolchain check; blinks D13 + RGB via pullup trick.
   - `02_I2C_Scanner` — scans bus; names known addresses (0x68 RTC, 0x57 4k EEPROM,
     0x76/0x77 BMP280, 0x23/0x5C BH1750, 0x50 32k+ EEPROM).
   - `03_RTC_Test` — register-level DS3231 check: OSF power-loss flag, temperature,
     clock ticking, 10 s alarm fired on D2 interrupt; PASS/FAIL summary.
   - `04_EEprom_Test` — page-aligned pattern write/read/verify across all 4096 bytes;
     PASS/FAIL with error count. (Destructive to logged data — run before deployment.)
   - `05_VccCal` — interactive InternalReferenceConstant calibration against a DVM;
     computes the corrected constant to enter in the main sketch's menu option [10].
   - `06_SleepCurrent` — powers everything down and sleeps forever so sleep current can
     be measured (target ≤ 2 µA logger-only; >100 µA indicates clone chip/regulator left on).
   - `07_SensorTest` — BMP280 (forcedBMX280) + BH1750 (hp_BH1750) sanity readings with
     plausible-range flags.

3. **`CHECKLIST.md`** — printable per-unit QC sheet: unit ID, builder, dates, stage
   sign-off checkboxes, and recorded values (sleep current µA, VREF constant, RTC OSF
   state, EEPROM test result, burn-in window, sensor addresses found, profile flashed).

## Verification of the deliverables themselves

If `arduino-cli` is available locally, compile-check all test sketches and the main sketch
under both profiles. Otherwise, document compile expectations and rely on first-batch use.

## Error handling philosophy

Test sketches print explicit PASS/FAIL lines and never hang silently; every wait has a
timeout with a failure message. Instructions give expected numeric ranges so failures are
recognizable at the bench.
