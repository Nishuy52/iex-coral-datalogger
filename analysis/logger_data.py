"""
Core parsing & unit-conversion for 2-Part ProMini EEprom Logger downloads.

A logger "download" is whatever you captured from the serial monitor with [1] DOWNLOAD.
That capture usually contains menu boilerplate (banners, "Logger:", "Deployment:",
the menu, etc.) wrapped around the actual CSV block, which looks like:

    UnixTime,LoBat[mv], RTC[°C], D7[Ω], D9[Ω], C.Bat[mv],
    1718553600,3302, 22.50, 10180, 53120, 3305,
    1718553610,3301, 22.50, 10175, 41980, 3304,
    ...

`load_capture()` finds that block (even amid the noise), tidies the column names,
adds a real datetime, and converts the NTC (D7) resistance to °C.

The other two scripts (process_logger.py, plot_logger.py) import from this file.
"""
from __future__ import annotations

import re
from pathlib import Path

import numpy as np
import pandas as pd

# --- NTC conversion constants ------------------------------------------------
# Match the build BOM: 10 kΩ NTC thermistor, β ≈ 3950, 10 kΩ reference resistor.
# Override these if you used a different thermistor.
NTC_R0_OHM = 10000.0    # nominal NTC resistance at the reference temperature
NTC_T0_C = 25.0         # reference temperature (°C)
NTC_BETA = 3950.0       # beta coefficient (kelvin)

# --- Column name mapping -----------------------------------------------------
# Keys are the *normalised* raw header labels (lowercase, only a-z0-9 kept).
# Values are tidy, code-friendly column names. Anything not listed is kept as-is,
# so a previously-cleaned CSV re-loads unchanged.
COLUMN_ALIASES = {
    "unixtime": "unix_time",
    "lobatmv": "lobat_mv",      # LoBat[mv]   - lowest battery during EEprom write
    "rtcc": "rtc_c",            # RTC[°C]     - DS3231 internal temperature
    "d7": "ntc_ohm",            # D7[Ω]       - NTC thermistor
    "d9": "ldr_ohm",            # D9[Ω]       - LDR / photoresistor
    "d6": "d6_ohm",             # D6[Ω]       - (e360 wiring variant)
    "bh1750lux": "lux",         # Bh1750[Lux]
    "cbme": "bmp_c",            # [°C]bmE     - BMP/BME280 temperature
    "mbarbme": "bmp_mbar",      # [mbar]bmE   - BMP/BME280 pressure
    "rhbme": "bme_rh",          # [%rh]bmE    - BME280 humidity
    "cbatmv": "cbat_mv",        # C.Bat[mv]   - current battery (after wake)
    "pircount": "pir_count",
}

# Friendly labels for plotting / printing, keyed by tidy column name.
COLUMN_LABELS = {
    "lobat_mv": "Lowest battery (mV)",
    "cbat_mv": "Current battery (mV)",
    "rtc_c": "RTC temperature (°C)",
    "ntc_c": "NTC temperature (°C)",
    "ntc_ohm": "NTC resistance (Ω)",
    "ldr_ohm": "LDR resistance (Ω)",
    "d6_ohm": "D6 resistance (Ω)",
    "lux": "BH1750 light (lux)",
    "bmp_c": "BMP/BME temperature (°C)",
    "bmp_mbar": "BMP/BME pressure (mbar)",
    "bme_rh": "BME humidity (%RH)",
    "pir_count": "PIR count",
}


def _norm(label: str) -> str:
    """Normalise a header label to lowercase alphanumerics only."""
    return re.sub(r"[^a-z0-9]", "", str(label).lower())


def _split(line: str) -> list[str]:
    return [c.strip() for c in line.split(",")]


def find_data_block(text: str):
    """Return (header_fields, data_rows) for the largest CSV block in `text`.

    Handles a capture that contains several DOWNLOAD attempts by keeping the
    block with the most data rows. Returns None if no block is found.
    """
    lines = text.splitlines()
    best = None
    i, n = 0, len(lines)
    while i < n:
        cells = _split(lines[i])
        if cells and _norm(cells[0]) == "unixtime":
            header = [c for c in cells if c != ""]
            rows, j = [], i + 1
            while j < n:
                rc = _split(lines[j])
                if not rc or not re.fullmatch(r"\d+", rc[0] or ""):
                    break  # first cell is no longer an integer unix time -> block ended
                rows.append(rc)
                j += 1
            if best is None or len(rows) > len(best[1]):
                best = (header, rows)
            i = max(j, i + 1)
        else:
            i += 1
    return best


def ntc_ohm_to_c(resistance) -> pd.Series:
    """Convert NTC resistance (ohms) to °C using the Beta-parameter equation."""
    r = pd.to_numeric(pd.Series(resistance), errors="coerce").astype(float)
    t0 = NTC_T0_C + 273.15
    with np.errstate(invalid="ignore", divide="ignore"):
        inv_t = (1.0 / t0) + (1.0 / NTC_BETA) * np.log(r / NTC_R0_OHM)
        temp_c = (1.0 / inv_t) - 273.15
    return temp_c.where(r > 0)


def load_capture(source, convert: bool = True) -> pd.DataFrame:
    """Load a logger capture (file path or raw text) into a tidy DataFrame.

    Works on both a raw serial capture and an already-cleaned CSV produced by
    process_logger.py. `convert=True` adds derived columns (datetime, ntc_c).
    """
    if isinstance(source, (str, Path)) and Path(str(source)).exists():
        text = Path(source).read_text(encoding="utf-8", errors="replace")
    else:
        text = str(source)

    block = find_data_block(text)
    if block is None:
        return pd.DataFrame()

    header, rows = block
    ncol = len(header)
    fixed = [(r[:ncol] + [""] * (ncol - len(r))) for r in rows]
    df = pd.DataFrame(fixed, columns=header)

    # Tidy column names (unknown labels are kept, so clean CSVs pass through).
    df = df.rename(columns=lambda c: COLUMN_ALIASES.get(_norm(c), str(c).strip()))
    df = df.loc[:, [c for c in df.columns if c != ""]]

    # Numeric coercion for everything except an existing datetime column.
    for col in df.columns:
        if col != "datetime":
            df[col] = pd.to_numeric(df[col], errors="coerce")

    if "unix_time" not in df.columns:
        return pd.DataFrame()
    df = df.dropna(subset=["unix_time"]).reset_index(drop=True)
    df["unix_time"] = df["unix_time"].astype("int64")

    if convert:
        # NOTE: the logger stores whatever wall-clock you set on the RTC, so this
        # datetime reflects *logger local time*, not necessarily UTC.
        df["datetime"] = pd.to_datetime(df["unix_time"], unit="s")
        if "ntc_ohm" in df.columns:
            df["ntc_c"] = ntc_ohm_to_c(df["ntc_ohm"]).round(2)
        # Put datetime / unix_time first for readability.
        front = [c for c in ("datetime", "unix_time") if c in df.columns]
        df = df[front + [c for c in df.columns if c not in front]]

    return df


def label_for(column: str) -> str:
    return COLUMN_LABELS.get(column, column)
