"""
plot_logger.py - visualise a logger capture or a cleaned CSV.

Usage:
    python plot_logger.py capture.txt                # raw capture is fine
    python plot_logger.py burnin_clean.csv           # or a cleaned CSV
    python plot_logger.py capture.txt --no-show      # save PNGs only
    python plot_logger.py capture.txt -o charts/     # choose output folder

Produces a stacked overview figure (one panel per sensor group) plus the
individual panels, saved as PNGs, and opens an interactive window unless
--no-show is given. Auto-adapts to whichever channels are present, so it
works for the BURN-IN, DEPLOY and CORE-TEST profiles.
"""
from __future__ import annotations

import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import logger_data as L  # noqa: E402

# Sensor groups: title -> (list of candidate columns, y-axis label, log-y?)
PANELS = [
    ("Battery", ["lobat_mv", "cbat_mv"], "millivolts", False),
    ("Temperature", ["rtc_c", "ntc_c", "bmp_c"], "°C", False),
    ("LDR / resistance", ["ldr_ohm", "d6_ohm"], "ohms", True),
    ("Light", ["lux"], "lux", False),
    ("Pressure", ["bmp_mbar"], "mbar", False),
    ("Humidity", ["bme_rh"], "%RH", False),
    ("Motion", ["pir_count"], "count", False),
]


def main(argv=None) -> int:
    p = argparse.ArgumentParser(description="Plot 2-Part ProMini logger data.")
    p.add_argument("input", help="raw capture (.txt) or cleaned CSV")
    p.add_argument("-o", "--outdir", default=None,
                   help="folder for PNGs (default: alongside the input file)")
    p.add_argument("--no-show", action="store_true", help="save PNGs without opening a window")
    args = p.parse_args(argv)

    if not os.path.exists(args.input):
        print(f"ERROR: input not found: {args.input}", file=sys.stderr)
        return 2

    import matplotlib
    if args.no_show:
        matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    import matplotlib.dates as mdates

    df = L.load_capture(args.input)
    if df.empty or "datetime" not in df.columns:
        print("No plottable data found in the capture.", file=sys.stderr)
        return 1

    outdir = args.outdir or os.path.dirname(os.path.abspath(args.input))
    os.makedirs(outdir, exist_ok=True)
    stem = os.path.splitext(os.path.basename(args.input))[0]

    # Keep only panels that actually have data.
    active = []
    for title, cols, ylab, logy in PANELS:
        present = [c for c in cols if c in df.columns and df[c].notna().any()]
        if present:
            active.append((title, present, ylab, logy))

    if not active:
        print("Data table found but none of the known sensor columns are present.", file=sys.stderr)
        return 1

    x = df["datetime"]

    def style_axis(ax, title, ylab, logy):
        ax.set_title(title, loc="left", fontsize=10, fontweight="bold")
        ax.set_ylabel(ylab)
        ax.grid(True, alpha=0.3)
        if logy:
            ax.set_yscale("log")
        ax.legend(fontsize=8, loc="best")

    # --- combined overview figure ---
    fig, axes = plt.subplots(len(active), 1, figsize=(11, 2.4 * len(active)),
                             sharex=True, constrained_layout=True)
    if len(active) == 1:
        axes = [axes]
    for ax, (title, cols, ylab, logy) in zip(axes, active):
        for c in cols:
            ax.plot(x, df[c], marker=".", ms=3, lw=1, label=L.label_for(c))
        style_axis(ax, title, ylab, logy)
    axes[-1].set_xlabel("Time (logger clock)")
    axes[-1].xaxis.set_major_formatter(mdates.DateFormatter("%m-%d %H:%M"))
    fig.suptitle(f"{stem} - {len(df)} records", fontsize=12)
    fig.autofmt_xdate()

    overview = os.path.join(outdir, f"{stem}_overview.png")
    fig.savefig(overview, dpi=130)
    print(f"Saved {overview}")

    # --- individual panels ---
    for title, cols, ylab, logy in active:
        f1, a1 = plt.subplots(figsize=(11, 3.5), constrained_layout=True)
        for c in cols:
            a1.plot(x, df[c], marker=".", ms=3, lw=1, label=L.label_for(c))
        style_axis(a1, title, ylab, logy)
        a1.set_xlabel("Time (logger clock)")
        a1.xaxis.set_major_formatter(mdates.DateFormatter("%m-%d %H:%M"))
        f1.autofmt_xdate()
        safe = title.split()[0].lower().replace("/", "")
        path = os.path.join(outdir, f"{stem}_{safe}.png")
        f1.savefig(path, dpi=130)
        print(f"Saved {path}")
        if args.no_show:
            plt.close(f1)

    if not args.no_show:
        plt.show()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
