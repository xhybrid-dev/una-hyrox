#!/usr/bin/env python3
"""Print every field a consumer might build an activity's title from.

Usage:  python3 -I fit_naming_report.py <file.fit> [more.fit ...]

For each file: file_id.product_name, session sport / sub_sport /
sport_profile_name, and the workout's name. This is the "what did we put in the
file" half of the naming experiment (NOTES.md 5.28); what Strava and Garmin
Connect then SHOW is Jon's half.
"""
import sys
import fitdecode


def show(path):
    product = profile = workout = None
    sport = sub = None
    with fitdecode.FitReader(path) as fit:
        for frame in fit:
            if not isinstance(frame, fitdecode.FitDataMessage):
                continue
            if frame.name == "file_id" and frame.has_field("product_name"):
                product = frame.get_value("product_name")
            elif frame.name == "session":
                sport = frame.get_value("sport", fallback=None)
                sub = frame.get_value("sub_sport", fallback=None)
                profile = frame.get_value("sport_profile_name", fallback=None)
            elif frame.name == "workout":
                workout = frame.get_value("wkt_name", fallback=None)
    print(path)
    print(f"  file_id.product_name      = {product!r}")
    print(f"  session.sport / sub_sport = {sport} / {sub}")
    print(f"  session.sport_profile_name= {profile!r}")
    print(f"  workout name              = {workout!r}")


if __name__ == "__main__":
    for p in sys.argv[1:]:
        show(p)
