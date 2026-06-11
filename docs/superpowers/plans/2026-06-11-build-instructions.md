# Batch Build Instructions & Verification Suite Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add INSTRUCTIONS.md (staged batch build guide), CHECKLIST.md (per-unit QC sheet), and seven standalone Arduino verification sketches to the 2-Part ProMini logger repo, compile-verified with a portable arduino-cli.

**Architecture:** Documentation references numbered test sketches; each build stage ends in a VERIFY box naming the sketch to run and its expected serial output. Sketches are dependency-light (raw `Wire` register access where possible) and print explicit PASS/FAIL lines with timeouts. Two 8-byte `#define` profiles (BURN-IN, DEPLOY) keep the main sketch's powers-of-2 storage rule satisfied across the two build phases.

**Tech Stack:** Arduino C++ (ATmega328p, 3.3 V / 8 MHz ProMini), arduino-cli (portable, `arduino:avr` core), libraries: LowPower (LowPowerLab), hp_BH1750, forcedBMX280. Serial at 500000 baud (= 8 MHz/16, zero UART clock error).

**Spec:** `docs/superpowers/specs/2026-06-11-build-instructions-design.md`

---

## Reference facts (single source of truth for all tasks)

**Hardware (from the Cave Pearl 2022 tutorial + sketch comments):**
- ProMini removals: voltage regulator (clip from 2-leg side), power-LED limit resistor, reset switch. Bootloader MUST be confirmed (blink upload) before any cutting.
- RTC module (DS3231 + AT24C32) removals: 200 Ω charging resistor, power-LED resistor, main VCC leg (2nd from corner — forces run from Vbat), 32 kHz header pin. Bridge VCC to the Vbat side at the diode's black-ring end.
- Module join: A4/A5 I2C cross over to RTC SDA/SCL, VCC + GND rail extensions, RTC SQW → ProMini D2, foam tape between boards, jumpers through the RTC pass-through port.
- RGB LED (common cathode): red leg CUT, A0 = GND leg (OUTPUT LOW), A1 = green, A2 = blue, lit via `INPUT_PULLUP` (keeps current < 50 µA). Onboard D13 red LED + its 1k resistor stay.
- NTC/LDR ICU circuit: 10 kΩ 1% metfilm on D6 (reference), 10 k NTC (3950) on D7, 300 Ω on D8 (ICP1 timing pin), LDR 5528 on D9, one 0.1 µF (104) ceramic cap to GND.
- Sleep current targets: 1–2 µA logger-only; >100 µA ⇒ clone chip or regulator still present.
- VREF calibration: `InternalReferenceConstant` default 1126400; ±400 ⇒ ∓~1 mV reported; calibrate to within ±20 mV of DVM reading.
- UART: 3.3 V FTDI/CP2102, 6-pin; logger only starts after serial handshake; start menu has 8-min timeout; menu [10] sets VREF, [2] sets RTC, [3] interval, [1] download, [99] toggles setup menu, [13] shutdown.

**I2C addresses:** DS3231 = 0x68, RTC-module 4k EEPROM = 0x57, BMP280 = 0x76 (alt 0x77), BH1750 = 0x23 (alt 0x5C), 32k/64k EEPROM modules = 0x50.

**DS3231 registers:** time 0x00–0x06, control 0x0E, status 0x0F (bit7 = OSF power-loss flag, bit0/1 = alarm flags), aging 0x10, temp 0x11/0x12. Alarm1 regs 0x07–0x0A.

**Profiles (both = 8 bytes/record, 512 records in 4k EEPROM, ~5.3 d @ 15 min):**
- BURN-IN: `logLowestBattery_1byte`, `logRTC_Temperature_1byte`, `logCurrentBattery_2byte`, `readD7resistorwD6ref_2byte`, `readD9resistorwD6ref_2byte`, `LED_GndGB_A0_A2`
- DEPLOY: `logLowestBattery_1byte`, `logRTC_Temperature_1byte`, `readD7resistorwD6ref_2byte`, `readBh1750_LUX_2byte`, `recordBMEtemp_2byteInt`, `LED_GndGB_A0_A2`

**Compile target FQBN:** `arduino:avr:pro:cpu=8MHzatmega328`

---

### Task 1: Portable arduino-cli toolchain

**Files:**
- Create: `.tools/` (gitignored), `.gitignore` (add `.tools/` line)

