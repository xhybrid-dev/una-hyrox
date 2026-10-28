#!/bin/bash
#
# Walk HybridX Run's VO2max screens in the PC simulator (R5) and photograph
# each: the VO2max page (overview and the run list), the profile menu, the
# three pickers, a save, and the page again after it. A seeded seven-run
# history gives the list two pages. Then prints app_config.json, which the
# saves rewrite, and the LVGL pool lines.
#
# As sim_run.sh: Xvfb, xdotool (keys 1..4 are L1, L2, R1, R2), ImageMagick's
# import. Build the simulator first.
#
# Usage:  UNA_SDK=/path/to/una-sdk ./capture_vo2.sh [out-dir]   (default docs/screens)
set -u
REPO=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:-$REPO/docs/screens}
SOFT="$REPO/Software"
BIN="$SOFT/Apps/LVGL-GUI/simulator/build/bin"
DISP=:97
LOG=${SIM_LOG:-/tmp/hybridx-run-vo2-capture.log}

[ -x "$BIN/HybridXRunSimulator" ] || { echo "Build the simulator first ($BIN)" >&2; exit 1; }
mkdir -p "$OUT"

rm -rf "$SOFT/Output"
mkdir -p "$SOFT/Output"
printf '{"schema":1,"values":{"birthYear":1986,"birthMonth":5,"maxHr":0,"restingHr":0,"tuningLog":true}}' \
    > "$SOFT/Output/app_config.json"
printf '{"v":2,"restHr":52,"maxes":[],"runs":[[1790200000,508,22],[1790500000,512,22],[1790900000,524,30],[1791100000,519,25],[1791300000,531,18],[1791500000,527,28],[1791700000,535,30]]}' \
    > "$SOFT/Output/vo2.json"
# Personal bests (NOTES, "Personal bests"): a 1 km and a 1 mile, no 5 km yet.
printf '{"v":1,"b":[[221000,1791500000],[368000,1791700000],[0,0]]}' > "$SOFT/Output/bests.json"

Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
sleep 2
cd "$BIN"
DISPLAY=$DISP stdbuf -oL -eL ./HybridXRunSimulator > "$LOG" 2>&1 & SIMPID=$!
cleanup() { kill -9 $SIMPID 2>/dev/null; kill $XPID 2>/dev/null; }
trap cleanup EXIT
sleep 6

WID=$(DISPLAY=$DISP xdotool search --name 'HybridX Run' | head -1)
[ -n "$WID" ] || { echo "FATAL: no simulator window" >&2; exit 1; }
key()  { DISPLAY=$DISP xdotool key "$1"; sleep "${2:-1}"; }
snap() { DISPLAY=$DISP timeout 10 import -window "$WID" "$OUT/$1.png" 2>/dev/null; return 0; }

key 2; key 2; key 2; key 2   # wheel: Start -> Treadmill -> Workouts -> Intervals -> VO2max
snap vo2-01-start-menu
key 3                        # R1: the VO2max page
snap vo2-02-overview
key 2; snap vo2-02b-bests     # L2: the personal bests
key 2; snap vo2-03-runs-1    # L2: the run list
key 2; snap vo2-04-runs-2
key 1; key 1; key 1          # back to the overview
key 3                        # R1: the profile menu
snap vo2-05-profile-menu
key 3                        # R1: Birth date
snap vo2-06-birth-month
key 2                        # L2: month + 1
key 3                        # R1: on to the year
key 1                        # L1: year - 1
snap vo2-07-birth-year
key 3                        # R1: save
key 2                        # L2: Max HR
key 3                        # R1: its picker (auto)
snap vo2-08-max-hr-auto
key 2; key 2; key 2; key 2   # L2: from auto to the value in use, then +3
snap vo2-09-max-hr-set
key 3                        # R1: save
key 2                        # L2: Resting HR
key 3                        # R1: its picker
snap vo2-10-rest-hr
key 4                        # R2: back without saving
key 2                        # L2: Tuning log
snap vo2-11-tuning-log-on
key 3                        # R1: toggle
snap vo2-12-tuning-log-off
key 4                        # R2: the VO2max page again
snap vo2-13-overview-after
key 4                        # R2: the start screen

echo "--- app_config.json after the saves"
cat "$SOFT/Output/app_config.json"; echo
echo "--- service log"
grep -E "VO2 profile set|Could not|Out of memory" "$LOG" | sed 's/^ *[0-9]* //'
echo "--- LVGL pool (peak)"
grep -E "LVGL pool" "$LOG" | sed 's/.*peak \([0-9]*\)%.*/\1/' | sort -n | tail -1
ls "$OUT"/vo2-*.png | wc -l
