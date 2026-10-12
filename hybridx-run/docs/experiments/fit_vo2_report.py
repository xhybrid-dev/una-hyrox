#!/usr/bin/env python3
"""Print the VO2max a HybridX Run FIT file carries, and check the file is sound.

Shows the session's developer fields (vo2max: this run; vo2max_recent: the
figure shown on the watch) and the standard max_met_data message, decoded the
way any FIT reader decodes them (fitdecode, with CRC checks), plus the other
session basics, so a file from the watch can be checked before it is
synced. As Race's fit_decode_report.py.

Usage:  python3 fit_vo2_report.py <file.fit> [more.fit ...]
Needs:  pip install fitdecode
"""
import sys

import fitdecode


def report(path):
    print(f"== {path}")
    session = None
    max_met = []
    dev_desc = {}
    with fitdecode.FitReader(path, check_crc=fitdecode.CrcCheck.RAISE) as fit:
        for frame in fit:
            if not isinstance(frame, fitdecode.FitDataMessage):
                continue
            if frame.name == "field_description":
                dev_desc[frame.get_value("field_definition_number")] = (
                    frame.get_value("field_name"), frame.get_value("units"))
            elif frame.name == "session":
                session = frame
            elif frame.name == "max_met_data":
                max_met.append(frame)

    print("  developer fields described:",
          ", ".join(f"{n}={name} ({units})" for n, (name, units) in sorted(dev_desc.items())))
    if session is None:
        print("  NO SESSION MESSAGE")
        return False
    print(f"  session: sport={session.get_value('sport')} "
          f"distance={session.get_value('total_distance')} m "
          f"timer={session.get_value('total_timer_time')} s "
          f"avg_hr={session.get_value('avg_heart_rate')}")
    for name in ("vo2max", "vo2max_recent"):
        if session.has_field(name):
            f = session.get_field(name)
            shown = f"{f.value:.1f}" if isinstance(f.value, float) else f.value
            print(f"  session.{name} = {shown} {f.units or ''}"
                  f"{'   (no figure: FIT invalid value)' if f.value is None else ''}")
        else:
            print(f"  session.{name}: MISSING")
    if not max_met:
        print("  max_met_data: none (no figure to show yet)")
    for m in max_met:
        print(f"  max_met_data: vo2_max={m.get_value('vo2_max')} mL/kg/min "
              f"sport={m.get_value('sport')} sub_sport={m.get_value('sub_sport')} "
              f"update_time={m.get_value('update_time')}")
    return True


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(2)
    ok = all([report(p) for p in sys.argv[1:]])
    sys.exit(0 if ok else 1)
