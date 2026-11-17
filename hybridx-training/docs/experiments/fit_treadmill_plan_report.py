#!/usr/bin/env python3
"""X Training: check the .fit of a planned session run on a treadmill (NOTES "Treadmill sessions").

    python3 fit_treadmill_plan_report.py <file.fit> --log <simulator log> [--entered METRES] [--link act_training.json]

It decodes the file with fitdecode and checks what a treadmill session promises:
  - the session is running / treadmill, the records have a distance that never falls and no position;
  - the session's total_distance is the entry (or the run's own total when 0 is passed: skipped), its
    avg_speed is distance / timer time, and the developer field distance_estimate is the records' last
    distance (the run's total before the console's correction);
  - the workout is there, and every lap belongs to a step;
  - Jon's rule: for every step the service logged a belt speed for ("Treadmill step N: belt M mm/s" in
    the simulator log), the laps of that step cover distance = belt speed x time, within 3% over all of
    them (a lap is whole seconds); and for a step with no belt speed nothing is claimed;
  - with --link, the newest entry of act_training.json has the session's distance, and sport 1 (run).
Exits non-zero on a failed check. Needs fitdecode (pip install fitdecode).
"""
import argparse
import json
import re
import sys

import fitdecode


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("fit")
    ap.add_argument("--log", required=True)
    ap.add_argument("--entered", type=float, default=None)
    ap.add_argument("--link", default=None)
    a = ap.parse_args()

    belts = {}   # the last belt speed logged for a step (a repeated step logs the same each time)
    for line in open(a.log, errors="replace"):
        m = re.search(r"Treadmill step (\d+): belt (\d+) mm/s", line)
        if m:
            belts[int(m.group(1))] = int(m.group(2))

    session, records, laps, steps, dev_est, workout = None, [], [], [], None, None
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
            elif fr.name == "lap":
                laps.append(fr)
            elif fr.name == "workout_step":
                steps.append(fr)
            elif fr.name == "workout":
                workout = fr

    ok = True

    def check(name, passed, detail):
        nonlocal ok
        ok &= bool(passed)
        print(f"{'PASS' if passed else 'FAIL':5} {name:<30} {detail}")

    if session is None:
        print("FAIL  no session message")
        return 1
    sport, sub = session.get_value("sport"), session.get_value("sub_sport")
    check("sport / sub_sport", sport == "running" and sub == "treadmill", f"{sport} / {sub}")
    check("the workout is in the file", workout is not None and len(steps) > 0,
          f"{len(steps)} steps" if workout is not None else "none")

    dists = [rec.get_value("distance") for rec in records if rec.has_field("distance")]
    dists = [d for d in dists if d is not None]
    pos = sum(1 for rec in records if rec.has_field("position_lat") and rec.get_value("position_lat") is not None)
    check("records have a distance", len(dists) == len(records) and len(records) > 0,
          f"{len(dists)} of {len(records)} records")
    check("record distance never falls", all(b >= a_ for a_, b in zip(dists, dists[1:])),
          f"0 .. {dists[-1] if dists else None} m")
    check("no position", pos == 0, f"{pos} records with a position")

    total = session.get_value("total_distance")
    timer = session.get_value("total_timer_time")
    check("distance_estimate = records", dev_est is not None and dists and abs(dev_est - dists[-1]) <= 1.0,
          f"{dev_est} vs {dists[-1] if dists else None} m")
    if a.entered is not None:
        want = a.entered if a.entered > 0 else dev_est
        check("total_distance = entry", want is not None and abs(total - want) <= 0.5, f"{total} m vs {want} m")
    avg = session.get_value("enhanced_avg_speed") or session.get_value("avg_speed")
    check("avg_speed = distance / time", timer and abs(avg - total / timer) <= 0.002,
          f"{avg} vs {total / timer if timer else None} m/s")

    unlinked = [i + 1 for i, lap in enumerate(laps) if lap.get_value("wkt_step_index") is None]
    check("laps belong to steps", not unlinked or unlinked == [len(laps)], f"unlinked: {unlinked}")

    # The rule: designated steps run at their belt speed, so a lap's distance is that speed x its time.
    want_m, got_m, n = 0.0, 0.0, 0
    for lap in laps:
        idx = lap.get_value("wkt_step_index")
        t = lap.get_value("total_timer_time")
        d = lap.get_value("total_distance")
        if idx in belts and belts[idx] > 0 and t and d is not None:
            want_m += belts[idx] / 1000.0 * t
            got_m += d
            n += 1
            print(f"      lap on step {idx}: {t:.0f} s, {d:.1f} m, belt {belts[idx] / 1000.0 * 3.6:.1f} km/h "
                  f"-> {belts[idx] / 1000.0 * t:.1f} m")
    check("designated laps = belt x time", n > 0 and abs(got_m - want_m) <= 0.03 * want_m,
          f"{n} laps: {got_m:.0f} m vs {want_m:.0f} m")

    if a.link:
        doc = json.load(open(a.link))
        last = max(doc["a"], key=lambda e: e[0])
        check("activity link distance", abs(last[3] - total) <= 1 and last[6] == 1,
              f"{last[3]} m, sport {last[6]} (session {total} m)")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
