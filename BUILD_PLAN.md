# Prototype Build Plan — deadline 27 June 2026

**Goal:** at least one fully working logger (DEPLOY profile, BMP280 + BH1750 attached,
clean multi-day data download) demonstrated by **Sat 27 Jun**.

**Hard constraints:**
- **Mon 22 Jun, end of day = last soldering.** After this date: UART work only
  (flashing, menus, test sketches, downloads, burn-in runs).
- Stages and test sketches referenced below are from [INSTRUCTIONS.md](INSTRUCTIONS.md).

## Strategy (read first)

1. **Build 3 units depth-first, not the whole batch breadth-first.** The batch
   assembly-line approach in INSTRUCTIONS.md is for when time is open-ended. Under a
   deadline you want 3 candidate units racing through all stages, so one bad module or
   clone chip (expect ~10–20% failure on cheap boards) doesn't sink the date. The rest of
   the batch waits until July.
2. **Run an early shakedown burn-in *before* the soldering deadline.** A burn-in that
   fails on Jun 24 is a dead prototype; the same failure on Jun 17 is a 20-minute re-flow.
   The plan front-loads a 48 h BURN-IN-profile run mid-window.
3. **Attach the deployment sensors before Jun 22**, even though INSTRUCTIONS.md sequences
   them after burn-in. Sensor soldering after the deadline is not an option, so Stage 5
   wiring moves inside the build window and the *final* burn-in runs with the DEPLOY
   profile — which is also exactly what you want to demonstrate on the 27th.
4. **The Jun 23–26 window fits the EEPROM perfectly:** 512 records × 15 min ≈ 5.3 days.
   Start the final run on the evening of the 22nd and it logs right up to download day.

## Calendar

| Date | Work | Stage refs |
|------|------|-----------|
| **Thu 11 Jun** | Inventory ALL parts against the BOM **today** — anything missing gets ordered now, this is the only float in the plan. Install IDE + 3 libraries; compile every `tests/` sketch and the main sketch in both profiles (they arrive compile-unverified). Label units U01–U03, print 3 checklists. | Stage 0 |
| **Fri 12 Jun** | Bootloader-check ≥4 ProMinis with `01_UploadBlink`, keep the best 3. Stage 1 mods (regulator, LED resistor, UART header) on U01–U03. Re-verify blink after mods. | Stage 1 |
| **Sat 13 Jun** | Stage 2 RTC module mods ×3. Stage 3: join modules, solder I2C crossover + rails + SQW→D2 ×3. | Stages 2–3 |
| **Sun 14 Jun** | Stage 3 verification ×3: `02_I2C_Scanner` (CORE PASS), `03_RTC_Test` (4× PASS), `04_EEprom_Test` (0 mismatches), coin-cell-only clock retention. Re-flow anything marginal NOW. RGB LED + NTC/LDR circuit ×3 (required Stage 1 items — INSTRUCTIONS.md solders these during Pro Mini prep; this schedule defers them to here, after the join verifies). | Stages 1, 3 |
| **Mon 15 Jun** | **GATE A: ≥2 units fully pass electrical QC.** If not, diagnose tonight — there are 7 build days left. Stage 4 calibration ×3: `05_VccCal` constant, `06_SleepCurrent` (≤2 µA). Flash BURN-IN profile, set clock + constant, **1-minute interval**, START. Overnight shakedown (8.5 h fills the EEPROM). | Stage 4 |
| **Tue 16 Jun** | Download shakedown data ×3. Check: no gaps, battery flat, NTC/LDR responding. Then re-flash, **15-min interval**, start the real 48–72 h burn-in on coin cell. | Stage 4 |
| **Wed 17 Jun** | Burn-in running — hands off. Prep sensor modules: bench-test every BMP280 & BH1750 on a spare ProMini (`02` + `07`) so only known-good sensors get soldered. | Stage 5 prep |
| **Thu 18 Jun** | Burn-in running. Buffer day for any rework queued from earlier checks. | — |
| **Fri 19 Jun** | **GATE B: download burn-in from all 3; ≥1 unit (target 2) must be clean.** Stage 5 on the two best units: solder BMP280 + BH1750 to the I2C bus, `02_I2C_Scanner` sees 0x76 + 0x23, `07_SensorTest` wiggle test PASS. | Stage 5 |
| **Sat 20 Jun** | Flash DEPLOY profile on both; startup must list 8 bytes/rec + 5 channels. Short live test (1-min interval, ~1 h), download, confirm all channels sane. | Stage 5 |
| **Sun 21 Jun** | Rework buffer: fix anything Sat exposed. Hot-glue coin cells. Final visual QC under magnification, flux cleanup. | — |
| **Mon 22 Jun** | **GATE C — LAST SOLDER DAY.** Any joint you're unsure about gets re-flowed this morning. Evening: fresh CR2032s, flash/menu sequence (clock `[2]`, constant `[10]`, **15-min interval** `[3]`, START `[6]`) on both units. Final burn-in begins tonight. | Stage 4 seq. |
| **Tue 23 – Thu 25 Jun** | Burn-in only (allowed). Leave units alone. Optional midpoint check Wed on the *backup* unit only — connecting UART restarts the run, so never touch the primary. | — |
| **Fri 26 Jun** | Download the backup unit first (PuTTY/CoolTerm), verify ~4 days of clean 5-channel data. If good, prototype is proven a day early. | — |
| **Sat 27 Jun** | **DEADLINE.** Download the primary unit → working prototype with a multi-day dataset. Sign off checklists. | — |

