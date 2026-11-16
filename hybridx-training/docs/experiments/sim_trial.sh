#!/bin/bash
#
# X Training in the PC simulator: a 5K time trial, end to end (TP5). Today is the 5K time trial
# of a cut-back week (the long-run day of week 4); the run goes through the warm-up (a held R2
# starts the effort), the 5 km (the simulator's GPS runs about 5 m/s, so this takes about 17
# minutes), the cool-down, and Save. It then reports what the service did with the result:
# the trial's time, the 5K it makes, the decision (apply, ask or keep), plan.json's "ak" and
# 5K, and the Saved screen.
#
# Usage:  UNA_SDK=/path/to/una-sdk ./sim_trial.sh [shots-dir]
set -u
SHOTS=${1:-}
REPO=$(cd "$(dirname "$0")/../.." && pwd)
SOFT="$REPO/Software"
BIN="$SOFT/Apps/LVGL-GUI/simulator/build/bin"
DISP=:94
LOG=${SIM_LOG:-/tmp/x-training-trial.log}
[ -x "$BIN/HybridXTrainingSimulator" ] || { echo "Build the simulator first ($BIN)" >&2; exit 1; }

TODAY=$(date +%Y%m%d)
DOW=$(( $(date +%u) - 1 ))
DAYS=0
for k in 0 2 3 5; do DAYS=$(( DAYS | (1 << ((DOW + k) % 7)) )); done     # today is a run day, the long run
START=$(date -d "$(date +%F) -$DOW days -3 weeks" +%Y%m%d)                # week 4: the time trial
rm -rf "$SOFT/Output" "$SOFT/SharedData"; mkdir -p "$SOFT/Output"
bool() { [ $(( (DAYS >> $1) & 1 )) = 1 ] && echo true || echo false; }
cat > "$SOFT/Output/app_config.json" <<EOC
{"schema":1,"values":{"plan":1,"fiveK":"22:00","fiveKRecent":true,"halfTime":"","raceDate":"","level":2,
"runMon":$(bool 0),"runTue":$(bool 1),"runWed":$(bool 2),"runThu":$(bool 3),"runFri":$(bool 4),"runSat":$(bool 5),
"runSun":$(bool 6),"longRunDay":$(( DOW + 1 )),"easyByHr":false}}
EOC
printf '{"v":1,"s":[1,2,%d,%d,0,0,0,1320,%s,1,0,0],"c":[0,%s,0,10],"log":[],"st":%s}' \
    "$DAYS" "$DOW" "$START" "$START" "$TODAY" > "$SOFT/Output/plan.json"

Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
sleep 2
cd "$BIN"
DISPLAY=$DISP stdbuf -oL -eL ./HybridXTrainingSimulator > "$LOG" 2>&1 & SIMPID=$!
trap 'kill -9 $SIMPID 2>/dev/null; kill $XPID 2>/dev/null' EXIT
for _ in $(seq 60); do grep -q "Mock.Buzzer::play" "$LOG" && break; sleep 1; done
sleep 2
WID=$(DISPLAY=$DISP xdotool search --name 'X Training' | head -1)
key()  { DISPLAY=$DISP xdotool key "$1"; sleep "${2:-1}"; }
snap() { [ -n "$SHOTS" ] && mkdir -p "$SHOTS" && DISPLAY=$DISP timeout 10 import -window "$WID" "$SHOTS/$1.png" 2>/dev/null; return 0; }

grep -E "Today:" "$LOG" | tail -1 | sed 's/^ *//'
key 3 2; key 3 8                                    # Today; R1: the countdown, then the warm-up
sleep 3
DISPLAY=$DISP xdotool keydown 4; sleep 1.5; DISPLAY=$DISP xdotool keyup 4; sleep 2   # the held R2 starts the effort
snap trial-effort
for _ in $(seq 150); do grep -q "Trial effort" "$LOG" && break; sleep 10; done   # about 17 minutes
grep -E "Trial effort" "$LOG" | sed 's/^ *//'
sleep 5; snap trial-after
key 4 2                                             # R2: the cool-down's lap
sleep 3
key 3 1; key 2; key 2; snap trial-menu              # R1 the menu; L2 x2: Save & End (the workout is over, so no End workout row)
DISPLAY=$DISP xdotool keydown 3; sleep 3; DISPLAY=$DISP xdotool keyup 3; sleep 2
snap trial-saved
echo "--- after the save"
grep -E "Planned session|Trial|Plan settled|Plan action" "$LOG" | sed 's/^ *//'
echo "plan.json: $(grep -oE '"(s|ak)":[^]]*' "$SOFT/Output/plan.json" | tr '\n' ' ')"
sleep 5; snap trial-today; key 4 1; key 3 2; snap trial-today2
grep -E "Today:" "$LOG" | tail -1 | sed 's/^ *//'
