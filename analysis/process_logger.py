"""
process_logger.py - turn a raw serial-monitor capture into a clean CSV.

Usage:
    python process_logger.py capture.txt
    python process_logger.py capture.txt -o burnin_clean.csv

Reads whatever you saved from the [1] DOWNLOAD output (menu noise and all),
extracts the data table, tidies the columns, adds a datetime column, and
(for the NTC on D7) an ntc_c temperature column, then writes a clean CSV.
"""
from __future__ import annotations

import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import logger_data as L  # noqa: E402


def main(argv=None) -> int:
    p = argparse.ArgumentParser(description="Clean a 2-Part ProMini logger capture into a tidy CSV.")
    p.add_argument("input", help="raw serial capture (.txt/.csv) saved from DOWNLOAD")
    p.add_argument("-o", "--output", help="output CSV path (default: <input>_clean.csv)")
    p.add_argument("--no-convert", action="store_true",
                   help="do not add datetime / NTC °C derived columns")
    args = p.parse_args(argv)

    if not os.path.exists(args.input):
        print(f"ERROR: input not found: {args.input}", file=sys.stderr)
        return 2

    df = L.load_capture(args.input, convert=not args.no_convert)
    if df.empty:
        print("No data rows found. The capture has only the header (empty EEprom) "
              "or no recognisable 'UnixTime,...' block.", file=sys.stderr)
        return 1

    out = args.output or (os.path.splitext(args.input)[0] + "_clean.csv")
    df.to_csv(out, index=False)

    # Summary
    print(f"Parsed {len(df)} records -> {out}")
    if "datetime" in df.columns and len(df):
        span = df["datetime"].iloc[-1] - df["datetime"].iloc[0]
        print(f"  Time span : {df['datetime'].iloc[0]}  ->  {df['datetime'].iloc[-1]}  ({span})")
    data_cols = [c for c in df.columns if c not in ("datetime", "unix_time")]
    print(f"  Columns   : {', '.join(data_cols)}")
    for c in data_cols:
        s = df[c].dropna()
        if len(s):
            print(f"    {L.label_for(c):28s} min={s.min():>10.2f}  max={s.max():>10.2f}  mean={s.mean():>10.2f}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
