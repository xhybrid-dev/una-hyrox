#!/usr/bin/env python3
"""Write GPX routes around the SDK simulator's running track, for trying
HybridX Trail in the PC simulator.

The simulator's GPS (una-sdk Libs/Source/Simulator/Components/Simulator/
GpsStepCounterSimulator.cpp) runs laps of a 400 m stadium track whose left
curve is centred on 49.2331 N, 28.4682 E: a straight 84.39 m east along
y = +36.5 m, a semicircle clockwise round the east end, the straight back west
along y = -36.5 m, and the west semicircle. These routes are drawn on the
same track, so the simulated runner is really on (or off) them:

  sim-3-laps.gpx       three laps of the track, the way the runner goes:
                       stays on course and finishes after 1.2 km.
  sim-wrong-turn.gpx   the first straight, then carries straight on east for
                       300 m and back: the runner, lapping the track, leaves
                       the route at the first bend. The off-course alert fires.

    python3 make_sim_routes.py <output folder>
"""

import math
import os
import sys

CENTRE_LAT, CENTRE_LON = 49.2331, 28.4682
STRAIGHT, RADIUS = 84.39, 36.5
M_PER_DEG_LAT = 111320.0
M_PER_DEG_LON = M_PER_DEG_LAT * math.cos(math.radians(CENTRE_LAT))


def to_ll(x, y):
    return CENTRE_LAT + y / M_PER_DEG_LAT, CENTRE_LON + x / M_PER_DEG_LON


def lap(step=5.0):
    """One lap as (x, y) metres, the simulator's formulas, every ~step metres."""
    curve = math.pi * RADIUS
    length = 2 * STRAIGHT + 2 * curve
    pts = []
    d = 0.0
    while d < length:
        if d < STRAIGHT:
            x, y = d, RADIUS
        elif d < STRAIGHT + curve:
            a = (d - STRAIGHT) / RADIUS
            x, y = STRAIGHT + RADIUS * math.sin(a), RADIUS * math.cos(a)
        elif d < 2 * STRAIGHT + curve:
            x, y = STRAIGHT - (d - STRAIGHT - curve), -RADIUS
        else:
            a = (d - 2 * STRAIGHT - curve) / RADIUS
            x, y = -RADIUS * math.sin(a), -RADIUS * math.cos(a)
        pts.append((x, y))
        d += step
    return pts


def gpx(name, xy):
    out = ['<?xml version="1.0" encoding="UTF-8"?>',
           '<gpx version="1.1" creator="make_sim_routes.py" xmlns="http://www.topografix.com/GPX/1/1">',
           "<trk><name>%s</name><trkseg>" % name]
    for x, y in xy:
        lat, lon = to_ll(x, y)
        out.append('<trkpt lat="%.7f" lon="%.7f"><ele>250.0</ele></trkpt>' % (lat, lon))
    out.append("</trkseg></trk></gpx>")
    return "\n".join(out) + "\n"


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    one = lap()
    three = one * 3 + [one[0]]
    wrong = [(x, RADIUS) for x in range(0, 390, 5)] + [(x, RADIUS - 10) for x in range(385, -1, -5)]
    for name, title, pts in (("sim-3-laps.gpx", "Stadium 3 laps", three),
                             ("sim-wrong-turn.gpx", "Stadium wrong turn", wrong)):
        path = os.path.join(out, name)
        with open(path, "w") as f:
            f.write(gpx(title, pts))
        print("wrote", path)


if __name__ == "__main__":
    main()