- [ ] **Step 1:** Download portable arduino-cli into `.tools\` (PowerShell):

```powershell
New-Item -ItemType Directory -Force .tools | Out-Null
Invoke-WebRequest "https://downloads.arduino.cc/arduino-cli/arduino-cli_latest_Windows_64bit.zip" -OutFile .tools\cli.zip
Expand-Archive .tools\cli.zip .tools -Force
.tools\arduino-cli.exe version
```
Expected: version string prints.

- [ ] **Step 2:** Configure an isolated data dir and install core + libraries:

```powershell
$cli = ".tools\arduino-cli.exe"
& $cli --config-file .tools\cli.yaml config init --dest-file .tools\cli.yaml --overwrite
# point directories.data/downloads/user into .tools via config set
& $cli --config-file .tools\cli.yaml config set directories.data .tools\data
& $cli --config-file .tools\cli.yaml config set directories.downloads .tools\dl
& $cli --config-file .tools\cli.yaml config set directories.user .tools\user
& $cli --config-file .tools\cli.yaml core update-index
& $cli --config-file .tools\cli.yaml core install arduino:avr
& $cli --config-file .tools\cli.yaml lib install "Low-Power" "hp_BH1750" "ForcedBMX280"
```
Expected: core + 3 libraries installed. (Library index names to verify with `lib search` if install fails: LowPowerLab's is published as "Low-Power".)

- [ ] **Step 3:** Add `.tools/` to `.gitignore`, commit `.gitignore` only.

### Task 2: tests/01_UploadBlink

**Files:** Create `tests/01_UploadBlink/01_UploadBlink.ino`

- [ ] **Step 1:** Write sketch: 500000 baud; prints file/date banner + incrementing heartbeat line each second; blinks D13 red, then green (A0 OUTPUT LOW + A1 INPUT_PULLUP), then blue (A2 INPUT_PULLUP) in rotation. No libraries.
- [ ] **Step 2:** Compile: `& $cli --config-file .tools\cli.yaml compile --fqbn arduino:avr:pro:cpu=8MHzatmega328 tests/01_UploadBlink` → "Sketch uses … bytes" success.
- [ ] **Step 3:** Commit.

### Task 3: tests/02_I2C_Scanner

**Files:** Create `tests/02_I2C_Scanner/02_I2C_Scanner.ino`

- [ ] **Step 1:** Write sketch: scans 0x08–0x77 every 3 s, prints found addresses with friendly names from the address table above, then a summary line `CORE CHECK: PASS/FAIL` requiring 0x68 AND 0x57, plus `SENSOR CHECK:` lines for 0x76/0x23 marked optional. Wire only.
- [ ] **Step 2:** Compile (same command, path `tests/02_I2C_Scanner`). Expected: success.
- [ ] **Step 3:** Commit.

### Task 4: tests/03_RTC_Test

**Files:** Create `tests/03_RTC_Test/03_RTC_Test.ino`

- [ ] **Step 1:** Write sketch using raw Wire register access (no RTC library):
  1. Read status 0x0F → report OSF bit ("power loss since last set — set the clock!").
  2. Read temp regs 0x11/0x12 → print °C; flag implausible (<-10 or >50 indoors).
  3. Read time regs 3× at 1 s intervals → verify seconds advance (TICK PASS/FAIL).
  4. Serial command `T` sets clock from `__DATE__`/`__TIME__` compile constants and clears OSF.
  5. Alarm test: program Alarm1 for now+10 s, enable INTCN+A1IE in control 0x0E, attach FALLING interrupt on D2, wait ≤15 s → `ALARM PASS` or `ALARM FAIL (timeout)`.
  Final summary block prints all PASS/FAIL results.
- [ ] **Step 2:** Compile. Expected: success.
- [ ] **Step 3:** Commit.

### Task 5: tests/04_EEprom_Test

**Files:** Create `tests/04_EEprom_Test/04_EEprom_Test.ino`

- [ ] **Step 1:** Write sketch: `#define EEPROM_ADDR 0x57`, `#define EEPROM_BYTES 4096`, `#define PAGE_SIZE 32`. Warn `** DESTRUCTIVE: overwrites logged data **`, require serial `Y` to start. Pass 1: write address-derived pattern `(addr*7+13)&0xFF` page-by-page (poll ACK for write completion). Pass 2: read back in 16-byte chunks, count mismatches. Pass 3: erase to 0xFF. Print bytes/s, mismatch count, `EEPROM TEST: PASS/FAIL`.
- [ ] **Step 2:** Compile. Expected: success.
- [ ] **Step 3:** Commit.

### Task 6: tests/05_VccCal

**Files:** Create `tests/05_VccCal/05_VccCal.ino`

- [ ] **Step 1:** Write sketch: reads rail via 1.1 V bandgap (`ADMUX = 0b01001110` on 328p, discard first conversion, average 8), prints `railmV = constant / ADC` once per second using `long constant = 1126400`. User types measured DVM millivolts (e.g. `3302`) → sketch computes `newConstant = (long)((float)constant * measured / reported)`, prints it and switches to using it live for confirmation. Banner explains: enter this number in main sketch menu option [10].
- [ ] **Step 2:** Compile. Expected: success.
- [ ] **Step 3:** Commit.

