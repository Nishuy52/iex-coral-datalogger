# Per-Unit QC Checklist — 2-Part ProMini EEprom Logger

Print **one copy per logger** and keep it with the unit through the batch. Stage numbers
match [INSTRUCTIONS.md](INSTRUCTIONS.md). Record actual measured values — the calibration
constant and sleep current are unique per chip and cannot be reconstructed later.

| Unit ID | Batch | Builder | Date started |
|---------|-------|---------|--------------|
| U____ | | | |

## Stage sign-off

| ✔ | Stage | Check | Result / value |
|---|-------|-------|----------------|
| ☐ | 1 | Bootloader OK (`01_UploadBlink` heartbeat before any mods) | |
| ☐ | 1 | Regulator + power-LED resistor removed; UART header on | |
| ☐ | 1 | `01_UploadBlink` re-run after mods (onboard D13 blinks, any colour) | |
| ☐ | 1 | RGB LED fitted: all 3 colours rotate in `01_UploadBlink` | |
| ☐ | 1 | NTC/LDR circuit soldered (D6 10k ref, D7 NTC, D8 300Ω, D9 LDR, 104 cap) | |
| ☐ | 2 | RTC module: charge resistor, LED resistor, VCC leg, 32 kHz pin removed | |
| ☐ | 2 | VCC bridged to Vbat side at diode | |
| ☐ | 3 | `02_I2C_Scanner` → CORE CHECK: PASS (0x68 + 0x57) | |
| ☐ | 3 | `03_RTC_Test` → all 4 PASS after clock set (`T`) | OSF cleared: Y / N |
| ☐ | 3 | `04_EEprom_Test` → PASS | mismatches: ______ / 4096 |
| ☐ | 3 | Clock survives 1 min on coin cell only (OSF still clear) | |
| ☐ | 4 | `05_VccCal` constant recorded → entered via menu [10] | constant: ____________ |
| ☐ | 4 | `06_SleepCurrent` measured | ________ µA (target ≤ 2) |
| ☐ | 4 | BURN-IN profile flashed; clock + interval set; START [6] | interval: ______ min |
| ☐ | 4 | Burn-in window | start: __________ end: __________ |
| ☐ | 4 | Burn-in download clean (no gaps/resets, battery curve flat) | records: ______ |
| ☐ | 5 | Sensors wired; `02_I2C_Scanner` sees 0x76 + 0x23 | |
| ☐ | 5 | `07_SensorTest`: temp rises to touch, lux drops when covered | |
| ☐ | 5 | **[OPT]** External EEprom fitted: `02` sees 0x50; `04` ALL PASS | size: ____k |
| ☐ | 5 | Profile flashed; startup lists 8 bytes/rec, 5 channels | ☐ DEPLOY ☐ DEPLOY-XL |
| ☐ | 5 | Coin cell hot-glued; final 1-h shakedown download clean | |

## Final

| Status | ☐ PASSED — ready to deploy | ☐ REWORK: ____________________ | ☐ REJECTED |
|--------|---------------------------|-------------------------------|------------|

Notes (clone chip? resoldered joints? odd readings?):

&nbsp;

&nbsp;

| Signed off by | Date |
|---------------|------|
| | |
