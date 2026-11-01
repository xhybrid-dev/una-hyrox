#!/bin/bash
#
# HybridX Run: re-scoring the kept runs (docs/NOTES.md, "VO2max switched on"). A pretend watch
# with a three-run history from before re-scoring (no windows kept, no profile key), two
# synthetic Trail runs to count (make_trail_run), and three starts of the simulator:
#   1. the Trail runs are counted and their files copied into Runs/; the first open of this build
#      scores everything once (the three old runs keep their figures: no windows), and the trend
#      is worked out under the 42-day limit;
#   2. the max HR entered on the phone changes: the two Trail runs are scored again with it;
#   3. nothing changed: no re-score.
# Prints the service's lines, vo2.json, the shared vo2max.json and Runs/ after each start.
#
# Usage:  UNA_SDK=/path/to/una-sdk ./sim_rescore.sh
# Build the simulator and `cmake --build hybridx-run/build-tests` first.
set -u
REPO=$(cd "$(dirname "$0")/../.." && pwd)
SOFT="$REPO/Software"
BIN="$SOFT/Apps/LVGL-GUI/simulator/build/bin"
MAKE="$REPO/build-tests/make_trail_run"
SHARED="$SOFT/SharedData/HybridX"
DISP=:97
LOG=${SIM_LOG:-/tmp/hybridx-run-rescore.log}

[ -x "$BIN/HybridXRunSimulator" ] || { echo "Build the simulator first ($BIN)" >&2; exit 1; }
[ -x "$MAKE" ] || { echo "Build the host tests first ($MAKE)" >&2; exit 1; }

rm -rf "$SOFT/Output" "$SHARED"
mkdir -p "$SOFT/Output" "$SHARED"
config() {   # config <max HR, 0 = from age>
    printf '{"schema":1,"values":{"birthYear":1986,"birthMonth":5,"restingHr":50,"maxHr":%s}}' "$1" \
        > "$SOFT/Output/app_config.json"
}
config 0
# Three runs from before re-scoring; the oldest is more than 42 days before the others.
printf '{"v":4,"restHr":0,"trail":0,"maxes":[],"runs":[[1786000000,560,22,0,560],[1790500000,512,22,0,536],[1790900000,524,30,0,529]]}' \
    > "$SOFT/Output/vo2.json"
"$MAKE" "$SHARED/trail_vo2_0.json" 1791200000 12 190 158 176
"$MAKE" "$SHARED/trail_vo2_1.json" 1791000000 12 200 160 178

start_sim() {   # start, wait for the service to load, stop
    rm -f "/tmp/.X${DISP#:}-lock"
    Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & local xpid=$!
    sleep 2
    ( cd "$BIN" && DISPLAY=$DISP stdbuf -oL -eL ./HybridXRunSimulator > "$LOG" 2>&1 & echo $! > /tmp/hybridx-run-sim.pid )
    sleep 6
    kill -9 "$(cat /tmp/hybridx-run-sim.pid)" 2>/dev/null; kill $xpid 2>/dev/null
    sleep 1
}
show() {
    echo "--- service log"
    grep -aE "Trail run|Re-score|VO2max history|VO2 profile" "$LOG" | sed 's/^ *//'
    echo "--- vo2.json"; cat "$SOFT/Output/vo2.json"; echo
    echo "--- shared vo2max.json"; cat "$SHARED/vo2max.json" 2>/dev/null; echo
    echo "--- Runs/"; ls "$SOFT/Output/Runs" 2>/dev/null | grep -v '\.bak$' | tr '\n' ' '; echo
}

echo "=== start 1: two Trail runs to count; the first open of this build"
start_sim; show
echo; echo "=== start 2: max HR 200 entered on the phone"
config 200
start_sim; show
echo; echo "=== start 3: nothing changed"
start_sim; show
