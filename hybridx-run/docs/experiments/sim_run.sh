#!/bin/bash
#
# Run HybridX Run in the PC simulator: start a run, let the simulated runner
# go for a while, then stop and save, and report what the service logged about
# VO2max and the files it wrote (vo2.json, ../SharedData/HybridX/vo2max.json).
#
# As Intervals' sim_run.sh: Xvfb for a display, xdotool for the buttons (keys
# 1..4 are L1, L2, R1, R2), ImageMagick's import for frames. Build the
# simulator first. The SDK's simulated runner and heart rate are noisy, so
# expect most windows to be rejected: this checks the plumbing, not the method
# (the host tests check the method).
#
# Usage:  UNA_SDK=/path/to/una-sdk ./sim_run.sh [seconds] [shots-dir] [app_config.json]
set -u
SECS=${1:-420}
SHOTS=${2:-}
CONFIG=${3:-}
REPO=$(cd "$(dirname "$0")/../.." && pwd)
SOFT="$REPO/Software"
BIN="$SOFT/Apps/LVGL-GUI/simulator/build/bin"
DISP=:96
LOG=${SIM_LOG:-/tmp/hybridx-run-sim.log}

[ -x "$BIN/HybridXRunSimulator" ] || { echo "Build the simulator first ($BIN)" >&2; exit 1; }

# A clean pretend watch, with the athlete's numbers if given.
rm -rf "$SOFT/Output"
mkdir -p "$SOFT/Output"
if [ -n "$CONFIG" ]; then
    cp "$CONFIG" "$SOFT/Output/app_config.json"
else
    printf '{"schema":1,"values":{"birthYear":1986,"birthMonth":5}}' > "$SOFT/Output/app_config.json"
fi

Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
sleep 2
cd "$BIN"
DISPLAY=$DISP stdbuf -oL -eL ./HybridXRunSimulator > "$LOG" 2>&1 & SIMPID=$!
cleanup() { kill -9 $SIMPID 2>/dev/null; kill $XPID 2>/dev/null; }
trap cleanup EXIT
sleep 8                                   # the simulated GPS finds a fix after ~5 s

WID=$(DISPLAY=$DISP xdotool search --name 'HybridX Run' | head -1)
[ -n "$WID" ] || { echo "FATAL: no simulator window" >&2; exit 1; }
key()  { DISPLAY=$DISP xdotool key "$1"; sleep "${2:-1}"; }
hold() { DISPLAY=$DISP xdotool keydown "$1"; sleep "$2"; DISPLAY=$DISP xdotool keyup "$1"; sleep 1; }
snap() { [ -n "$SHOTS" ] && mkdir -p "$SHOTS" && DISPLAY=$DISP timeout 10 import -window "$WID" "$SHOTS/$1.png" 2>/dev/null; return 0; }

snap 01-start
key 3 2                                   # R1: start
snap 02-running
sleep "$SECS"
snap 03-after
key 3 1                                   # R1: action menu (pauses)
snap 04-menu
key 2; key 2                              # L2 twice: Save
hold 3 3                                  # hold R1: save
sleep 3
snap 05-saved                             # the summary: L2 steps through its faces
key 2; snap 06-overview
key 2; snap 07-heart-rate
key 2; snap 08-vo2max

echo "--- VO2 (service log)"
grep -E "VO2|vo2|Resting HR|Out of memory" "$LOG" | sed 's/^ *//'
echo "--- files"
find "$SOFT/Output" "$SOFT/SharedData" -maxdepth 3 \( -name "*.fit" -o -name "vo2*.json" \) -printf "%p %s bytes\n" 2>/dev/null
for f in "$SOFT/Output/vo2.json" "$SOFT/SharedData/HybridX/vo2max.json"; do
    [ -f "$f" ] && { echo "$f:"; cat "$f"; echo; }
done
