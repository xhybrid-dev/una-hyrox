#!/bin/bash
#
# The field-test amendments (T4) in the PC simulator, captured:
#   1. the wrong way: sim-reverse.gpx (the stadium track the other way round),
#      so the simulated runner runs the route backwards. The cap across the
#      top says "Wrong way", the map stays visible under it; the navigation
#      face says so too. Then discarded.
#   2. the finish: sim-3-laps.gpx, run on past the finish. "Route complete",
#      then the map's top lines and the navigation face show the run's time
#      and distance still counting; saved, and the FIT session checked to run
#      past the finish (the recording never stopped).
#
# Needs Xvfb, xdotool, ImageMagick and fitdecode. Keys 1..4 are L1, L2, R1, R2.
# Usage:  UNA_SDK=/path/to/una-sdk ./capture_t4.sh [output-dir]
set -u
REPO=$(cd "$(dirname "$0")/../.." && pwd)
SOFT="$REPO/Software"
BIN="$SOFT/Apps/LVGL-GUI/simulator/build/bin"
OUT=${1:-$REPO/docs/screens}
DISP=:95
LOG=/tmp/hybridx-trail-t4.log

[ -x "$BIN/HybridXTrailSimulator" ] || { echo "Build the simulator first ($BIN)" >&2; exit 1; }
mkdir -p "$OUT"

Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
SIMPID=""
cleanup() { [ -n "$SIMPID" ] && kill -9 $SIMPID 2>/dev/null; kill $XPID 2>/dev/null; }
trap cleanup EXIT
sleep 2

# A pretend watch with the simulator routes and $1 chosen, then the simulator.
start_sim() {
    [ -n "$SIMPID" ] && kill -9 $SIMPID 2>/dev/null && sleep 1
    rm -rf "$SOFT/Output"
    mkdir -p "$SOFT/Output/Routes"
    python3 "$REPO/Tools/TestRoutes/make_sim_routes.py" "$SOFT/Output/Routes" >/dev/null
    printf "%s" "$1" > "$SOFT/Output/route.sel"
    (cd "$BIN" && DISPLAY=$DISP stdbuf -oL -eL ./HybridXTrailSimulator > "$LOG" 2>&1) & SIMPID=$!
    sleep 8
    WID=$(DISPLAY=$DISP xdotool search --name 'HybridX Trail' | head -1)
    [ -n "$WID" ] || { echo "FATAL: no simulator window" >&2; exit 1; }
}
key()  { DISPLAY=$DISP xdotool key "$1"; sleep "${2:-1.5}"; }
hold() { DISPLAY=$DISP xdotool keydown "$1"; sleep "$2"; DISPLAY=$DISP xdotool keyup "$1"; sleep 1.5; }
waitlog() { for _ in $(seq "$2"); do grep -q "$1" "$LOG" && return 0; sleep 1; done; echo "timed out: $1" >&2; return 1; }
snap() {
    DISPLAY=$DISP import -window "$WID" "$OUT/$1.png" 2>/dev/null
    local w h
    read -r w h < <(identify -format "%w %h" "$OUT/$1.png")
    convert "$OUT/$1.png" \( -size "${w}x${h}" xc:black -fill white \
        -draw "circle $((w / 2)),$((h / 2)) $((w / 2)),0" \) \
        -alpha off -compose CopyOpacity -composite -background black -alpha remove -alpha off "$OUT/$1.png"
    echo "captured $1"
}

# -- 1. The wrong way -----------------------------------------------------------
start_sim sim-reverse.gpx
key 3 4                                            # R1: start the run
waitlog "Navigation: went wrong way" 90
sleep 2;          snap t4-01-wrong-way             # on the map, the cap over its top
key 4;            snap t4-02-nav-wrong-way         # R2: the navigation face
key 4                                              # back to the map
waitlog "Navigation: still wrong way" 90
echo "--- wrong way (service log)"
grep -a -E "Route:|Navigation:" "$LOG"
key 3 1; key 2; key 2; key 2; hold 3 3             # discard it

# -- 2. On past the finish ------------------------------------------------------------
start_sim sim-3-laps.gpx
key 3 4                                            # R1: start the run
waitlog "Navigation: finished" 400
sleep 1;          snap t4-03-route-complete        # the cap, green, 8 s
sleep 10;         snap t4-04-map-after-finish      # the run's time and distance, in place of "0 m to go"
sleep 10;         snap t4-05-map-after-finish-10s  # still counting
key 4;            snap t4-06-nav-after-finish      # R2: the navigation face
sleep 30
key 3 1; key 2; key 2; hold 3 3                    # pause, then save
sleep 3
echo "--- after the finish (service log)"
grep -a -E "Route:|Navigation:|Track (paused|stopped)" "$LOG"
FIT=$(find "$SOFT/Output" -name "*.fit" | head -1)
echo "--- $FIT"
python3 - "$FIT" "$LOG" <<'PY'
import re, sys, fitdecode
fit, log = sys.argv[1], sys.argv[2]
ms = {}
for line in open(log, errors="replace"):
    m = re.match(r"\s*(\d+) .*(Navigation: finished|Track paused)", line)
    if m and m.group(2) not in ms:
        ms[m.group(2)] = int(m.group(1))
after = (ms["Track paused"] - ms["Navigation: finished"]) / 1000.0
records = []
with fitdecode.FitReader(fit) as r:
    for frame in r:
        if isinstance(frame, fitdecode.FitDataMessage):
            if frame.name == "record":
                records.append(frame.get_value("timestamp"))
            elif frame.name == "session":
                timer = frame.get_value("total_timer_time")
                dist = frame.get_value("total_distance")
end = records[-1]
late = sum(1 for t in records if (end - t).total_seconds() < after - 1)
print("route finished %.0f s before the pause; session timer %.0f s, %.0f m; %d records, %d of them after the finish"
      % (after, timer, dist, len(records), late))
print("PASS: recorded every second past the finish" if late >= after - 4 and timer > after else "FAIL")
PY
grep -a -o "peak [0-9]*%" "$LOG" | sort -k2 -n | tail -1
