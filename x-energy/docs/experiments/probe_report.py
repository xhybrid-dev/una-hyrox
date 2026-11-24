#!/usr/bin/env python3
"""The Energy Probe's logs, summed up (x-energy/docs/PROBE.md, "What to send back").

    python3 x-energy/docs/experiments/probe_report.py <Logs dir> [--md report.md]

<Logs dir> is the probe's Logs folder copied off the watch (Apps/EnergyProbe/Logs). Reads
nights.csv, min_*.csv, events.csv and check_*.csv, and prints:

  1. the battery: each night's drain, then each mode against the Off baseline;
  2. coverage: what each sensor delivered, day by day;
  3. the sensor check: what each type the SDK lists said;
  4. a first look at sleep on the nights with movement: when the arm went still and moved
     again (van Hees 2015: no change in arm angle over 5 degrees for 5 minutes or more),
     and the lowest 30 minutes of heart rate. A first look only: nothing here is validated.

Standard library only.
"""

import argparse
import csv
import glob
import os
import statistics
import sys
from datetime import datetime, timezone

FLAGS = {1: "charged", 2: "restarted", 4: "partial", 8: "no battery", 16: "mode changed"}
MODE_ORDER = ["off", "full", "motion", "heart", "heart5"]


def num(v, scale=1.0):
    """A CSV cell as a number divided by scale, or None if empty."""
    if v is None or v == "":
        return None
    try:
        return float(v) / scale
    except ValueError:
        return None


def flags_text(f):
    f = int(f or 0)
    words = [w for bit, w in FLAGS.items() if f & bit]
    return ", ".join(words) if words else ""


def read_csv(path):
    with open(path, newline="") as fh:
        return list(csv.DictReader(fh))


def hhmm(utc):
    return datetime.fromtimestamp(int(utc), tz=timezone.utc).strftime("%H:%M UTC")


def battery(logs, out):
    path = os.path.join(logs, "nights.csv")
    out.append("## 1. Battery\n")
    if not os.path.exists(path):
        out.append("No nights.csv yet: no night has finished.\n")
        return
    rows = read_csv(path)
    out.append("| Night | Mode | Hours | Battery | Drain %/h | mA (mAh drop) | mA (gauge) | Worn min | HR min | Flags |")
    out.append("|---|---|---|---|---|---|---|---|---|---|")
    by_mode = {}
    for r in rows:
        hours = num(r["hours_x100"], 100)
        drain = num(r["drain_pct_h_x100"], 100)
        cap = num(r["cap_ma_x100"], 100)
        gauge = num(r["gauge_ma_x100"], 100)
        start = num(r["batt_start_x100"], 100)
        end = num(r["batt_end_x100"], 100)
        batt = f"{start:.2f} to {end:.2f}" if start is not None and end is not None else "?"
        out.append(
            f"| {r['night']} | {r['mode']} | {hours:.2f} | {batt} | "
            f"{'?' if drain is None else f'{drain:.2f}'} | {'' if cap is None else f'{cap:.2f}'} | "
            f"{'' if gauge is None else f'{gauge:.2f}'} | {r['worn_min']} | {r['hr_min']} | {flags_text(r['flags'])} |"
        )
        clean = int(r["flags"] or 0) & (1 | 8 | 16) == 0 and drain is not None and hours is not None and hours >= 4
        if clean:
            by_mode.setdefault(r["mode"], []).append(drain)
    out.append("")
    out.append("Clean nights only (4 hours or more, no charger, no mode change, battery read at both ends):\n")
    if not by_mode:
        out.append("None yet.\n")
        return
    base = statistics.mean(by_mode["off"]) if "off" in by_mode else None
    out.append("| Mode | Nights | Mean drain %/h | Over Off, %/h | Days on a charge at this rate |")
    out.append("|---|---|---|---|---|")
    for mode in sorted(by_mode, key=lambda m: MODE_ORDER.index(m) if m in MODE_ORDER else 99):
        d = by_mode[mode]
        mean = statistics.mean(d)
        over = "" if base is None else f"{mean - base:+.2f}"
        days = f"{100.0 / (mean * 24):.1f}" if mean > 0 else "?"
        out.append(f"| {mode} | {len(d)} | {mean:.2f} | {over} | {days} |")
    out.append("")
    if base is None:
        out.append("No clean Off night yet, so no baseline to compare with.\n")