## Go/no-go gates

| Gate | Date | Bar | If missed |
|------|------|-----|-----------|
| A | Mon 15 | ≥2 of 3 units pass core electrical QC | Pull spare modules, rebuild worst unit; drop to 2 candidates |
| B | Fri 19 | ≥1 clean 72 h burn-in | Use the cleanest unit anyway; shorten final burn-in bar to "48 h clean" |
| C | Mon 22 EOD | ≥1 unit sensor-fitted, DEPLOY-flashed, `07` PASS | No recovery — this is why 3 units started |

## Contingencies (all UART-only, so allowed after Jun 22)

- **Primary unit dies during final burn-in** → backup unit's download becomes the
  deliverable; this is the whole reason two units enter the final run.
- **Data gaps in final run** → a *partial* clean dataset still demonstrates a working
  prototype; pair it with the clean Jun 16–19 burn-in download as supporting evidence.
- **Battery contact resets** (the classic field failure) → hot glue is applied Jun 21,
  but if it recurs, re-seat the cell and restart the run — even Jun 25→27 yields ~2 days
  of data, which is enough to demonstrate.
- **Both sensor units fail `07` after Jun 22** → fall back to a BURN-IN-profile prototype
  (NTC + LDR are real sensors; the logger still demonstrably logs). Lesser deliverable,
  but not zero.

## What is deliberately cut

- No falcon-tube housing, no 32k EEPROM, no RTC aging calibration — none are needed for a
  working prototype.
- Rail-buffer caps were originally cut too, but a post-deadline coin-cell test on the DEPLOY
  unit showed the exact symptom they exist to prevent: `LoBat[mv]`/`RTC[°C]` read healthy
  while `[°C]bmE` stuck at a fixed wrong value and `D7[Ω]` read implausibly low, only when
  running on the coin cell (fine over UART) — see the diagnostic note in
  [INSTRUCTIONS.md](INSTRUCTIONS.md#wiring-the-shared-i2c-bus-both-sensors--optional-eeprom).
  Add the 220–1000 µF tantalum rail cap to any unit showing this pattern.
- Remaining batch units: park them at whatever stage their parts are in; resume with the
  normal INSTRUCTIONS.md assembly-line flow after the 27th.
