#!/bin/bash
#
# HybridX Run, L2: does the app count the runs HybridX Trail left in the shared
# folder? A pretend watch with a three-run history and the athlete's numbers,
# three synthetic Trail files (make_trail_run, from the host tests' build), and
# three starts of the simulator:
#   1. two new runs, one junk file: both runs are counted, in time order;
#   2. the same files: nothing is counted again;
#   3. a newer run with too few steady minutes: the mark moves, no estimate.
# Prints the service's Trail lines and vo2.json after each start.
#
# Usage:  UNA_SDK=/path/to/una-sdk ./sim_trail_ingest.sh
# Build the simulator and `cmake --build hybridx-run/build-tests` first.
set -u
REPO=$(cd "$(dirname "$0")/../.." && pwd)
SOFT="$REPO/Software"
BIN="$SOFT/Apps/LVGL-GUI/simulator/build/bin"
MAKE="$REPO/build-tests/make_trail_run"
SHARED="$SOFT/SharedData/HybridX"
DISP=:97
LOG=${SIM_LOG:-/tmp/hybridx-run-trail-sim.log}

[ -x "$BIN/HybridXRunSimulator" ] || { echo "Build the simulator first ($BIN)" >&2; exit 1; }
[ -x "$MAKE" ] || { echo "Build the host tests first ($MAKE)" >&2; exit 1; }

rm -rf "$SOFT/Output" "$SHARED"
mkdir -p "$SOFT/Output" "$SHARED"
# Resting HR entered (the simulator's own watch figure is its running floor).
printf '{"schema":1,"values":{"birthYear":1986,"birthMonth":5,"restingHr":50}}' > "$SOFT/Output/app_config.json"
printf '{"v":1,"autoMaxHr":0,"runs":[[1790500000,512,22],[1790900000,524,30],[1791300000,531,18]]}' > "$SOFT/Output/vo2.json"

start_sim() {   # start, wait for the service to load, stop
    Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & local xpid=$!
    sleep 2
    ( cd "$BIN" && DISPLAY=$DISP stdbuf -oL -eL ./HybridXRunSimulator > "$LOG" 2>&1 & echo $! > /tmp/hybridx-run-sim.pid )
    sleep 6
    kill -9 "$(cat /tmp/hybridx-run-sim.pid)" 2>/dev/null; kill $xpid 2>/dev/null
    sleep 1
}
show() {
    echo "--- service log"
    grep -aE "Trail|VO2max history|VO2 profile" "$LOG" | sed 's/^ *//'
    echo "--- vo2.json"; cat "$SOFT/Output/vo2.json"; echo
    echo "--- shared vo2max.json"; cat "$SOFT/SharedData/HybridX/vo2max.json" 2>/dev/null; echo
}

echo "=== start 1: Trail runs 1791000000 (12 windows) and 1791200000 (8), one junk file"
"$MAKE" "$SHARED/trail_vo2_0.json" 1791200000 8 190 158 176
"$MAKE" "$SHARED/trail_vo2_1.json" 1791000000 12 200 160 178
printf 'not a window file' > "$SHARED/trail_vo2_2.json"
start_sim; show

echo; echo "=== start 2: the same files again"
start_sim; show

echo; echo "=== start 3: a newer run with 3 windows (too few)"
"$MAKE" "$SHARED/trail_vo2_2.json" 1791500000 3 200 160
start_sim; show
