#!/usr/bin/env python3
"""Check a HybridX Run treadmill activity file (docs/NOTES.md, "Treadmill").

Decodes the .fit with fitdecode and checks:
  - the session is running / treadmill (fitdecode names the values from Garmin's profile);
  - the records carry a distance, never decreasing, and no position;
  - the developer field distance_estimate is the records' last distance (the estimate
    from cadence, before the athlete's correction);
  - the session's total_distance is the corrected one: with --entered M, that it is M
    (or the estimate when the entry was skipped or rejected: pass --entered 0);
  - the session's avg_speed is total_distance / total_timer_time.
With --link act_run.json, also that the newest entry there has the session's distance.

Usage:  fit_treadmill_report.py <file.fit> [--entered METRES] [--link act_run.json]
Exits non-zero on a failed check.
"""
import argparse
import json
import sys

import fitdecode


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("fit")
    ap.add_argument("--entered", type=float, default=None)
    ap.add_argument("--link", default=None)
    a = ap.parse_args()

    session, records, dev_est = None, [], None
    with fitdecode.FitReader(a.fit) as r:
        for fr in r:
            if not isinstance(fr, fitdecode.FitDataMessage):
                continue
            if fr.name == "session":
                session = fr
                for f in fr.fields:
                    if f.name == "distance_estimate":
                        dev_est = f.value
            elif fr.name == "record":
                records.append(fr)

    ok = True

    def check(name, passed, detail):
        nonlocal ok
        ok &= bool(passed)
        print(f"{'PASS' if passed else 'FAIL':5} {name:<28} {detail}")

    if session is None:
        print("FAIL  no session message")
        return 1
    sport, sub = session.get_value("sport"), session.get_value("sub_sport")
    check("sport / sub_sport", sport == "running" and sub == "treadmill", f"{sport} / {sub}")

    dists = [rec.get_value("distance") for rec in records if rec.has_field("distance")]
    dists = [d for d in dists if d is not None]
    pos = sum(1 for rec in records if rec.has_field("position_lat") and rec.get_value("position_lat") is not None)
    check("records have a distance", len(dists) == len(records) and len(records) > 0,
          f"{len(dists)} of {len(records)} records")
    check("record distance never falls", all(b >= a for a, b in zip(dists, dists[1:])),
          f"0 .. {dists[-1] if dists else None} m")
    check("no position", pos == 0, f"{pos} records with a position")

    total = session.get_value("total_distance")
    timer = session.get_value("total_timer_time")
    check("distance_estimate = records", dev_est is not None and dists and abs(dev_est - dists[-1]) <= 1.0,
          f"{dev_est} vs {dists[-1] if dists else None} m")
    if a.entered is not None:
        want = a.entered if a.entered > 0 else dev_est
        check("total_distance = entry", want is not None and abs(total - want) <= 0.5,
              f"{total} m vs {want} m")
    avg = session.get_value("enhanced_avg_speed") or session.get_value("avg_speed")
    check("avg_speed = distance / time", timer and abs(avg - total / timer) <= 0.002,
          f"{avg} vs {total / timer if timer else None} m/s")

    if a.link:
        doc = json.load(open(a.link))
        last = max(doc["a"], key=lambda e: e[0])
        check("activity link distance", abs(last[3] - total) <= 1 and last[6] == 1,
              f"{last[3]} m, sport {last[6]} (session {total} m)")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
