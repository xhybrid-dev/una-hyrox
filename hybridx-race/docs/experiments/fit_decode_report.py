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

# What each lap of a Full race with Roxzone off should be, in order.
SEGMENTS = [
    ("RUN 1", 0, 1, 0), ("SKIERG", 2, 1, 1),
    ("RUN 2", 0, 2, 0), ("SLED PUSH", 2, 2, 2),
    ("RUN 3", 0, 3, 0), ("SLED PULL", 2, 3, 3),
    ("RUN 4", 0, 4, 0), ("BURPEES", 2, 4, 4),
    ("RUN 5", 0, 5, 0), ("ROW", 2, 5, 5),
    ("RUN 6", 0, 6, 0), ("CARRY", 2, 6, 6),
    ("RUN 7", 0, 7, 0), ("LUNGES", 2, 7, 7),
    ("RUN 8", 0, 8, 0), ("WALL BALLS", 2, 8, 8),
]


def pace(speed_mps):
    """m/s -> m:ss per km, or None when there is no speed."""
    if not speed_mps:
        return None
    s = int(round(1000.0 / speed_mps))
    return f"{s // 60}:{s % 60:02d}"


def report(path):
    counts = collections.Counter()
    order, laps, events, steps = [], [], [], []
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

    print("\n  LAPS")
    print(f"    {'#':>2}  {'expected':11s} {'dist':>6} {'timer':>6} "
          f"{'pace/km':>8} {'step':>4} {'type':>4} {'rnd':>3} {'stn':>3}  ok")
    ok = True
    for i, lap in enumerate(laps):
        name, styp, rnd, sid = SEGMENTS[i] if i < len(SEGMENTS) else ("?", None, None, None)
        d = lap.get("total_distance") or 0
        good = (lap.get("segment_type") == styp and lap.get("round") == rnd
                and lap.get("station_id") == sid)
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
    print(f"  developer fields on every lap: {'yes' if ok else 'NO'}")
    print()
    return ok and ordered


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    if not all([report(p) for p in sys.argv[1:]]):
        sys.exit("one or more files did not decode as expected")
