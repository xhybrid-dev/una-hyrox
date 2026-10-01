#!/usr/bin/env python3
"""Decode a HybridX Race FIT file and print everything a consumer app reads.

Written after the 22 September 2026 round trip, when Garmin Connect showed an
activity with no distance, no pace and no moving time and there was no way to
tell from the file whether the data was missing or merely unreadable. It was
missing. This report exists so that question is never open again: it prints what
IS in the file, field by field, and flags what is not.

Usage:  python3 fit_decode_report.py <file.fit> [more.fit ...]
Needs:  pip install fitdecode
"""
import collections
import sys

import fitdecode

STATIONS = [("SKIERG", 1000), ("SLED PUSH", 50), ("SLED PULL", 50),
            ("BURPEES", 80), ("ROW", 1000), ("CARRY", 200), ("LUNGES", 100),
            ("WALL BALLS", 0)]


def expected_segments(roxzone, run_m):
    """(name, segment_type, round, station_id, metres) for a Full race, in order.

    Roxzone off: RUN, STATION per round. On: RUN, ROX IN, STATION, ROX OUT, with
    no ROX OUT after the last station (brief 7.2). Types: 0 run, 1 rox in,
    2 station, 3 rox out.
    """
    out = []
    for r in range(1, 9):
        name, metres = STATIONS[r - 1]
        out.append((f"RUN {r}", 0, r, 0, run_m))
        if roxzone:
            out.append(("ROX IN", 1, r, 0, 0))
        out.append((name, 2, r, r, metres))
        if roxzone and r < 8:
            out.append(("ROX OUT", 3, r, 0, 0))
    return out


def pace(speed_mps):
    """m/s -> m:ss per km, or None when there is no speed."""
    if not speed_mps:
        return None
    s = int(round(1000.0 / speed_mps))
    return f"{s // 60}:{s % 60:02d}"


def consumer_checks(laps, records, session):
    """What Strava and Garmin compute from, as opposed to what we wrote.

    Both build lap pace, splits and moving time from the per-second distance,
    not from the lap totals, so the series has to agree with the totals.
    """
    print("\n  WHAT A CONSUMER APP COMPUTES FROM")
    good = True

    def check(label, passed, detail=""):
        nonlocal good
        good = good and passed
        print(f"    {label:52s} {'yes' if passed else 'NO '} {detail}")

    elapsed = sum(l.get("total_elapsed_time") or 0 for l in laps)
    check("laps sum to the session's elapsed time",
          abs(elapsed - session.get("total_elapsed_time")) < 0.5,
          f"({elapsed:.0f} s of {session.get('total_elapsed_time'):.0f} s)")

    starts_ok = all(
        abs((laps[i + 1]["start_time"] - laps[i]["timestamp"]).total_seconds()) < 0.5
        for i in range(len(laps) - 1))
    check("every lap starts where the last one ended", starts_ok)
    check("every lap is marked manual",
          all(l.get("lap_trigger") == "manual" for l in laps),
          f"({collections.Counter(l.get('lap_trigger') for l in laps)})")

    dist = [r.get("distance") for r in records]
    check("every record carries a distance", all(d is not None for d in dist))
    if not all(d is not None for d in dist):
        return False
    check("distance never goes backwards",
          all(b >= a for a, b in zip(dist, dist[1:])))
    check("last record equals the session's total distance",
          abs(dist[-1] - session.get("total_distance")) < 0.5,
          f"({dist[-1]:.0f} m of {session.get('total_distance'):.0f} m)")

    # The distance the records show inside each lap, against the lap's own total.
    # A lap's seconds are the records stamped from its start up to, but not
    # including, its end (the record AT the end is the next lap's first second),
    # so the lap's distance is the gain from one lap's last record to the next's.
    # A consumer that takes the record stamped exactly at the end instead is out
    # by one second of the next lap's ramp, at most a few metres.
    lap_ok = True
    worst = 0.0
    prev_end = 0.0
    for lap in laps:
        inside = [r["distance"] for r in records
                  if lap["start_time"] <= r["timestamp"] < lap["timestamp"]]
        if not inside:
            lap_ok = False
            break
        got = inside[-1] - prev_end
        worst = max(worst, abs(got - (lap.get("total_distance") or 0)))
        prev_end = inside[-1]
    check("each lap's distance in the records equals its total",
          lap_ok and worst < 0.5, f"(worst difference {worst:.1f} m)")

    # A jump of a kilometre in one second is a 1000 m/s speed to a consumer app.
    steps_m = [b - a for a, b in zip(dist, dist[1:])]
    fastest = max(steps_m) if steps_m else 0
    check("no second moves faster than 10 m/s (a sprint)", fastest <= 10.0,
          f"(fastest second {fastest:.1f} m)")

    # Moving seconds: the ones where distance advances. Strava excludes the rest.
    moving = sum(1 for m in steps_m if m > 0.05)
    print(f"    seconds in which distance advances: {moving} of {len(steps_m)}"
          f"  (the rest are zero-metre laps: Roxzone, Wall Balls)")
    return good


