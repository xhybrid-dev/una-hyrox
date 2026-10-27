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
#
# DISCARD=1 discards the run instead of saving it.
#
# TREADMILL=1 records a treadmill run (NOTES, "Treadmill"): it turns the main menu's
# switch on first, and after Save answers the distance screen: TREAD_TENTHS presses of
# L2 (default 2, +0.20), R1, TREAD_FINE presses of L2 (default 3, +0.03), R1. TREAD_SKIP=1
# presses R2 instead, keeping the estimate; TREAD_CLOSE=1 closes the app on that screen.
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

if [ "${TREADMILL:-0}" = 1 ]; then
    key 2                                 # L2: the Treadmill switch
    key 3                                 # R1: on
    snap 01a-treadmill-on
    key 1                                 # L1: back to Start
fi
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
if [ "${DISCARD:-0}" = 1 ]; then
    key 2; key 2; key 2                   # L2 three times: Discard
    hold 3 3                              # hold R1: discard
    sleep 3
    snap 05-discarded
    echo "--- discarded: files"
    find "$SOFT/Output" -name "*.fit" -printf "%p %s bytes\n"
    grep -E "Treadmill|discard" "$LOG" | sed 's/^ *//'
    exit 0
fi
key 2; key 2                              # L2 twice: Save
hold 3 3                                  # hold R1: save
sleep 3
if [ "${TREADMILL:-0}" = 1 ]; then
    snap 05a-distance                     # the treadmill's distance, the estimate offered
    if [ "${TREAD_CLOSE:-0}" = 1 ]; then
        kill -TERM $SIMPID                # the app closes here (SDL turns it into a quit)
        sleep 4                           # the estimate must be kept and the file closed
    elif [ "${TREAD_SKIP:-0}" = 1 ]; then
        key 4 2                           # R2: keep the estimate
    else
        for _ in $(seq "${TREAD_TENTHS:-2}"); do key 2 0.3; done
        snap 05b-distance-tenths
        key 3                             # R1: on to the hundredths
        for _ in $(seq "${TREAD_FINE:-3}"); do key 2 0.3; done
        snap 05c-distance-fine
        key 3 2                           # R1: save
    fi
fi
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
echo "--- treadmill (service log)"
grep -E "Treadmill|treadmill" "$LOG" | sed 's/^ *//'
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