def coverage(logs, out):
    out.append("## 2. Coverage (min_*.csv)\n")
    files = sorted(glob.glob(os.path.join(logs, "min_*.csv")))
    if not files:
        out.append("No minute files.\n")
        return {}
    out.append("| File | Rows | Modes | Accel | HR trusted | Beats | Temp | Touch | Gauge mAh | Wakes/row |")
    out.append("|---|---|---|---|---|---|---|---|---|---|")
    nights = {}
    for f in files:
        rows = read_csv(f)
        if not rows:
            continue
        n = len(rows)

        def share(key, test):
            return f"{100.0 * sum(1 for r in rows if test(r.get(key, ''))) / n:.0f}%"

        modes = sorted({r["mode"] for r in rows})
        wakes = statistics.mean(num(r["wakes"]) or 0 for r in rows)
        out.append(
            f"| {os.path.basename(f)} | {n} | {', '.join(modes)} | "
            f"{share('acc_n', lambda v: (num(v) or 0) > 0)} | {share('hr_ok', lambda v: (num(v) or 0) > 0)} | "
            f"{share('beat_n', lambda v: (num(v) or 0) > 0)} | {share('temp_n', lambda v: (num(v) or 0) > 0)} | "
            f"{share('touch', lambda v: v != '')} | {share('batt_mah_x10', lambda v: v != '')} | {wakes:.1f} |"
        )
        for r in rows:
            if r.get("period") == "night":
                nights.setdefault(r["mode"] + " " + f, []).append(r)
    out.append("")
    return nights


def check(logs, out):
    out.append("## 3. Sensor check (check_*.csv)\n")
    files = sorted(glob.glob(os.path.join(logs, "check_*.csv")))
    if not files:
        out.append("No sensor check run yet.\n")
        return
    rows = read_csv(files[-1])
    out.append(f"From {os.path.basename(files[-1])}:\n")
    out.append("| Sensor | Handle | Connected | Samples | Fields | Description | First values |")
    out.append("|---|---|---|---|---|---|---|")
    for r in rows:
        first = " ".join(x for x in (r.get("s0_f0"), r.get("s0_f1"), r.get("s0_f2"), r.get("s0_f3")) if x)
        state = "absent" if r["handle"] == "0" else ("yes" + (" (shared)" if r["shared"] == "1" else "")
                                                     if r["connected"] == "1" else "no")
        out.append(f"| {r['name']} | {r['handle']} | {state} | {r['samples']} | {r['fields']} | "
                   f"{r['desc']} | {first} |")
    out.append("")


def sleep_look(nights, out):
    out.append("## 4. A first look at sleep (not validated)\n")
    shown = 0
    for key, rows in sorted(nights.items()):
        rows = [r for r in rows if (num(r.get("acc_n")) or 0) > 0]
        if len(rows) < 60:
            continue
        # Per minute: the largest change between its 5-second arm angles.
        changes = []
        for r in rows:
            angles = [num(r.get(f"ang{i}"), 10) for i in range(12)]
            angles = [a for a in angles if a is not None]
            big = max((abs(b - a) for a, b in zip(angles, angles[1:])), default=0.0)
            changes.append(big)
        # Still: this minute and the next four all under 5 degrees.
        still = [all(c < 5.0 for c in changes[i:i + 5]) and i + 5 <= len(changes) for i in range(len(changes))]
        first = next((i for i, s in enumerate(still) if s), None)
        last = next((i for i in range(len(still) - 1, -1, -1) if still[i]), None)
        hrs = [num(r.get("hr_mean_x10"), 10) for r in rows]
        lows = []
        for i in range(0, len(hrs) - 29):
            w = [h for h in hrs[i:i + 30] if h is not None]
            if len(w) >= 20:
                lows.append(statistics.mean(w))
        still_share = 100.0 * sum(still) / len(still)
        out.append(f"**{key.split(' ')[0]}, {os.path.basename(key.split(' ', 1)[1])}**: {len(rows)} minutes with "
                   f"movement data, {still_share:.0f}% of them still.")
        if first is not None:
            out.append(f"First still block {rows[first]['local']}, last {rows[last]['local']} "
                       f"(a crude sleep window).")
        if lows:
            out.append(f"Lowest 30-minute heart rate {min(lows):.1f} bpm.")
        out.append("")
        shown += 1
    if shown == 0:
        out.append("No night with an hour or more of movement data yet.\n")


def events(logs, out):
    path = os.path.join(logs, "events.csv")
    out.append("## 5. Events\n")
    if not os.path.exists(path):
        out.append("No events.csv.\n")
        return
    rows = read_csv(path)
    keep = [r for r in rows if r["event"] in ("start", "stop", "connect_failed", "charging", "night", "mode",
                                               "override", "window", "day_mode", "accel", "beats")]
    starts = sum(1 for r in rows if r["event"] == "start")
    out.append(f"{len(rows)} events; the service started {starts} times.\n")
    for r in keep[-40:]:
        out.append(f"- {hhmm(r['utc'])} uptime {r['uptime_s']} s: {r['event']} {r['detail']}")
    out.append("")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("logs", help="the probe's Logs folder")
    ap.add_argument("--md", help="also write the report to this Markdown file")
    a = ap.parse_args()
    if not os.path.isdir(a.logs):
        sys.exit(f"{a.logs}: not a folder")
    out = ["# Energy Probe report\n", f"From `{os.path.abspath(a.logs)}`.\n"]
    battery(a.logs, out)
    nights = coverage(a.logs, out)
    check(a.logs, out)
    sleep_look(nights, out)
    events(a.logs, out)
    text = "\n".join(out)
    print(text)
    if a.md:
        with open(a.md, "w") as fh:
            fh.write(text + "\n")


if __name__ == "__main__":
    main()
