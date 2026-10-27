#!/usr/bin/env python3
"""
Recompute the run detail page's numbers from a saved HybridX Run .fit and, with --log, compare
them with what the service worked out as the run went.

  python3 fit_detail_report.py <file.fit> [--log <simulator log>] [--zones 95,114,133,152,171,190]

Needs `pip install fitdecode`. From the file's records (one a second) it takes the speed and heart
rate, integrates the distance (sum of speed x 1 s), and computes, by brute force and in floating
point, with none of the service's bounded-memory tricks:

  - seconds in each heart-rate zone (the watch's thresholds, default 95,114,133,152,171,190:
    a heart rate above threshold i is in zone i + 1, so at or below the first is "below zone 1");
  - the fastest 1 km, 1 mile and 5 km (by interpolation over the distance series, so, unlike the
    service, a best may end between 10 m boundaries);
  - the pace of the first and second half of the run.

The service's distance is the GPS distance counter, not the integrated speed, and the two can
differ (a lot in the simulator, whose mock GPS speed and distance do not agree: its pace readout
and its distance total disagree by about 25%). So the integrated distance is scaled to equal the
session's total_distance, which IS the counter's, and the scale is printed. That assumes the counter
runs in proportion to speed over the run, which holds in the simulator and is only an approximation
on a watch. The comparison allows 3% on times and paces, equality to 2 s on zone seconds (they do
not depend on distance) and 2 s on a best.
Exit status 1 if a comparison fails.
"""
import re
import sys

import fitdecode

MILE_M = 1609.344
LENGTHS = [("1 km", 1000.0), ("1 mile", MILE_M), ("5 km", 5000.0)]


def session_distance(path):
    with fitdecode.FitReader(path, check_crc=fitdecode.CrcCheck.RAISE) as fit:
        for frame in fit:
            if isinstance(frame, fitdecode.FitDataMessage) and frame.name == "session":
                return frame.get_value("total_distance", fallback=None)
    return None


def records(path):
    rows = []
    with fitdecode.FitReader(path, check_crc=fitdecode.CrcCheck.RAISE) as fit:
        for frame in fit:
            if isinstance(frame, fitdecode.FitDataMessage) and frame.name == "record":
                rows.append({
                    "t": frame.get_value("timestamp", fallback=None),
                    "v": frame.get_value("enhanced_speed", fallback=None),
                    "hr": frame.get_value("heart_rate", fallback=None),
                })
    return [r for r in rows if r["t"] is not None]


def best_time(dist, times, length):
    """Fastest time to cover `length` metres: interpolate the time at distance d and d - length."""
    import bisect
    best = None
    for k in range(len(dist)):
        if dist[k] < length:
            continue
        target = dist[k] - length
        i = bisect.bisect_right(dist, target) - 1
        if i < 0 or i + 1 >= len(dist) or dist[i + 1] == dist[i]:
            continue
        f = (target - dist[i]) / (dist[i + 1] - dist[i])
        t0 = times[i] + f * (times[i + 1] - times[i])
        dt = times[k] - t0
        if dt > 0 and (best is None or dt < best):
            best = dt
    return best


def main(argv):
    path, log, zones = None, None, [95, 114, 133, 152, 171, 190]
    it = iter(argv[1:])
    for a in it:
        if a == "--log":
            log = next(it)
        elif a == "--zones":
            zones = [int(x) for x in next(it).split(",")]
        else:
            path = a
    if not path:
        print(__doc__)
        return 2

    rows = records(path)
    t0 = rows[0]["t"]
    times, dist, d = [], [], 0.0
    zone_s = [0] * 6
    for i, r in enumerate(rows):
        if i > 0:
            d += (r["v"] or 0.0) * (r["t"] - rows[i - 1]["t"]).total_seconds()
        times.append((r["t"] - t0).total_seconds())
        dist.append(d)
        hr = r["hr"]
        if hr is not None and 30 <= hr <= 230:
            z = -1
            for j in range(min(5, len(zones)) - 1, -1, -1):
                if hr > zones[j]:
                    z = j
                    break
            zone_s[z + 1] += 1

    total = times[-1]
    counter = session_distance(path)
    scale = 1.0
    if counter and d > 0:
        scale = counter / d
        dist = [x * scale for x in dist]
        d = dist[-1]
    print(f"{path}: {len(rows)} records, {total:.0f} s, {d:.0f} m (the session's total_distance; "
          f"integrated speed x {scale:.3f})")
    print("  zones below/1/2/3/4/5 (s):", "/".join(str(z) for z in zone_s))
    best = {}
    for name, length in LENGTHS:
        best[name] = best_time(dist, times, length)
        print(f"  best {name}: " + ("none" if best[name] is None else f"{best[name]:.1f} s"))
    # halves by active time (records are one a second: time is the index)
    half = total / 2.0
    import bisect
    i = bisect.bisect_left(times, half)
    d_half = dist[i]
    p1 = half / d_half if d_half > 0 else None
    p2 = (total - half) / (d - d_half) if d - d_half > 0 else None
    print("  half pace (s/km): " + (f"{p1 * 1000:.0f}" if p1 else "none") + " / " + (f"{p2 * 1000:.0f}" if p2 else "none"))

    if not log:
        return 0
    line = None
    for l in open(log, errors="replace"):
        if "Run detail:" in l:
            line = l      # the last one is the save
    if not line:
        print("  no 'Run detail:' line in the log")
        return 1
    m = re.search(r"zones (\d+)/(\d+)/(\d+)/(\d+)/(\d+)/(\d+) s; best (\d+)/(\d+)/(\d+) ms; half pace (\d+)/(\d+) s/km", line)
    if not m:
        print("  cannot read:", line.strip())
        return 1
    v = [int(x) for x in m.groups()]
    svc_zone, svc_best, svc_half = v[0:6], v[6:9], v[9:11]
    print("  service:   zones", "/".join(map(str, svc_zone)), "| best (s)",
          "/".join(f"{b / 1000:.1f}" if b else "none" for b in svc_best), "| half (s/km)", svc_half)
    fails = []
    for i_, (a, b) in enumerate(zip(svc_zone, zone_s)):
        if abs(a - b) > 2:
            fails.append(f"zone {i_}: service {a} s, file {b} s")
    for (name, _), s_ms in zip(LENGTHS, svc_best):
        b = best[name]
        if (s_ms == 0) != (b is None):
            fails.append(f"best {name}: one has it, the other does not")
        elif b is not None and abs(s_ms / 1000.0 - b) > max(0.03 * b, 2.0):
            fails.append(f"best {name}: service {s_ms / 1000:.1f} s, file {b:.1f} s")
    for k, (s_, f_) in enumerate(zip(svc_half, (p1, p2))):
        if f_ and abs(s_ - f_ * 1000) > 0.03 * f_ * 1000:
            fails.append(f"half {k + 1} pace: service {s_}, file {f_ * 1000:.0f} s/km")
    print("  RESULT:", "ok" if not fails else "FAILED: " + "; ".join(fails))
    return 0 if not fails else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
