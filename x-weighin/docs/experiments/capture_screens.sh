#!/bin/bash
#
# Capture every X Weigh-in screen in the PC simulator, and check that a weigh-in
# reaches both the log and its SharedData copy.
#
# As sim_run.sh: Xvfb for a display, xdotool for the buttons (keys 1-4 are
# UP, DOWN, SELECT, BACK), ImageMagick's import for frames. Build the simulator
# and the host tools (Tests/Host: make_log) first.
#
# Two runs:
#   1. A 40-day sample log ending today (make_log, seed 3) and the phone
#      settings (born May 1986, male): Home, Graph, Recent, Body, then a
#      weigh-in 0.3 kg up from the last one, Saved, and Home after.
#   2. No log at all: Home and the weigh-in screen as a new user sees them.
#
# The SDK's mock kernel leaves the UNA profile's height and weight at 0, so the
# Body page shows the height as not set and the empty run starts at 70.0.
#
# Usage:  UNA_SDK=/path/to/una-sdk ./capture_screens.sh [out-dir]
#   out-dir defaults to docs/screens (files w2-*.png).
set -u
REPO=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:-$REPO/docs/screens}
SOFT="$REPO/Software"
BIN="$SOFT/Apps/LVGL-GUI/simulator/build/bin"
MAKE_LOG="$REPO/build-tests/make_log"
DISP=:98
LOG=${SIM_LOG:-/tmp/x-weighin-capture.log}
OWN="$SOFT/Output/weighins.csv"
SHARED="$SOFT/SharedData/HybridX/weighins.csv"

[ -x "$BIN/XWeighInSimulator" ] || { echo "Build the simulator first ($BIN)" >&2; exit 1; }
[ -x "$MAKE_LOG" ] || { echo "Build the host tools first ($MAKE_LOG)" >&2; exit 1; }
mkdir -p "$OUT"

Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
SIMPID=""
cleanup() { [ -n "$SIMPID" ] && kill -9 $SIMPID 2>/dev/null; kill $XPID 2>/dev/null; }
trap cleanup EXIT
sleep 2

start_sim() {
    ( cd "$BIN" && exec env DISPLAY=$DISP stdbuf -oL -eL ./XWeighInSimulator >> "$LOG" 2>&1 ) & SIMPID=$!
    sleep 4
    WID=$(DISPLAY=$DISP xdotool search --name 'X Weigh-in' | tail -1)
    [ -n "$WID" ] || { echo "FATAL: no simulator window" >&2; exit 1; }
}
stop_sim() {
    kill -9 $SIMPID 2>/dev/null; wait $SIMPID 2>/dev/null; SIMPID=""
    # Wait for the window to go, so the next run's search cannot find it.
    for _ in 1 2 3 4 5 6 7 8 9 10; do
        DISPLAY=$DISP xdotool search --name 'X Weigh-in' >/dev/null 2>&1 || break
        sleep 0.5
    done
}
key()  { DISPLAY=$DISP xdotool key "$1"; sleep "${2:-1}"; }
snap() { DISPLAY=$DISP timeout 10 import -window "$WID" "$OUT/$1.png" 2>/dev/null; echo "  $1"; }

fresh_watch() {
    rm -rf "$SOFT/Output" "$SOFT/SharedData"
    mkdir -p "$SOFT/Output"
    printf '{"schema":1,"values":{"birthYear":1986,"birthMonth":5,"sex":2}}' > "$SOFT/Output/app_config.json"
}

: > "$LOG"
echo "Run 1: a 40-day log"
fresh_watch
"$MAKE_LOG" "$OWN" 40 3 "$(date +%F)" 2>/dev/null
start_sim
snap w2-1-home
key 2; snap w2-2-graph
key 2; snap w2-3-recent
key 2; snap w2-4-body
key 2                                       # DOWN again: round to Home
key 3; snap w2-5-weighin                    # SELECT: weigh in
key 1 0.3; key 1 0.3; key 1 1; snap w2-6-weighin-up
before=$(tail -n 1 "$OWN")
key 3 1.5; snap w2-7-saved                  # SELECT: save
sleep 3.5; snap w2-8-home-after             # Saved returns to Home by itself
stop_sim

echo "Check: the weigh-in reached both files"
after=$(tail -n 1 "$OWN")
echo "  last line before: $before"
echo "  last line after:  $after"
if [ -f "$SHARED" ] && cmp -s "$OWN" "$SHARED"; then
    echo "  SharedData copy: identical ($(wc -l < "$SHARED") lines)"
else
    echo "  SharedData copy: MISSING or DIFFERENT"
fi

echo "Run 2: no log"
fresh_watch
start_sim
snap w2-9-home-empty
key 3; snap w2-10-weighin-empty
key 4; stop_sim

echo "=== service log ==="
grep -E "log:|weigh-in|settings|profile|LVGL pool" "$LOG" | sed 's/^ *[0-9]* //; s/^/  /'
