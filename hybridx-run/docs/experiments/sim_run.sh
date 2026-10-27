#!/bin/bash
#
# Run HybridX Run in the PC simulator: start a run, let the simulated runner
# go for a while, then stop and save, and report what the service logged about
# VO2max and the files it wrote (vo2.json, ../SharedData/HybridX/vo2max.json and
# act_run.json, the summary HybridX Streak reads; it keeps the last 16 runs, and
# this script does not clear SharedData, so a second run adds to it).
#
# As Intervals' sim_run.sh: Xvfb for a display, xdotool for the buttons (keys
# 1..4 are L1, L2, R1, R2), ImageMagick's import for frames. Build the
# simulator first. The SDK's simulated runner and heart rate are noisy, so
# expect most windows to be rejected: this checks the plumbing, not the method
# (the host tests check the method).
#
# Usage:  UNA_SDK=/path/to/una-sdk ./sim_run.sh [seconds] [shots-dir] [app_config.json]
#   SEED=1 first writes a three-run history, so the screens have a recent figure.
# The simulator has a cadence mock (ConfigurationSimulator.hpp), so the form faces show 170 spm.
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
if [ "${SEED:-0}" = 1 ]; then
    printf '{"v":1,"autoMaxHr":0,"runs":[[1790500000,512,22],[1790900000,524,30],[1791300000,531,18]]}' \
        > "$SOFT/Output/vo2.json"
fi

Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
sleep 2
cd "$BIN"
DISPLAY=$DISP stdbuf -oL -eL ./HybridXRunSimulator > "$LOG" 2>&1 & SIMPID=$!
cleanup() { kill -9 $SIMPID 2>/dev/null; kill $XPID 2>/dev/null; }
trap cleanup EXIT
# Wait for the simulated GPS fix (RunLVGL's first-fix buzz is logged): a
# Start pressed before it opens the "no GPS" confirmation instead.
for _ in $(seq 60); do
    grep -q "Mock.Buzzer::play" "$LOG" && break
    sleep 1
done
sleep 2

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
key 2; snap 03b-lap                       # L2: the lap face
key 2; snap 03c-form                      # L2: the form face (cadence, step length)
key 2; snap 03d-status                    # L2: the status face
key 1; key 1; key 1                       # L1 back round to the totals face
key 3 1                                   # R1: action menu (pauses)
snap 04-menu
key 2; key 2                              # L2 twice: Save
hold 3 3                                  # hold R1: save
sleep 3
snap 05-saved                             # the summary: L2 steps through its faces
key 2; snap 06-overview
key 2; snap 07-heart-rate
key 2; snap 08-vo2max
key 2; snap 09-form                       # the run's average cadence and step length
key 2; snap 10-zones                      # time in the watch's heart-rate zones
key 2; snap 11-splits                     # best 1 km / 1 mi / 5 km, and the two halves
key 2; snap 12-pacing                     # uphill-adjusted pace and drift (a long steady run only)

echo "--- LVGL pool (every screen switch; the summary builds one face at a time)"
grep -E "LVGL pool" "$LOG" | sed 's/^ *//' | sort -t'(' -k2 | awk '{print}' | tail -n 14
echo "--- form (service log)"
grep -E "Form|cadence|Cadence" "$LOG" | sed 's/^ *//' | tail -n 5
echo "--- VO2 (service log)"
grep -E "VO2|vo2|Resting HR|Out of memory" "$LOG" | sed 's/^ *//'
echo "--- files"
find "$SOFT/Output" "$SOFT/SharedData" -maxdepth 3 \( -name "*.fit" -o -name "vo2*.json" -o -name "act_*.json" \) -printf "%p %s bytes\n" 2>/dev/null
for f in "$SOFT/Output/vo2.json" "$SOFT/SharedData/HybridX/vo2max.json" "$SOFT/SharedData/HybridX/act_run.json"; do
    [ -f "$f" ] && { echo "$f:"; cat "$f"; echo; }
done
echo "--- tuning log"
for f in "$SOFT"/Output/Logs/vo2_*.csv; do
    [ -f "$f" ] && { echo "$f:"; cat "$f"; }
done
echo "--- app_config.json"
cat "$SOFT/Output/app_config.json" 2>/dev/null; echo
