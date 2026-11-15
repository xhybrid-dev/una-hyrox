#!/bin/bash
#
# X Training in the PC simulator: today's planned session, end to end (TP2).
#
# Makes a pretend watch on which today is a quality day of a plan already under
# way, opens the app, checks that Start offers today's session, runs it through
# the SDK's workout engine (stepping through the steps with the lap button,
# R2, as a runner may), saves it, and reports what the service logged, the
# plan file, and the FIT file decoded (the workout and its targets, a lap per
# step, the session).
#
# The pretend watch: app_config.json for the 10K plan at level 2 with a 22:00
# 5K, on four days placed so that today is the first quality day; and a
# plan.json whose calendar started WEEK weeks ago, so today is in week WEEK+1
# (default 4: week 5, "INT 5 x 3"). The service keeps that plan because the
# phone's settings match it.
#
# As X Run's sim_run.sh: Xvfb for a display, xdotool for the buttons (keys 1..4
# are L1, L2, R1, R2), ImageMagick's import for frames, and fitdecode for the
# file. Build the simulator first (its CMakeLists.txt).
#
# Usage:  UNA_SDK=/path/to/una-sdk ./sim_plan.sh [shots-dir]
#   WEEK=n     weeks the plan has run (0-9);
#   STEP_SECS  seconds to run each step before pressing R2 (default 12);
#   PLAN=2     the half-marathon plan instead (12 weeks).
set -u
SHOTS=${1:-}
WEEK=${WEEK:-4}
STEP_SECS=${STEP_SECS:-12}
PLAN=${PLAN:-1}
REPO=$(cd "$(dirname "$0")/../.." && pwd)
SOFT="$REPO/Software"
BIN="$SOFT/Apps/LVGL-GUI/simulator/build/bin"
DISP=:95
LOG=${SIM_LOG:-/tmp/x-training-sim.log}

[ -x "$BIN/HybridXTrainingSimulator" ] || { echo "Build the simulator first ($BIN)" >&2; exit 1; }

# Today, as the simulator will see it (it uses the host's clock and zone).
TODAY=$(date +%Y%m%d)
DOW=$(( $(date +%u) - 1 ))            # 0 Monday .. 6 Sunday
LONG=$(( (DOW + 5) % 7 ))             # today is two days after the long run: the first quality day
DAYS=0
for k in 0 2 3 5; do DAYS=$(( DAYS | (1 << ((LONG + k) % 7)) )); done
START=$(date -d "$(date +%F) -$DOW days -$(( WEEK * 7 )) days" +%Y%m%d)
WEEKS=$([ "$PLAN" = 2 ] && echo 12 || echo 10)

rm -rf "$SOFT/Output" "$SOFT/SharedData"
mkdir -p "$SOFT/Output"
bool() { [ $(( (DAYS >> $1) & 1 )) = 1 ] && echo true || echo false; }
cat > "$SOFT/Output/app_config.json" <<EOF
{"schema":1,"values":{"plan":$PLAN,"fiveK":"22:00","fiveKRecent":true,"halfTime":"","raceDate":"","level":2,
"runMon":$(bool 0),"runTue":$(bool 1),"runWed":$(bool 2),"runThu":$(bool 3),"runFri":$(bool 4),"runSat":$(bool 5),
"runSun":$(bool 6),"longRunDay":$(( LONG + 1 )),"easyByHr":false}}
EOF
printf '{"v":1,"s":[%d,2,%d,%d,0,0,0,1320,%s,1,0,0],"c":[0,%s,0,%d],"log":[]}' \
    "$PLAN" "$DAYS" "$LONG" "$START" "$START" "$WEEKS" > "$SOFT/Output/plan.json"
echo "--- pretend watch: today $TODAY (weekday $DOW), long run day $LONG, days mask $DAYS, plan from $START"

Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
sleep 2
cd "$BIN"
DISPLAY=$DISP stdbuf -oL -eL ./HybridXTrainingSimulator > "$LOG" 2>&1 & SIMPID=$!
cleanup() { kill -9 $SIMPID 2>/dev/null; kill $XPID 2>/dev/null; }
trap cleanup EXIT
for _ in $(seq 60); do
    grep -q "Mock.Buzzer::play" "$LOG" && break   # RunLVGL's first-fix buzz
    sleep 1
done
sleep 2
WID=$(DISPLAY=$DISP xdotool search --name 'X Training' | head -1)
[ -n "$WID" ] || { echo "FATAL: no simulator window" >&2; exit 1; }
key()  { DISPLAY=$DISP xdotool key "$1"; sleep "${2:-1}"; }
hold() { DISPLAY=$DISP xdotool keydown "$1"; sleep "$2"; DISPLAY=$DISP xdotool keyup "$1"; sleep 1; }
snap() { [ -n "$SHOTS" ] && mkdir -p "$SHOTS" && DISPLAY=$DISP timeout 10 import -window "$WID" "$SHOTS/$1.png" 2>/dev/null; return 0; }

echo "--- the plan, as the service read it"
grep -E "Plan settings|New plan|Paces s/km|Today:" "$LOG" | sed 's/^ *//'
snap 01-start                             # Start, with today's session under it
key 3 1                                   # R1: Start -> the countdown
snap 02-countdown
sleep 6                                   # the countdown runs out; the session starts
snap 03-first-step
grep -E "Planned session started" "$LOG" | sed 's/^ *//'
STEPS=$(grep -oE "Planned session started: .*, ([0-9]+) steps" "$LOG" | grep -oE "[0-9]+ steps" | grep -oE "[0-9]+")
RUNS=0
# Step through: each step runs a few seconds, then R2 ends it, until the workout ends.
for i in $(seq 1 60); do
    sleep "$STEP_SECS"
    [ "$i" = 2 ] && snap 04-a-step
    key 4 1                               # R2: the next step
    RUNS=$i
    grep -q "Workout completed" "$LOG" && break   # the service, after the last step
done
echo "--- $RUNS steps ended with R2 (the program has $STEPS steps, repeats included in the run)"
sleep 2
snap 05-completed                         # "Workout completed", which dismisses itself
sleep 8
snap 05b-after-steps
key 3 1                                   # R1: action menu (pauses)
snap 06-menu
key 2; key 2                              # L2 twice: Save (Resume, End workout or Summary, Save)
snap 06b-menu-save
hold 3 3                                  # hold R1: save
sleep 3
snap 07-saved
sleep 2

echo "--- after the run (service log)"
grep -E "Planned session|Today:|Workout|plan.json|Track stopped|activity" "$LOG" | sed 's/^ *//' | tail -n 20
echo "--- plan.json"
cat "$SOFT/Output/plan.json"; echo
echo "--- shared files (there should be none: no activity link until TP4)"
find "$SOFT/SharedData" -type f 2>/dev/null
echo "--- the FIT file"
FIT=$(find "$SOFT/Output" -name "*.fit" | head -1)
if [ -n "$FIT" ]; then
    python3 "$REPO/docs/experiments/fit_plan_report.py" "$FIT"
else
    echo "no .fit written"
fi
