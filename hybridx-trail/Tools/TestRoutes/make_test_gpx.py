#!/usr/bin/env python3
"""Write two synthetic GPX files, for the simulator and for trying the probe.

These are made up, not real routes: shapes with known answers, in the two
styles real planners export. The host tests build the same shapes in C++
(Tests/Host/GpxReaderTest.cpp), and keep short_route.gpx as a fixture, so the
1 MB loop never needs committing.

  long_loop_track.gpx  a 5,000-point track (a point every ~2 m), GPX 1.1 with a
                       namespace prefix, times and Garmin-style extensions,
                       like a recorded run or a Komoot/Strava export. A
                       circle of radius 1,600 m: ~10.05 km, one climb of 80 m
                       and back down, as a sine over the loop.
  short_route.gpx      an 11-point <rtept> route, single quotes, CRLF line
                       ends, an entity in the name and a waypoint, like an
                       OS Maps-style planned route. A straight 1 km line due
                       north, flat.

    python3 make_test_gpx.py <output folder>
"""

import math
import os
import sys

# Arbitrary centre in open fell country; the numbers matter, the place doesn't.
LAT0, LON0 = 54.4500000, -3.0500000
R_EARTH = 6371000.0


def offset(lat, lon, north_m, east_m):
    dlat = north_m / R_EARTH * 180.0 / math.pi
    dlon = east_m / (R_EARTH * math.cos(math.radians(lat))) * 180.0 / math.pi
    return lat + dlat, lon + dlon


def long_loop():
    n = 5000
    radius = 1600.0
    out = ['<?xml version="1.0" encoding="UTF-8"?>',
           '<!-- synthetic test route: a 1.6 km-radius loop -->',
           '<gpx:gpx version="1.1" creator="make_test_gpx.py" '
           'xmlns:gpx="http://www.topografix.com/GPX/1/1" '
           'xmlns:gpxtpx="http://www.garmin.com/xmlschemas/TrackPointExtension/v1">',
           '<gpx:metadata><gpx:name>Metadata name wins</gpx:name></gpx:metadata>',
           '<gpx:trk><gpx:name>Not this one</gpx:name><gpx:trkseg>']
    for i in range(n + 1):
        a = 2 * math.pi * i / n
        lat, lon = offset(LAT0, LON0, radius * math.sin(a), radius * (1 - math.cos(a)))
        ele = 200.0 + 40.0 * (1 - math.cos(a))   # 200 m up to 280 m and back
        out.append('<gpx:trkpt lat="%.7f" lon="%.7f"><gpx:ele>%.1f</gpx:ele>'
                   '<gpx:time>2026-09-26T10:%02d:%02dZ</gpx:time><gpx:extensions>'
                   '<gpxtpx:TrackPointExtension><gpxtpx:hr>150</gpxtpx:hr>'
                   '</gpxtpx:TrackPointExtension></gpx:extensions></gpx:trkpt>'
                   % (lat, lon, ele, (i // 60) % 60, i % 60))
    out.append('</gpx:trkseg></gpx:trk></gpx:gpx>')
    return "\n".join(out) + "\n"


def short_route():
    out = ["<?xml version='1.0' encoding='UTF-8'?>",
           "<gpx version='1.1' creator='make_test_gpx.py' xmlns='http://www.topografix.com/GPX/1/1'>",
           "<wpt lat='54.46' lon='-3.05'><name>Summit cairn</name></wpt>",
           "<rte><name>Fell &amp; Back</name>"]
    for i in range(11):
        lat, lon = offset(LAT0, LON0, 100.0 * i, 0.0)
        out.append("  <rtept lat='%.7f' lon='%.7f'></rtept>" % (lat, lon))
    out.append("</rte></gpx>")
    return "\r\n".join(out) + "\r\n"


if len(sys.argv) != 2:
    sys.exit(__doc__)
OUT = sys.argv[1]
os.makedirs(OUT, exist_ok=True)
for name, body in (("long_loop_track.gpx", long_loop()), ("short_route.gpx", short_route())):
    path = os.path.join(OUT, name)
    with open(path, "w", newline="") as f:
        f.write(body)
    print("wrote", path, len(body), "bytes")
