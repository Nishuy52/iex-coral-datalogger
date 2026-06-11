# Batch Build & Verification Instructions — 2-Part ProMini EEprom Logger

Step-by-step build, test, and calibration guide for assembling these loggers **in batches**.
Based on the [Cave Pearl Project 2022 tutorial](https://thecavepearlproject.org/2022/03/09/powering-a-promini-logger-for-one-year-on-a-coin-cell/)
and its [build video](https://www.youtube.com/watch?v=58ps9fUyY0Q). This document adds a
staged verification workflow: every stage ends with a **VERIFY** box, most backed by a
numbered test sketch in the [`tests/`](tests/) folder.

> **Batch workflow:** label every unit first (U01, U02, …), then complete each stage across
> **all** units before starting the next stage. Print one [CHECKLIST.md](CHECKLIST.md) per
> unit and fill it in as you go — the recorded values (sleep current, calibration constant)
> are per-chip and cannot be reconstructed later. Expect ~10–20% of cheap modules to fail
> QC ("infant mortality"); buy spares and test everything before assembly.

Steps tagged **[OPTIONAL]** can be skipped without breaking the core logger.
Everything else is required for coin-cell operation.

<img src="images/2-PartEEpromLogger_CavePearlProject_2022.jpg" width="340" alt="finished 2-part logger"> <img src="images/20240713_2PartLogger_schematic_400x240pixw.png" width="400" alt="full wiring schematic">

*Left: the finished 2-module stack. Right: the full schematic — keep it open while soldering
(both files are local in [images/](images/)). Step photos throughout this guide are
hotlinked from the [Cave Pearl Project tutorial](https://thecavepearlproject.org/2022/03/09/powering-a-promini-logger-for-one-year-on-a-coin-cell/)
(© Edward Mallon) and need an internet connection to display.*

---

## Build configuration used in this guide

Two `#define` profiles for the main sketch keep the **powers-of-2 rule** satisfied
(explained in [Appendix A](#appendix-a--the-powers-of-2-rule)):

**BURN-IN profile** — bare logger soak test, no external sensors (8 bytes/record):

```cpp
#define logLowestBattery_1byte
#define logRTC_Temperature_1byte
#define logCurrentBattery_2byte          // padding to reach 8 bytes
#define readD7resistorwD6ref_2byte       // NTC thermistor
#define readD9resistorwD6ref_2byte       // LDR
#define LED_GndGB_A0_A2
```

**DEPLOY profile** — BMP280 (temperature only) + BH1750 attached (8 bytes/record):

```cpp
#define logLowestBattery_1byte
#define logRTC_Temperature_1byte
#define readD7resistorwD6ref_2byte       // NTC thermistor
#define readBh1750_LUX_2byte             // BH1750 light sensor
#define recordBMEtemp_2byteInt           // BMP280 temp via forcedBMX280 library
#define LED_GndGB_A0_A2
// readD9resistorwD6ref_2byte stays commented out: the LDR remains soldered
// but is not logged - the BH1750 measures light far better.
// BMP280 pressure (recordBMEpressure_2byteInt) is not used in this build.
```

Both profiles store **512 records** in the RTC module's 4k EEprom — about **5.3 days at a
15-minute interval** (or 8.5 hours at 1-minute, useful for quick shakedown runs).
All other `#define` sensor lines in the sketch must stay commented out.

---

## Stage 0 — Parts, tools & software (once per batch)

### Bill of materials

Grouped by subsystem so you can kit each build stage separately — for batch work, count
out and bag each group per unit before the session starts.

**Core logger** (Stages 1–3, every unit):

| # | Component | Connects to | Per unit | × N units |
|---|-----------|-------------|---------:|----------:|
| 1 | Pro Mini clone, **3.3 V / 8 MHz** ATmega328P | — | 1 | |
| 2 | DS3231 RTC module with AT24C32 EEprom & CR2032 holder | A4/A5 (I2C), SQW→D2, VCC, GND | 1 | |
| 3 | CR2032 lithium coin cell (name brand; no rechargeable LIR2032) | RTC module holder | 1 | |
| 4 | Header pins (6-pin UART strip + offcuts) | programming edge | 1 strip | |
| 5 | 26 AWG wire / resistor-leg offcuts + heat-shrink | module-to-module joins | — | |
| 6 | Double-sided foam tape | between the two boards | — | |

**Indicator LED** (Stage 4):

| # | Component | Connects to | Per unit | × N units |
|---|-----------|-------------|---------:|----------:|
| 7 | Common-cathode 5 mm RGB LED, diffused (red leg cut at assembly) | cathode→A0, green→A1, blue→A2 | 1 | |

No series resistor — channels are driven through the 328p's internal pullups.

**NTC / LDR burn-in circuit** (Stage 5):

| # | Component | Connects to | Per unit | × N units |
|---|-----------|-------------|---------:|----------:|
| 8 | 10 kΩ **1% metal-film** resistor (reference — precision matters here) | D6 → common node | 1 | |
| 9 | 10 kΩ NTC thermistor, β≈3950 | D7 → common node | 1 | |
| 10 | 300 Ω resistor, any type | D8 → common node | 1 | |
| 11 | LDR, e.g. GL5528 | D9 → common node | 1 | |
| 12 | 0.1 µF (104) ceramic capacitor | common node → GND | 1 | |

**BMP280 temperature sensor** (Stage 7):

| # | Component | Connects to | Per unit | × N units |
|---|-----------|-------------|---------:|----------:|
| 13 | BMP280 breakout module, 3.3 V, I2C address 0x76 | VCC, GND, SDA (A4 side), SCL (A5 side) | 1 | |

Buy genuine-looking GY-BMP280-3.3 boards; some "BMP280" listings ship BME280s (also fine —
same library, chip ID 0x60 instead of 0x58 in `07_SensorTest`) or fakes that fail the scan.

**BH1750 light sensor** (Stage 7):

| # | Component | Connects to | Per unit | × N units |
|---|-----------|-------------|---------:|----------:|
| 14 | BH1750 breakout (GY-302 or GY-30), I2C address 0x23 | VCC, GND, SDA (A4 side), SCL (A5 side) | 1 | |

Leave the ADDR pin unconnected/low (= 0x23). Both I2C sensors share the same four bus
wires — chain them rather than running separate leads back to the ProMini.

**[OPTIONAL] extras:**

| # | Component | Used for | Per unit | × N units |
|---|-----------|----------|---------:|----------:|
| 15 | 220–1000 µF tantalum capacitor, ≥25 V rating | rail buffering (Stage 7 step 6) | 1 | |
| 16 | 50 ml falcon tube + 3D-printed rail + desiccant | deployment housing (Appendix C) | 1 | |

### Tools

Soldering iron + fine solder, flush cutters, **3.3 V** USB-UART adapter (FTDI Basic or
CP2102, 6-pin), multimeter with a **µA range**, DVM leads, isopropyl alcohol + brush for
flux cleanup, hot glue (battery retention).

### Software (install once)

1. [Arduino IDE](https://www.arduino.cc/en/software). Board setting for everything in this
   guide: **Tools ▸ Board ▸ Arduino Pro or Pro Mini**, **Processor ▸ ATmega328P (3.3 V, 8 MHz)**.
2. Libraries (Sketch ▸ Include Library ▸ Manage Libraries…):
   - **LowPower** by LowPowerLab — required by the main sketch and `06_SleepCurrent`
   - **hp_BH1750** by Stefan Armborst — BH1750 (main sketch + `07_SensorTest`)
   - **ForcedBMX280** by soylentOrange — BMP280 (main sketch + `07_SensorTest`)
3. Serial monitor is always **500000 baud** (it is 8 MHz ÷ 16 — the only fast rate with
   zero clock error on these boards). Garbled text almost always means the wrong baud rate.

> **VERIFY (Stage 0):** `tests/01_UploadBlink` compiles in the IDE with the board settings
> above, and the three libraries appear in Sketch ▸ Include Library.
> *(The test sketches in this repo have been desk-reviewed but arrive compile-unverified —
> build one of each in the IDE before the batch session starts.)*

---

## Stage 1 — Pro Mini preparation

**Do this first, on every board:** upload `tests/01_UploadBlink` over the UART adapter.
Per the tutorial: *do not progress with the build until you have confirmed a working
bootloader.* A board that won't take an upload is a reject — no point modifying it.

Then, per board:

1. Solder the 6-pin UART header to the programming edge; trim the three tails flush on the
   back so they can't short against the RTC module later.
2. **Remove the voltage regulator** — clip it away from the 2-leg side. Left in place it
   back-leaks ~80 µA, which alone would kill a CR2032 in weeks.
3. **Remove the power-LED limit resistor** (next to the regulator) with the iron tip.
   Removing the resistor rather than the LED avoids damage to nearby traces.
4. **Remove the reset switch** (clip it off). This logger is only ever started through
   serial handshakes — an exposed reset button inside a deployment housing is a liability.
5. Clean flux residue with isopropyl (flux is mildly conductive and shows up as
   mystery-µA later).

<img src="https://thecavepearlproject.org/wp-content/uploads/2023/11/jumper14_blinktesting_900pixw.jpg" width="220" alt="bootloader blink test"> <img src="https://thecavepearlproject.org/wp-content/uploads/2023/11/pm1_cliprega_900pixw_square.jpg" width="220" alt="clipping the regulator"> <img src="https://thecavepearlproject.org/wp-content/uploads/2023/11/pm6_ledlimitresistorb_900pixwsq.jpg" width="220" alt="removing power LED resistor"> <img src="https://thecavepearlproject.org/wp-content/uploads/2023/11/pm4_clipresetswitch_900pixwsq.jpg" width="220" alt="clipping the reset switch">

*L→R: blink test before any mods · clip the regulator (2-leg side) · lift the power-LED
limit resistor · clip the reset switch. Header detail:
[trimming the UART pin tails](https://thecavepearlproject.org/wp-content/uploads/2022/02/prom_headerpins_640pixw.jpg).*

> **VERIFY (Stage 1):** re-run `tests/01_UploadBlink`. Heartbeat still prints at 500000
> baud and the **red D13 LED** still blinks (green/blue come later — red-only = PASS).
> The board now only powers up while the UART adapter is connected: expected.

---

## Stage 2 — RTC module preparation

The DS3231 module is modified to run permanently from the coin cell ("backup" input):

1. **Remove the 200 Ω charging resistor** — it would try to charge the non-rechargeable
   CR2032 (the cell tolerates small reverse currents, but the charge circuit also leaks).
2. **Remove the power-LED limit resistor.**
3. **Lift/cut the module's main VCC leg** (2nd pin from the corner on the header) so the
   DS3231 runs from the battery input only.
4. **Bridge VCC to the coin-cell side** at the black-ring end of the module's diode, so
   the logger rail and RTC share the cell.
5. **Clip the 32 kHz header pin** — that output is non-functional when running from Vbat.
6. **[OPTIONAL]** conformal-coat the module (avoiding the battery clip and header) after
   it passes Stage 3 testing.

<img src="https://thecavepearlproject.org/wp-content/uploads/2022/02/rtc_clipvcc_600pixw.jpg" width="220" alt="clipping RTC VCC leg"> <img src="https://thecavepearlproject.org/wp-content/uploads/2022/02/rtc_disconnectpowerled_640pixw.jpg" width="220" alt="disconnecting RTC power LED"> <img src="https://thecavepearlproject.org/wp-content/uploads/2022/02/rtc_bridgevcc-bkuppower_640pixw.jpg" width="220" alt="bridging VCC to backup power"> <img src="https://thecavepearlproject.org/wp-content/uploads/2022/02/rtc_modscomplete_640pixw.jpg" width="220" alt="all RTC mods complete">

*L→R: clip the VCC leg · disconnect the power LED · bridge VCC to the coin-cell side at
the diode's black ring · all mods done (use this last photo as your visual-QC reference).*

> **VERIFY (Stage 2):** visual only — four removals confirmed under magnification, no
> solder bridges, battery clip spring intact. Electrical verification comes at Stage 3.

---

## Stage 3 — Joining the two modules

1. Stick double-sided foam tape across the RTC module's chips; press the Pro Mini
   (component side out) onto it, programming header away from the battery holder.
2. Extend **A4 → SDA** and **A5 → SCL** with resistor-leg offcuts in heat-shrink.
   **These two must cross over each other** to land on the right RTC pads — straight-through
   is the single most common Stage 3 mistake.
3. Extend **VCC** and **GND** rails between the boards the same way.
4. Thread the jumpers through the RTC module's pass-through port, press, solder all four.
5. Solder the alarm line: **RTC SQW → Pro Mini D2** (resistor leg or 26 AWG).
6. Insert a CR2032.

<img src="https://thecavepearlproject.org/wp-content/uploads/2023/11/p3b_dstape2_900pixw.jpg" width="220" alt="foam tape on RTC chips"> <img src="https://thecavepearlproject.org/wp-content/uploads/2022/02/i2cbus_2_hscrossover_600pixw.jpg" width="220" alt="I2C jumpers crossing over"> <img src="https://thecavepearlproject.org/wp-content/uploads/2022/02/i2cbus_5_threadpassthrough_600pixw.jpg" width="220" alt="threading jumpers through pass-through"> <img src="https://thecavepearlproject.org/wp-content/uploads/2023/11/p3f_solderi2cjumpers_900pixw.jpg" width="220" alt="soldering the four I2C joints">

*L→R: foam tape across the RTC chips · the A4/A5 **crossover** in heat-shrink · threading
through the pass-through port · soldering the four joints.*

<img src="https://thecavepearlproject.org/wp-content/uploads/2022/02/d2alarmline_1_600pixh.jpg" width="220" alt="32kHz pin trimmed, SQW header"> <img src="https://thecavepearlproject.org/wp-content/uploads/2022/02/d2_surfacejumper_3_640pixw.jpg" width="220" alt="finished SQW to D2 jumper">

*The alarm line: 32 kHz pin already trimmed, then the heat-shrinked SQW → D2 jumper.*

> **VERIFY (Stage 3):** with UART connected, run in order:
> 1. `tests/02_I2C_Scanner` → `CORE CHECK: PASS` (0x68 + 0x57 both found).
>    FAIL → re-flow the four joints; check the A4/A5 crossover.
> 2. `tests/03_RTC_Test` → send `T` once to set the clock, then all four summary lines
>    PASS. `D2 Alarm: FAIL` → re-flow the SQW→D2 jumper.
> 3. `tests/04_EEprom_Test` → type `Y` → `EEPROM TEST: PASS (0 mismatches)`.
> Then pull the UART adapter and confirm the RTC keeps time on the coin cell alone:
> reconnect after a minute and re-run `03_RTC_Test` — OSF must still read PASS.

---

## Stage 4 — RGB indicator LED

The main sketch expects `LED_GndGB_A0_A2`: common-cathode RGB with the **red leg cut off**
(the onboard D13 red LED + its 1 kΩ resistor stay and serve as the red channel).

1. Cut the RGB LED's **red** leg at the body.
2. Solder: longest leg (common cathode) → **A0**, green → **A1**, blue → **A2**.
3. No series resistor needed — the code lights the channels through the 328p's internal
   pullups (~36 kΩ), keeping LED current under 50 µA.

<img src="https://thecavepearlproject.org/wp-content/uploads/2022/03/20230523_leda0-2_640pixw.jpg" width="300" alt="RGB LED on A0-A2">

*RGB LED seated on A0–A2 with the red leg removed.*

> **VERIFY (Stage 4):** `tests/01_UploadBlink` again — now all three colours must rotate:
> RED (D13) → GREEN → BLUE, matching the serial monitor's announcements. A colour that
> never lights = wrong leg order; all dark = cathode not on A0.

---

## Stage 5 — NTC / LDR burn-in circuit

Resistances are read by timing a capacitor's rise through each component using the
328p's Input Capture Unit — no analog pins used
([method](https://thecavepearlproject.org/2019/03/25/using-arduinos-input-capture-unit-for-high-resolution-sensor-readings/)).

Solder, with each component's far ends joined at a common node with the 0.1 µF cap to GND:

| Pin | Component |
|-----|-----------|
| D6 | 10 kΩ 1% metal-film **reference** resistor |
| D7 | 10 kΩ NTC thermistor (β 3950) |
| D8 | 300 Ω resistor (the ICU timing pin) |
| D9 | LDR (GL5528) |
| GND | 0.1 µF (104) ceramic cap from the common node |

<img src="https://thecavepearlproject.org/wp-content/uploads/2022/02/ntccds_1_640pixw.jpg" width="220" alt="NTC and LDR component layout on D6-D9"> <img src="https://thecavepearlproject.org/wp-content/uploads/2022/02/ntccds_2_640pixw.jpg" width="220" alt="components joined at common node"> <img src="https://thecavepearlproject.org/wp-content/uploads/2022/02/ntccds_3_640pixw.jpg" width="220" alt="104 capacitor completes the timing circuit"> <img src="images/20240713_2PartLogger_schematic_400x240pixw.png" width="280" alt="schematic with D6-D9 node">

*L→R: component layout on D6–D9 · far ends gathered into the common node · the 104 cap
completing the ICU timing circuit · the same node on the schematic (local copy).*

> **VERIFY (Stage 5):** no dedicated test sketch — the burn-in run in Stage 6 exercises
> this circuit. Visual check: all five components share one node, cap to GND, no bridges
> between D6–D9. (After Stage 6 starts, sane values are: NTC ≈ 10000 Ω at ~25 °C, warming
> it with a finger drops the value; LDR swings hugely between room light and darkness.)

---

## Stage 6 — Calibration, main sketch & burn-in run

Per unit, UART connected:

<img src="https://thecavepearlproject.org/wp-content/uploads/2023/12/interconnecteduartpromini_1200pixw.jpg" width="280" alt="UART adapter connected to logger"> <img src="https://thecavepearlproject.org/wp-content/uploads/2023/11/contactpoints_calvref_large.jpg" width="280" alt="multimeter contact points for Vref calibration">

*UART hookup for all Stage 6 work, and the VCC/GND contact points to probe while
calibrating the voltage reference.*

1. **Calibrate the voltage reference:** run `tests/05_VccCal`, measure VCC↔GND with your
   multimeter, type the millivolts (e.g. `3302`), and **record the printed constant on the
   unit's checklist**. Each 328p's internal bandgap is different; uncalibrated readings
   can be off by >100 mV, which corrupts the low-battery shutdown logic.
2. **Measure sleep current:** run `tests/06_SleepCurrent`, follow its banner (unplug UART,
   meter in µA range in series with the coin cell). **Target ≤ 2 µA**; >100 µA = clone
   chip or regulator remnant — reject or rework the unit. Record the value.
3. **Flash the main sketch** (`2-Part_ProMiniFalconTubeLogger`) with the **BURN-IN
   profile** defines from the top of this document.
4. Open the serial monitor at 500000 baud. In the start menu:
   - `[2]` set the RTC clock (this also clears any power-loss flag)
   - `[99]` → `[10]` enter the calibration constant from step 1
   - `[3]` set a 15-minute interval (or 1 minute for a quick shakedown)
   - `[6]` START logging — the LED pips confirm sampling has begun
   - the menu times out after 8 minutes of inactivity; reconnect to restart it
5. Disconnect UART, note the start time on the checklist, and leave the unit running on
   coin cell for **24–72 h**.
6. **Download & inspect:** reconnect UART (the running logger restarts into its menu),
   choose `[1]` DOWNLOAD, and copy the CSV from the monitor (PuTTY/CoolTerm/Termite handle
   big transfers more gracefully). A healthy unit shows:
   - no missed intervals (timestamps evenly spaced),
   - battery (LowestBattery) flat or gently declining — a new CR2032 sags under load by
     only tens of mV; a steep slope or >100 mV random jumps = weak cell or bad battery
     contact (hot-glue the cell in place),
   - RTC temperature tracking the room, NTC ohms moving inversely to temperature, LDR
     tracking day/night.

<img src="https://thecavepearlproject.org/wp-content/uploads/2023/07/20230725_bmp280_longburntestgraph_460pixw.jpg" width="320" alt="healthy long-run battery curve with BMP280"> <img src="https://thecavepearlproject.org/wp-content/uploads/2023/07/20230725_bh1750_longburntestgraph_460pixw.jpg" width="320" alt="healthy long-run battery curve with BH1750">

*What a healthy battery curve looks like over a full deployment (tutorial's 11–12 month
runs with BMP280 and BH1750) — your 24–72 h burn-in should sit on the flat left end of
curves like these.*

> **VERIFY (Stage 6):** checklist gets: calibration constant, sleep current µA, burn-in
> start/end, "download clean Y/N". Any unit with gaps, resets, or battery sag fails QC.

---

## Stage 7 — Deployment sensors & final QC

1. Wire the **BMP280** and **BH1750** modules to the shared I2C bus: VCC, GND, SDA (A4
   side), SCL (A5 side). Keep leads short; both boards are 3.3 V-safe as used here.
2. Run `tests/02_I2C_Scanner` → sensor lines now show `BMP280 found, BH1750 found`.
3. Run `tests/07_SensorTest` → both sensors report; finger on the BMP280 raises the
   temperature, covering the BH1750 drops lux toward 0. Plausible-but-frozen values mean
   a dead sensor on a good bus.
4. **Re-flash the main sketch with the DEPLOY profile** (LDR define now commented out).
5. Repeat the start-menu sequence from Stage 6 step 4 (clock, constant, interval, START).
   The startup screen must show 8 bytes/record and list: Battery, RTC °C, NTC, BH1750 lux,
   BMP temp. A `not PowerOfTwo → MUST CHANGE CONFIG!` shutdown means the defines don't
   match the profile — fix and re-flash.
6. **[OPTIONAL]** add the rail buffer capacitor (220–1000 µF tantalum across VCC/GND,
   [placement photo](https://thecavepearlproject.org/wp-content/uploads/2023/11/2023_indicator-on1000ufcap_900pixw.jpg))
   if a unit shows voltage-sag resets in burn-in data; costs only ~15–25 nA of leakage.
7. Hot-glue the coin cell against its spring contact, run a final short logging interval
   (1-minute interval for an hour), download, and sign off the checklist.

<img src="https://thecavepearlproject.org/wp-content/uploads/2022/02/bh1750dupontcable_640pixw.jpg" width="240" alt="Dupont cable I2C connection to sensor"> <img src="https://thecavepearlproject.org/wp-content/uploads/2023/12/2025_hotgluebattery1_660pixw.jpg" width="240" alt="hot glue on battery holder"> <img src="https://thecavepearlproject.org/wp-content/uploads/2023/12/2025_hotgluebattery4_660pixw.jpg" width="240" alt="glue conformed around coin cell">

*L→R: F-F Dupont leads carrying the I2C bus out to a sensor (remember SDA/SCL crossover) ·
hot-glue drops in the holder · the glue conformed around the seated cell.*

> **VERIFY (Stage 7):** checklist gets: both sensor addresses seen, sensor wiggle test
> PASS, DEPLOY profile flashed, final 1-h shakedown clean. Unit is ready to deploy.

---

## Troubleshooting

| Symptom | Likely cause → fix |
|---|---|
| Garbled serial monitor text | Wrong baud — set **500000** in the monitor's dropdown and reopen it |
| Upload fails / no bootloader | Wrong board setting (must be 3.3 V/8 MHz), TX/RX swapped, or dead clone — reject |
| `02_I2C_Scanner`: no devices | VCC/GND joints between modules; UART adapter set to 5 V instead of 3.3 V |
| Scanner finds 0x68 but not 0x57 | EEprom corner pads of RTC module bridged/cold joint |
| `03_RTC_Test` D2 Alarm FAIL | SQW→D2 jumper cold joint, or wrong RTC pad used |
| OSF flag keeps returning / date shows `*2000*` | Coin cell not making contact (hot-glue it), VCC-to-Vbat bridge missed in Stage 2 |
| Sleep current 50–100 µA | Regulator not fully removed — clip remaining legs |
| Sleep current >100 µA | Clone/fake 328p (won't sleep below ~100 µA) — reject board |
| Sleep current 3–10 µA | Flux residue (clean with isopropyl), meter burden voltage, optional cap leakage |
| `not PowerOfTwo → MUST CHANGE CONFIG!` at startup | Enabled `#define`s don't sum to 1/2/4/8/16 bytes — use a profile from this guide verbatim |
| Logger starts then dies randomly on battery | Weak coin-cell spring contact; cell below ~2850 mV under load |
| Readings stop partway through a deployment | EEprom full (512 records) — normal; shorten interval math or add 32k module |

---

## Appendix A — the powers-of-2 rule

Every enabled sensor `#define` adds bytes to each record (the count is printed at
startup). The total **must be 1, 2, 4, 8 or 16** because records must align with the
EEprom's internal 32-byte page boundaries — a 6-byte record would eventually straddle a
page edge, and the chip wraps around *within* the page instead of advancing, silently
corrupting data. The sketch checks this at startup and shuts down rather than log garbage.
That is why both profiles above pad or trim to exactly 8 bytes.

| This build's byte costs | Bytes |
|---|---|
| `logLowestBattery_1byte` | 1 |
| `logRTC_Temperature_1byte` | 1 |
| `logCurrentBattery_2byte` (padding) | 2 |
| `readD7resistorwD6ref_2byte` (NTC) | 2 |
| `readD9resistorwD6ref_2byte` (LDR) | 2 |
| `readBh1750_LUX_2byte` | 2 |
| `recordBMEtemp_2byteInt` (BMP280) | 2 |

## Appendix B — [OPTIONAL] storage upgrade

A 32k EEprom module (0x50) raises capacity from 512 to 4096 records: change
`EEpromI2Caddr` to `0x50` and `totalBytesOfStorage` to `32768` in the main sketch (both
the STEP0 block *and* the duplicate defines ~40 lines below it), and re-run
`tests/04_EEprom_Test` with its two `#define`s updated to match. Not used in this build.

## Appendix C — [OPTIONAL] falcon-tube housing

The repo root contains `2PartProMiniLoggerRail.stl` (logger carrier rail) and
`2PartLoggerr_30mlMountingBracket_v9.stl` (tube mounting bracket) for a 50 ml falcon-tube
deployment housing. Print, slide the finished logger onto the rail, desiccant pack in the
tube cap, and see the tutorial video for the o-ring details. Not part of batch bench QC.

<img src="https://thecavepearlproject.org/wp-content/uploads/2022/03/2partloggerrails_2024.jpg" width="280" alt="3D printed rail system"> <img src="https://thecavepearlproject.org/wp-content/uploads/2022/03/2partrails5_intube_1024pixw_2024.jpg" width="280" alt="logger on rail inside tube with desiccant">

*The printed rail system and a completed logger in its tube with desiccant packs.*
