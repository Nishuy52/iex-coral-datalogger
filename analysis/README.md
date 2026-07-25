# Logger data analysis

Process and visualise data downloaded from the 2-Part ProMini EEprom Logger.

## Setup (once)

```
pip install -r requirements.txt
```

## Workflow

1. In the serial monitor, choose `[1] DOWNLOAD` and save everything it prints to a text
   file, e.g. `capture.txt`. The menu banners around the data are fine — the scripts
   find the `UnixTime,...` table automatically.

2. Clean it into a tidy CSV (optional but handy for Excel):

   ```
   python process_logger.py capture.txt
   ```

   Writes `capture_clean.csv` and prints a summary (record count, time span, per-channel
   min/max/mean).

3. Plot it (accepts either the raw capture or the cleaned CSV):

   ```
   python plot_logger.py capture.txt
   ```

   Saves a stacked `*_overview.png` plus one PNG per sensor group, and opens an
   interactive window. Use `--no-show` to save PNGs only, or `-o some/folder` to choose
   the output location.

## What it does

- Finds the data block even amid serial-monitor noise (and picks the largest block if you
  captured several download attempts).
- Renames the cryptic headers (`LoBat[mv]`, `D7[Ω]`, `C.Bat[mv]`, …) to tidy names
  (`lobat_mv`, `ntc_ohm`, `cbat_mv`, …).
- Adds a `datetime` column from the logger's UnixTime (this is the **logger's local
  clock**, i.e. whatever you set on the RTC — not necessarily UTC).
- Converts the **NTC on D7** from ohms to `ntc_c` (°C) using the Beta equation for a
  10 kΩ / β3950 thermistor. Change `NTC_R0_OHM`, `NTC_T0_C`, `NTC_BETA` at the top of
  `logger_data.py` if your thermistor differs.
- Auto-adapts to the profile: works for BURN-IN (NTC + LDR + battery), DEPLOY
  (NTC + BH1750 lux + BMP °C), and the CORE-TEST (battery + RTC °C) sketches.

## Files

- `logger_data.py` - parsing + unit conversion (imported by the other two).
- `process_logger.py` - raw capture -> clean CSV.
- `plot_logger.py` - capture or clean CSV -> charts.
