#!/bin/bash
#
# Run X Weigh-in in the PC simulator and capture its screen: once straight
# away, and once after the daily heart-rate metrics have reported (the
# simulated sensor reports one period after connecting: 60 s).
#
# As HybridX Run's sim_run.sh: Xvfb for a display, xdotool for the buttons
# (keys 1..4 are L1, L2, R1, R2), ImageMagick's import for frames. Build the
# simulator first. The SDK's mock kernel leaves the profile's height and
# weight at 0, so the simulator always shows them as not set; the watch is
# where they are checked (Gate W0).
#
# Usage:  UNA_SDK=/path/to/una-sdk ./sim_run.sh [shots-dir] [app_config.json]
#   With no app_config.json there are no phone settings (the not-set state).
#   WAIT=seconds overrides the 65 s wait for the resting HR.
set -u
SHOTS=${1:-}
CONFIG=${2:-}
WAIT=${WAIT:-65}
REPO=$(cd "$(dirname "$0")/../.." && pwd)
SOFT="$REPO/Software"
BIN="$SOFT/Apps/LVGL-GUI/simulator/build/bin"
DISP=:97
LOG=${SIM_LOG:-/tmp/x-weighin-sim.log}

[ -x "$BIN/XWeighInSimulator" ] || { echo "Build the simulator first ($BIN)" >&2; exit 1; }

# A clean pretend watch, with the phone settings if given.
rm -rf "$SOFT/Output"
mkdir -p "$SOFT/Output"
if [ -n "$CONFIG" ]; then
    cp "$CONFIG" "$SOFT/Output/app_config.json"
fi

Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
sleep 2
cd "$BIN"
DISPLAY=$DISP stdbuf -oL -eL ./XWeighInSimulator > "$LOG" 2>&1 & SIMPID=$!
cleanup() { kill -9 $SIMPID 2>/dev/null; kill $XPID 2>/dev/null; }
trap cleanup EXIT
sleep 4

WID=$(DISPLAY=$DISP xdotool search --name 'X Weigh-in' | head -1)
[ -n "$WID" ] || { echo "FATAL: no simulator window" >&2; exit 1; }
snap() { [ -n "$SHOTS" ] && mkdir -p "$SHOTS" && DISPLAY=$DISP timeout 10 import -window "$WID" "$SHOTS/$1.png" 2>/dev/null; return 0; }

snap 01-open
sleep "$WAIT"
snap 02-resting-hr
DISPLAY=$DISP xdotool key 4; sleep 2      # R2: exit

echo "=== service log ==="
grep -E "settings|profile|Resting HR|clock|GUI" "$LOG" | sed 's/^/  /'
if kill -0 $SIMPID 2>/dev/null; then
    echo "R2 did not close the app"
else
    echo "R2 closed the app"
fi
