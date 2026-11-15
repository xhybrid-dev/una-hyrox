#!/usr/bin/env python3
"""X Training: what a planned session's .fit file holds (TP2).

    python3 fit_plan_report.py <file.fit>

Prints the workout and its steps (with each speed target as a pace band), the
laps with the workout step each belongs to, the session, and the developer
fields. Then checks what X Training promises of the file: the workout is there
with its targets, every lap belongs to a step, the session is a run, and none of
X Run's VO2max fields are declared (the UNA app makes a card of every developer
field). Exits 1 if a check fails. Needs fitdecode (pip install fitdecode).
"""

import sys

import fitdecode


def pace(mmps):
    if not mmps:
        return "-"
    s = round(1e6 / mmps)
    return f"{s // 60}:{s % 60:02d}"


def main(path):
    workout, steps, laps, session, devfields = None, [], [], None, []
    with fitdecode.FitReader(path) as fit:
        for frame in fit:
            if not isinstance(frame, fitdecode.FitDataMessage):
                continue
            v = {f.name: f.value for f in frame.fields}
            if frame.name == "workout":
                workout = v
            elif frame.name == "workout_step":
                steps.append(v)
            elif frame.name == "lap":
                laps.append(v)
            elif frame.name == "session":
                session = v
            elif frame.name == "field_description":
                devfields.append(v.get("field_name"))

    problems = []
    print(f"workout: {workout.get('wkt_name') if workout else None!r}, "
          f"{workout.get('num_valid_steps') if workout else 0} steps, sport {workout.get('sport') if workout else None}")
    for s in steps:
        dur = s.get("duration_type")
        val = s.get("duration_value") if "duration_value" in s else s.get("duration_time") or s.get("duration_distance")
        tgt = s.get("target_type")
        band = ""
        if tgt == "speed":
            lo = s.get("custom_target_speed_low") or s.get("custom_target_value_low")
            hi = s.get("custom_target_speed_high") or s.get("custom_target_value_high")
            # fitdecode scales speed to m/s
            lo_mmps = lo * 1000 if isinstance(lo, float) else lo
            hi_mmps = hi * 1000 if isinstance(hi, float) else hi
            band = f" {pace(hi_mmps)}-{pace(lo_mmps)} /km"
        elif tgt == "heart_rate":
            band = f" zone {s.get('target_hr_zone') or s.get('target_value')}"
        print(f"  {s.get('message_index')}: {s.get('wkt_step_name')!r} {s.get('intensity')} {dur} {val} "
              f"target {tgt}{band}")
    if workout is None or not steps:
        problems.append("no workout in the file")
    if not any(s.get("target_type") == "speed" for s in steps):
        problems.append("no step has a speed target")

    print(f"laps: {len(laps)}")
    for i, lap in enumerate(laps):
        print(f"  {i + 1}: step {lap.get('wkt_step_index')}, {lap.get('total_timer_time')} s, "
              f"{lap.get('total_distance')} m, {lap.get('lap_trigger')}, {lap.get('intensity')}")
    unlinked = [i + 1 for i, lap in enumerate(laps) if lap.get("wkt_step_index") is None]
    if unlinked and unlinked != [len(laps)]:
        problems.append(f"laps not linked to a step: {unlinked}")

    if session is None:
        problems.append("no session")
    else:
        print(f"session: sport {session.get('sport')}, sub_sport {session.get('sub_sport')}, "
              f"{session.get('total_timer_time')} s, {session.get('total_distance')} m, "
              f"{session.get('num_laps')} laps")
        if session.get("sport") != "running":
            problems.append("the session is not a run")
    print(f"developer fields: {devfields}")
    if any(name and "vo2" in name.lower() for name in devfields):
        problems.append("X Run's VO2max developer fields are declared")

    if problems:
        print("PROBLEMS: " + "; ".join(problems))
        return 1
    print("OK: the workout with its targets, laps linked to steps, a running session, no VO2max fields")
    return 0


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(__doc__)
        sys.exit(2)
    sys.exit(main(sys.argv[1]))