def report(path):
    counts = collections.Counter()
    order, laps, events, steps, records = [], [], [], [], []
    session = workout = None

    with fitdecode.FitReader(path) as fr:
        for frame in fr:
            if not isinstance(frame, fitdecode.FitDataMessage):
                continue
            counts[frame.name] += 1
            order.append(frame.name)
            fields = {f.name: f.value for f in frame.fields}
            if frame.name == "lap":
                laps.append(fields)
            elif frame.name == "record":
                records.append(fields)
            elif frame.name == "session":
                session = fields
            elif frame.name == "workout":
                workout = fields
            elif frame.name == "workout_step":
                steps.append(fields)
            elif frame.name == "event":
                events.append(fields)

    print(f"=== {path} ===")
    print("  messages:", ", ".join(f"{k}x{v}" for k, v in sorted(counts.items())))

    ev = [(e.get("event"), e.get("event_type")) for e in events]
    print(f"  timer events: {ev}"
          f"{'' if ('timer', 'stop') in ev else '   <-- NO STOP EVENT'}")

    if workout:
        named = sum(1 for st in steps if st.get("wkt_step_name"))
        print(f"  workout: {workout.get('wkt_name')!r}, "
              f"{workout.get('num_valid_steps')} steps, {len(steps)} written, "
              f"{named} named"
              f"{'' if named == len(steps) else '   <-- laps cannot be labelled'}")
        if named:
            print("    step names: "
                  + ", ".join(str(st.get("wkt_step_name")) for st in steps[:4]) + " ...")
    else:
        print("  workout: none")

    if not session:
        print("  SESSION MISSING")
        return False

    print("\n  SESSION")
    dist = session.get("total_distance")
    spd = session.get("avg_speed")
    for key, value, warn in (
        ("sport", session.get("sport"), False),
        ("sub_sport", session.get("sub_sport"), False),
        ("total_distance", f"{dist:.0f} m" if dist else None, not dist),
        ("avg_speed", f"{spd:.3f} m/s  ({pace(spd)} /km)" if spd else None, not spd),
        ("total_timer_time", f"{session.get('total_timer_time'):.0f} s", False),
        ("total_elapsed_time", f"{session.get('total_elapsed_time'):.0f} s", False),
        ("num_laps", session.get("num_laps"), False),
        ("avg/max heart rate", f"{session.get('avg_heart_rate')}"
                               f"/{session.get('max_heart_rate')} bpm", False),
        ("total_calories", session.get("total_calories"),
         session.get("total_calories") is None),
        ("race_format", session.get("race_format"), False),
        ("roxzone_mode", session.get("roxzone_mode"), False),
        ("run_distance_m", session.get("run_distance_m"),
         session.get("run_distance_m") is None),
        ("completed", session.get("completed"), False),
    ):
        flag = "   <-- absent, shows as '--'" if warn else ""
        print(f"    {key:20s} = {value}{flag}")

    expected = expected_segments(bool(session.get("roxzone_mode")),
                                 session.get("run_distance_m") or 1000)
    print("\n  LAPS")
    print(f"    {'#':>2}  {'expected':11s} {'dist':>6} {'timer':>6} "
          f"{'pace/km':>8} {'step':>4} {'type':>4} {'rnd':>3} {'stn':>3}  ok")
    ok = len(laps) == len(expected)
    for i, lap in enumerate(laps):
        name, styp, rnd, sid, metres = (expected[i] if i < len(expected)
                                        else ("?", None, None, None, None))
        d = lap.get("total_distance") or 0
        good = (lap.get("segment_type") == styp and lap.get("round") == rnd
                and lap.get("station_id") == sid and d == metres)
        ok = ok and good
        print(f"    {i:2d}  {name:11s} {d:6.0f} {lap.get('total_timer_time'):6.0f} "
              f"{str(pace(lap.get('avg_speed')) or '--'):>8} "
              f"{lap.get('wkt_step_index'):4} {lap.get('segment_type'):4} "
              f"{lap.get('round'):3} {lap.get('station_id'):3}  "
              f"{'yes' if good else 'NO'}")

    # Ordering: every lap after every record, and before the session.
    rec = [i for i, n in enumerate(order) if n == "record"]
    lap_i = [i for i, n in enumerate(order) if n == "lap"]
    ses = order.index("session")
    ordered = (not rec or not lap_i) or (max(rec) < min(lap_i) < ses)
    print(f"\n  laps batched after the records and before the session: "
          f"{'yes' if ordered else 'NO'}")
    print(f"  developer fields, distances and order on every lap: "
          f"{'yes' if ok else 'NO'}")

    consumer_ok = consumer_checks(laps, records, session)
    print()
    return ok and ordered and consumer_ok


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    if not all([report(p) for p in sys.argv[1:]]):
        sys.exit("one or more files did not decode as expected")
