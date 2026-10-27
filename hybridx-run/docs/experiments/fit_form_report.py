#!/usr/bin/env python3
"""
Check the cadence and step length in a HybridX Run .fit file.

  python3 fit_form_report.py <file.fit> [...]

Needs `pip install fitdecode`. For each file it prints the record series (how many
records carry cadence and step length), the lap and session averages, and checks
that every average equals what the records it covers say:

  - cadence: the record's `cadence` (strides per minute) plus `fractional_cadence`,
    times two, is steps per minute; the lap or session `avg_cadence` plus
    `avg_fractional_cadence`, times two, must be within 0.6 steps per minute of the
    mean over the records in it that have one;
  - step length: `avg_step_length` (mm) must be within 3 mm of the cadence-weighted
    mean of the records' `step_length` (the averages are distance over steps).

For a running session fitdecode names field 18 `avg_running_cadence` (a subfield the profile
selects by sport; same field number, strides per minute), so either name is accepted.

A lap or session with no cadence in its records must carry none (FIT's invalid value,
which fitdecode reads as None). Exit status 1 if any check fails.

The field names and numbers are Garmin's profile as fitdecode ships it (lap
avg_cadence 17, avg_fractional_cadence 80, avg_step_length 120; session 18, 92, 134).
"""
import sys

import fitdecode

CADENCE_TOL = 0.6   # steps per minute (the files hold strides + 1/128)
STEP_TOL_MM = 3.0


def either(row, *names):
    """The first of these fields the row has, with a value."""
    for n in names:
        if row.get(n) is not None:
            return row[n]
    return None


def steps_per_min(cadence, fractional):
    if cadence is None:
        return None
    return 2.0 * (cadence + (fractional or 0.0))


def check(label, avg_cad, avg_frac, avg_step, records, failures):
    """Compare one lap's or the session's averages with its records."""
    cads = []   # (steps per minute, step mm or None)
    for r in records:
        c = steps_per_min(r.get("cadence"), r.get("fractional_cadence"))
        if c is not None and c > 0:
            cads.append((c, r.get("step_length")))
    got_cad = steps_per_min(avg_cad, avg_frac)

    if not cads:
        ok = got_cad is None and avg_step is None
        print(f"  {label}: no cadence in its records; averages {'absent' if ok else 'PRESENT'}")
        if not ok:
            failures.append(f"{label}: averages present with no cadence in the records")
        return

    want_cad = sum(c for c, _ in cads) / len(cads)
    weighted = [(c, s) for c, s in cads if s is not None]
    want_step = sum(c * s for c, s in weighted) / sum(c for c, _ in weighted) if weighted else None

    cad_ok = got_cad is not None and abs(got_cad - want_cad) <= CADENCE_TOL
    if want_step is None:
        step_ok = avg_step is None
    else:
        step_ok = avg_step is not None and abs(avg_step - want_step) <= STEP_TOL_MM
    print(f"  {label}: cadence {got_cad if got_cad is None else round(got_cad, 1)} spm "
          f"(records {want_cad:.1f}) {'ok' if cad_ok else 'WRONG'}; step length "
          f"{avg_step if avg_step is None else round(avg_step)} mm "
          f"(records {'none' if want_step is None else round(want_step)}) {'ok' if step_ok else 'WRONG'}"
          f"  [{len(cads)} records]")
    if not cad_ok:
        failures.append(f"{label}: average cadence")
    if not step_ok:
        failures.append(f"{label}: average step length")


def report(path):
    failures = []
    records, laps, sessions = [], [], []
    with fitdecode.FitReader(path, check_crc=fitdecode.CrcCheck.RAISE) as fit:
        for frame in fit:
            if not isinstance(frame, fitdecode.FitDataMessage):
                continue
            names = {f.name for f in frame.fields}
            row = {n: frame.get_value(n, fallback=None) for n in names}
            if frame.name == "record":
                records.append(row)
            elif frame.name == "lap":
                laps.append(row)
            elif frame.name == "session":
                sessions.append(row)

    with_cad = sum(1 for r in records if r.get("cadence") is not None)
    with_step = sum(1 for r in records if r.get("step_length") is not None)
    print(f"{path}")
    print(f"  records {len(records)}; with cadence {with_cad}, with step length {with_step}")
    if records and with_cad and with_step:
        cs = [steps_per_min(r.get("cadence"), r.get("fractional_cadence")) for r in records if r.get("cadence") is not None]
        ss = [r["step_length"] for r in records if r.get("step_length") is not None]
        print(f"  record cadence {min(cs):.1f}..{max(cs):.1f} spm; step length {min(ss):.0f}..{max(ss):.0f} mm")

    for i, lap in enumerate(laps):
        start, end = lap.get("start_time"), lap.get("timestamp")
        # A lap ends at the second the next one starts, and the record written at that second
        # belongs to the lap that was running: so the first lap takes both ends, and the
        # later ones start just after their start_time.
        inside = [r for r in records if start is not None and end is not None
                  and (start <= r.get("timestamp") if i == 0 else start < r.get("timestamp"))
                  and r.get("timestamp") <= end]
        check(f"lap {i + 1}", either(lap, "avg_cadence", "avg_running_cadence"), lap.get("avg_fractional_cadence"),
              lap.get("avg_step_length"), inside, failures)
    for s in sessions:
        check("session", either(s, "avg_cadence", "avg_running_cadence"), s.get("avg_fractional_cadence"),
              s.get("avg_step_length"), records, failures)
    if not sessions:
        failures.append("no session message")
    print("  RESULT:", "ok" if not failures else "FAILED: " + "; ".join(failures))
    return not failures


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(2)
    sys.exit(0 if all([report(p) for p in sys.argv[1:]]) else 1)