### Task 7: tests/06_SleepCurrent

**Files:** Create `tests/06_SleepCurrent/06_SleepCurrent.ino`

- [ ] **Step 1:** Write sketch (needs LowPower lib): prints instructions (disconnect UART, insert meter in µA range in series with coin cell), 10 s countdown with D13 flashes, then: all pins INPUT (D13 LOW first), ADC off (`ADCSRA=0`), `power_all_disable()`, `LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF)`. Banner states targets: ≤2 µA PASS, 3–10 µA investigate, >100 µA clone/regulator fail.
- [ ] **Step 2:** Compile. Expected: success.
- [ ] **Step 3:** Commit.

### Task 8: tests/07_SensorTest

**Files:** Create `tests/07_SensorTest/07_SensorTest.ino`

- [ ] **Step 1:** Write sketch using hp_BH1750 + forcedBMX280: detect each sensor (report MISSING if begin fails), then every 2 s print BMP280 temp (flag outside −10…+50 °C) and BH1750 lux (flag 0 or saturated 65535 as "check sensor/cover"). Running PASS criteria: both sensors respond and values move when you breathe on / cover them (manual check, stated in banner).
- [ ] **Step 2:** Compile. Expected: success.
- [ ] **Step 3:** Commit.

### Task 9: INSTRUCTIONS.md

**Files:** Create `INSTRUCTIONS.md`

- [ ] **Step 1:** Write the staged guide per the spec's Stage 0–7 outline using the Reference facts above. Required elements: batch-workflow intro (one stage across all units, label units first); BOM table (component, per-unit qty, ×N column to fill in); library install list; both profile `#define` blocks verbatim; a VERIFY box at the end of every stage (visual/multimeter checks, test sketch to run, expected serial output); `[OPTIONAL]` tags on RGB LED note (D13 fallback), rail buffer cap, conformal coating, falcon-tube housing appendix (point to the two STL files in repo root); powers-of-2 rule explanation with the byte table; burn-in procedure (VREF cal via menu [10], RTC set via [2], interval via [3], start via [6], 24–72 h, download via [1], what a healthy battery curve looks like); troubleshooting table (garbled serial → 500000 baud; missing 0x68/0x57 → joints/crossover; high sleep current → clone or regulator; `not PowerOfTwo` error → profile defines; OSF/`*2000*` date → re-set clock; random shutdowns → coin-cell spring contact, hot-glue fix).
- [ ] **Step 2:** Cross-check every VERIFY box references an existing sketch name and every menu number matches the sketch source (menu list at `2-Part_ProMiniFalconTubeLogger.ino:1820-1831`).
- [ ] **Step 3:** Commit.

### Task 10: CHECKLIST.md

**Files:** Create `CHECKLIST.md`

- [ ] **Step 1:** Write printable per-unit sheet: header fields (Unit ID, builder, batch, dates), one row/checkbox per stage sign-off, blank fields for recorded values: bootloader OK, sleep current ___ µA, VREF constant ______, RTC OSF cleared, EEPROM result ___/4096, burn-in start/end + records downloaded, sensor addresses seen, profile flashed (BURN-IN/DEPLOY), final notes. Keep to one printed page; instructions at top to print one per unit.
- [ ] **Step 2:** Commit.

### Task 11: Main-sketch profile compile verification

**Files:** None permanent (temporary edits to a copy)

- [ ] **Step 1:** Copy `2-Part_ProMiniFalconTubeLogger/` to `.tools\profilecheck\2-Part_ProMiniFalconTubeLogger\`; edit defines to BURN-IN profile; compile with same FQBN. Expected: success.
- [ ] **Step 2:** Edit copy to DEPLOY profile; compile. Expected: success (requires hp_BH1750 + forcedBMX280 installed in Task 1).
- [ ] **Step 3:** Delete the copy. Report both results in final summary. Do NOT modify the user's committed sketch defines — INSTRUCTIONS.md documents the profiles instead (user builds in batches and switches profiles per phase).

### Task 12: README pointer + final review

**Files:** Modify `README.md` (top section)

- [ ] **Step 1:** Add two lines after the tutorial links: link to `INSTRUCTIONS.md` (build + verification) and `tests/` (verification sketches).
- [ ] **Step 2:** Run requesting-code-review checklist against spec; fix anything found.
- [ ] **Step 3:** Commit.

## Self-review notes

- Spec coverage: Stages 0–7 → Task 9; test sketches → Tasks 2–8; checklist → Task 10; compile verification → Tasks 1, 11; both profiles documented not committed (user switches per phase) → Task 11 Step 3 rationale.
- Type consistency: sketch folder names match .ino names (Arduino IDE requirement); all compile commands use the same FQBN string.
