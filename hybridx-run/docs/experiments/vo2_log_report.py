#!/usr/bin/env python3
"""Summarise HybridX Run tuning logs (Logs/vo2_*.csv), for tuning Vo2Config.hpp.

For each log: how many windows were kept and why the others were not, then each
kept window's VO2max worked out the app's way (ACSM cost already logged;
%HRR = (HR - rest) / (max - rest); VO2max = 3.5 + (cost - 3.5) / %HRR; the
50% HRR floor and the 15-95 plausibility range), and their median, which is
the run's figure when at least 5 windows remain. --max-hr / --rest-hr replace
the logged profile, to see what a measured max HR (or Garmin's) would have
given.

Logs from R6b on have a `scale` column: the GPS speed correction (SDK
GpsSpeedFilter) the speed was multiplied by. For those the report also shows
what the same windows would have given on the receiver's raw speed (speed /
scale), i.e. what the app said before R6b. Older logs have no `scale`: their
speed was raw. Standard library only.

Usage:  python3 vo2_log_report.py [--max-hr N] [--rest-hr N] [--rows] log.csv [more.csv ...]
"""
import argparse
import csv
import re
import statistics
import sys

MIN_HRR = 0.50          # Vo2Config kMinHrrPct
VO2_RANGE = (15.0, 95.0)
MIN_WINDOWS = 5


def acsm_cost(m_min, grade_pct):
    """The app's ACSM cost (Vo2Run.cpp acsmRunningVo2): downhill counts as 0."""
    grade = grade_pct / 100.0 if grade_pct > 0 else 0.0
    return 3.5 + 0.2 * m_min + 0.9 * m_min * grade


def read_log(path):
    context, result, rows = "", "", []
    with open(path, newline="") as f:
        body = []
        for line in f:
            if line.startswith("# result"):
                result = line[2:].strip()
            elif line.startswith("#"):
                context = line[2:].strip()
            elif line.strip():
                body.append(line)
    rows = list(csv.DictReader(body))
    return context, result, rows


def profile_from(context):
    m = re.search(r"max_hr (\d+)", context)
    r = re.search(r"rest_hr (\d+)", context)
    return (int(m.group(1)) if m else 0), (int(r.group(1)) if r else 0)


def report(path, args):
    context, result, rows = read_log(path)
    max_hr, rest_hr = profile_from(context)
    max_hr = args.max_hr or max_hr
    rest_hr = args.rest_hr or rest_hr
    print(f"== {path}")
    print(f"   {context}")
    outcomes = {}
    for r in rows:
        outcomes[r["outcome"]] = outcomes.get(r["outcome"], 0) + 1
    print("   windows:", ", ".join(f"{k} {v}" for k, v in sorted(outcomes.items())) or "none")

    estimates = []
    raw_estimates = []   # the same windows on the receiver's raw speed (R6b logs)
    scales = []
    if max_hr <= rest_hr:
        print(f"   no estimate: max HR {max_hr} not above resting HR {rest_hr}")
    else:
        for r in rows:
            if r["outcome"] != "kept":
                continue
            hr, cost = float(r["hr_avg"]), float(r["cost"])
            hrr = (hr - rest_hr) / (max_hr - rest_hr)
            why = ""
            if hrr > 1.0:
                why = "HR above max"
            elif hrr < MIN_HRR:
                why = "too easy"
            else:
                vo2 = 3.5 + (cost - 3.5) / hrr
                if VO2_RANGE[0] <= vo2 <= VO2_RANGE[1]:
                    estimates.append(vo2)
                    scale = float(r.get("scale") or 0.0)
                    if scale > 0.0:
                        raw_cost = acsm_cost(float(r["speed_m_min"]) / scale, float(r["grade_pct"]))
                        raw_estimates.append(3.5 + (raw_cost - 3.5) / hrr)
                        scales.append(scale)
                else:
                    why = f"implausible {vo2:.1f}"
            if args.rows:
                shown = f"{estimates[-1]:.1f}" if not why else why
                print(f"     w{r['window']:>3} {float(r['speed_m_min']):6.1f} m/min  grade {float(r['grade_pct']):5.1f}%  "
                      f"HR {hr:5.1f}  %HRR {hrr * 100:5.1f}  -> {shown}")
    print(f"   profile used: max HR {max_hr}, resting HR {rest_hr}")
    if len(estimates) >= MIN_WINDOWS:
        print(f"   estimate: {statistics.median(estimates):.1f} ml/kg/min from {len(estimates)} windows "
              f"(spread {min(estimates):.1f}-{max(estimates):.1f})")
    else:
        print(f"   estimate: none ({len(estimates)} usable windows, {MIN_WINDOWS} needed)")
    if len(raw_estimates) >= MIN_WINDOWS:
        corrected = statistics.median(estimates)
        raw = statistics.median(raw_estimates)
        print(f"   on the raw receiver speed it would have been {raw:.1f} "
              f"(speed correction x{statistics.mean(scales):.3f} on average, {corrected - raw:+.1f})")
    if result:
        print(f"   watch said: {result}")


def main():
    p = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    p.add_argument("--max-hr", type=int, default=0, help="use this max HR instead of the logged one")
    p.add_argument("--rest-hr", type=int, default=0, help="use this resting HR instead of the logged one")
    p.add_argument("--rows", action="store_true", help="print every kept window")
    p.add_argument("logs", nargs="+")
    args = p.parse_args()
    for path in args.logs:
        report(path, args)
    return 0


if __name__ == "__main__":
    sys.exit(main())
