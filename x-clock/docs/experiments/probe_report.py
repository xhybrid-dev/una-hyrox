#!/usr/bin/env python3
"""Turns the Clock Probe's logs (the folder Apps/ClockProbe/Logs, copied back over USB) into the answers
to Gate F0's questions (x-clock/docs/NOTES.md).

    python3 x-clock/docs/experiments/probe_report.py <Logs dir> [--md report.md]

It reads events.csv, min_*.csv, days.csv, scan_*.txt and state.txt, and says for each question what the
log shows, or that it cannot tell from this log (and what to do about it).
"""
import csv
import glob
import os
import statistics
import sys
from collections import Counter, defaultdict


def rows(path):
    with open(path, newline="", encoding="utf-8", errors="replace") as f:
        return list(csv.DictReader(f))


def num(v):
    try:
        return int(v)
    except (TypeError, ValueError):
        return None


def kv(detail):
    out = {}
    for part in detail.replace(";", " ").split():
        if "=" in part:
            k, v = part.split("=", 1)
            out[k] = v
    return out


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    md = None
    if "--md" in sys.argv:
        md = sys.argv[sys.argv.index("--md") + 1]
        args = [a for a in args if a != md]
    if not args:
        print(__doc__)
        return 2
    d = args[0]
    out = []
    p = out.append

    events = rows(os.path.join(d, "events.csv")) if os.path.exists(os.path.join(d, "events.csv")) else []
    mins = []
    for f in sorted(glob.glob(os.path.join(d, "min_*.csv"))):
        for r in rows(f):
            r["_day"] = os.path.basename(f)[4:12]
            mins.append(r)
    days = rows(os.path.join(d, "days.csv")) if os.path.exists(os.path.join(d, "days.csv")) else []
    scans = sorted(glob.glob(os.path.join(d, "scan_*.txt")))

    p("# Clock Probe report")
    p("")
    p(f"Logs: `{d}`: {len(events)} events, {len(mins)} minute rows, {len(days)} day rows, {len(scans)} scan dumps.")
    p("")

    # ---- 1 and 3: did it run, and can a face write its own folder -------------------------------
    p("## 1 and 3. An LVGL face ran, and wrote its own folder")
    starts = [e for e in events if e["event"] == "start"]
    p(f"- {len(starts)} start(s) logged. " + ("The log exists, so a Clockface can write its own folder." if events else "No events.csv: either it could not write, or it never ran. Photograph the screen's 'Log write' row."))
    for e in starts[:3]:
        p(f"  - {e['detail']}")
    stops = Counter(e["detail"] for e in events if e["event"] == "stop")
    if stops:
        p("- how it stopped: " + ", ".join(f"{k} x{v}" for k, v in stops.items()))
    fails = sorted({e["detail"] for e in events if e["event"] == "connect_failed"})
    if fails:
        p("- sensors that would not connect: " + ", ".join(fails))
    p("")

    # ---- 2: the shared folder ---------------------------------------------------------------------
    p("## 2. The shared folder")
    sc = [e for e in events if e["event"] == "scan"]
    if sc:
        p(f"- {len(sc)} scans; the last: {sc[-1]['detail']}")
        why = Counter(e["detail"].split(":")[0] for e in sc)
        p("- triggers: " + ", ".join(f"{k} x{v}" for k, v in why.items()))
        changed = [e for e in sc if e["detail"].endswith("changed")]
        p(f"- {len(changed)} scan(s) saw something change (the full dump is in scan_*.txt)")
    if scans:
        last = open(scans[-1], encoding="utf-8", errors="replace").read().splitlines()
        # The newest dump (a file holds one or more '== scan' blocks).
        idx = max(i for i, l in enumerate(last) if l.startswith("== scan"))
        p(f"- the newest dump ({os.path.basename(scans[-1])}):")
        p("")
        for l in last[idx:]:
            if l.startswith(("== scan", "list ", "file ")) or l.startswith("  "):
                p("      " + l.rstrip())
        p("")
        p("  The lines under each `file` are the file's real content: the format the face will read.")
    else:
        p("- no scan dump: the probe could not write one, or never scanned.")
    p("")

    # ---- 4: the wrist ------------------------------------------------------------------------------
    p("## 4. Wrist raises, and the GUI's ticking")
    wr = [e for e in events if e["event"] == "wrist"]
    gs = [e for e in events if e["event"] == "gesture"]
    p(f"- {len(wr)} wrist event(s) logged, {len(gs)} gesture event(s).")
    if wr:
        resume = [num(kv(e["detail"]).get("resume_ago_ms")) for e in wr]
        report = [num(kv(e["detail"]).get("report_ago_ms")) for e in wr]
        resume = [v for v in resume if v is not None]
        report = [v for v in report if v is not None]
        if resume:
            near = sum(1 for v in resume if v < 2000)
            p(f"- time since the GUI's last resume at each raise: median {statistics.median(resume)/1000:.1f} s; "
              f"{near} of {len(resume)} within 2 s (the face woke with the raise).")
        if report:
            quiet = sum(1 for v in report if v > 6000)
            p(f"- time since the GUI's last tick report: median {statistics.median(report)/1000:.1f} s; "
              f"{quiet} of {len(report)} over 6 s (the GUI had not been ticking before the raise).")
    else:
        p("- none heard. If the mode was `on` that day and the wrist was raised, then a face does not get them: "
          "say so, and the face needs another way to wake.")
    gr = [e for e in events if e["event"] in ("gui_resume", "gui_suspend")]
    p(f"- GUI resumes: {sum(1 for e in gr if e['event']=='gui_resume')}, suspends: {sum(1 for e in gr if e['event']=='gui_suspend')}")
    if mins:
        full = sum(1 for r in mins if (num(r["gui_reports"]) or 0) >= 10)
        none = sum(1 for r in mins if (num(r["gui_reports"]) or 0) == 0)
        p(f"- of {len(mins)} minutes: {full} had 10 or more tick reports (ticked the whole minute), "
          f"{none} had none (not ticked at all). A face with the screen dark and no ticks shows as zeros here.")
        fps = [num(r["gui_fps_x10"]) for r in mins if num(r["gui_fps_x10"])]
        if fps:
            p(f"- frame rate when ticked: median {statistics.median(fps)/10:.1f} a second (the watch's is 10).")
    p("")

    # ---- 5: the cost of the wrist subscription ------------------------------------------------------
    p("## 5. What the wrist subscription costs")
    by = defaultdict(list)
    drain = defaultdict(list)
    for r in mins:
        a = num(r["avg_ma_x100"])
        if a is not None and num(r["charging"]) != 1:
            by[r["wrist_mode"]].append(a / 100.0)
    for mode in ("on", "off"):
        if by[mode]:
            p(f"- wrist {mode}: {len(by[mode])} minutes, mean discharge current {-statistics.mean(by[mode]):.3f} mA "
              f"(median {-statistics.median(by[mode]):.3f})")
    if by["on"] and by["off"]:
        diff = -statistics.mean(by["on"]) + statistics.mean(by["off"])
        p(f"- difference, on minus off: {diff:+.3f} mA. Compare with the watch's whole-day average (about 0.55 mA, "
          f"Energy NOTES); the days differ in use, so look at several.")
    else:
        p("- needs minutes with the gauge reading in both modes: leave it a few days, or pin a mode with mode.txt.")
    p("")

    # ---- 8: the history of activity ---------------------------------------------------------------
    p("## 8. The watch's own daily figures, for choosing none / low / high")
    if days:
        last_of = {}
        for r in days:
            last_of[r["date"]] = r
        p("| date | last hour | steps | active min | floors |")
        p("|---|---|---|---|---|")
        for k in sorted(last_of):
            r = last_of[k]
            p(f"| {k} | {r['hour']} | {r['steps'] or '?'} | {r['active_min'] or '?'} | {r['floors'] or '?'} |")
        p("")
        p("These are the last reading of each day (the day's total if the probe ran to its end). The family's "
          "training load per day is in the scan dump's `act_*` lines (the eighth number of each entry).")
    else:
        p("- no days.csv: the daily sensors did not connect, or the probe ran less than an hour.")
    p("")

    text = "\n".join(out)
    if md:
        open(md, "w", encoding="utf-8").write(text + "\n")
        print(f"wrote {md}")
    else:
        print(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
