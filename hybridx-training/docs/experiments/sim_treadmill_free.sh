#!/bin/bash
#
# X Training in the PC simulator: a FREE run with the main menu's Treadmill switch on (X Run's treadmill
# run, kept in TP2), to know the path Today's treadmill session shares is sound. The switch is turned on,
# Free run is started with no GPS fix, the run goes for SECS seconds, is saved, and the console's distance
# is answered (TREAD_SKIP=1 keeps the estimate). The .fit is then checked by fit_treadmill_plan_report.py's
# treadmill checks (it has no workout, so only those it applies: sport, no positions, distance, link).
#
# Usage:  UNA_SDK=/path/to/una-sdk ./sim_treadmill_free.sh [seconds] [shots-dir]
set -u
SECS=${1:-60}
SHOTS=${2:-}
REPO=$(cd "$(dirname "$0")/../.." && pwd)
SOFT="$REPO/Software"
BIN="$SOFT/Apps/LVGL-GUI/simulator/build/bin"
DISP=:93
LOG=${SIM_LOG:-/tmp/x-training-free.log}
[ -x "$BIN/HybridXTrainingSimulator" ] || { echo "Build the simulator first ($BIN)" >&2; exit 1; }

rm -rf "$SOFT/Output" "$SOFT/SharedData"; mkdir -p "$SOFT/Output"
echo '{"schema":1,"values":{"plan":0,"fiveK":"22:00","fiveKRecent":true,"halfTime":"","raceDate":"","level":2,"runMon":true,"runTue":true,"runWed":true,"runThu":true,"runFri":true,"runSat":true,"runSun":true,"longRunDay":7,"easyByHr":false}}' > "$SOFT/Output/app_config.json"

Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
sleep 2
cd "$BIN"
DISPLAY=$DISP stdbuf -oL -eL ./HybridXTrainingSimulator > "$LOG" 2>&1 & SIMPID=$!
trap 'kill -9 $SIMPID 2>/dev/null; kill $XPID 2>/dev/null' EXIT
sleep 8
WID=$(DISPLAY=$DISP xdotool search --name 'X Training' | head -1)
[ -n "$WID" ] || { echo "FATAL: no simulator window" >&2; exit 1; }
key()  { DISPLAY=$DISP xdotool key "$1"; sleep "${2:-1}"; }
hold() { DISPLAY=$DISP xdotool keydown "$1"; sleep "$2"; DISPLAY=$DISP xdotool keyup "$1"; sleep 1; }
snap() { [ -n "$SHOTS" ] && mkdir -p "$SHOTS" && DISPLAY=$DISP timeout 10 import -window "$WID" "$SHOTS/$1.png" 2>/dev/null; return 0; }

key 2; key 2; key 3; snap f1-treadmill-on      # Today -> Free run -> the Treadmill switch, on
key 1                                          # back to Free run
snap f2-free-run
key 3 2                                        # R1: Start (no GPS fix to wait for)
snap f3-running
sleep "$SECS"
key 3 1                                        # R1: the action menu
key 2; key 2                                   # L2 twice: Save
hold 3 3
sleep 6
snap f4-distance
if [ "${TREAD_SKIP:-0}" = 1 ]; then
    key 4 2
else
    key 2 0.3; key 2 0.3; key 3; key 2 0.3; key 3 2
fi
snap f5-summary
sleep 2
echo "--- the service log"
grep -E "Treadmill|GPS|Track started|Planned" "$LOG" | sed 's/^ *//' | head -12
FIT=$(find "$SOFT/Output" -name "*.fit" | head -1)
ENTERED=$(grep -oE "Treadmill run saved: .* entered [0-9]+ m" "$LOG" | grep -oE "entered [0-9]+ m" | grep -oE "[0-9]+" | tail -1)
echo "--- the .fit (entered ${ENTERED:-0} m)"
[ -n "$FIT" ] && python3 "$REPO/docs/experiments/fit_treadmill_plan_report.py" "$FIT" --log "$LOG" --entered "${ENTERED:-0}" \
    --link "$SOFT/SharedData/HybridX/act_training.json" | grep -v "workout is in the file\|designated laps"
