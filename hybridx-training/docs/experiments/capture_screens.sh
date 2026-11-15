#!/bin/bash
#
# X Training's screens in the PC simulator (TP3), by key presses, into
# docs/screens/tp3-*.png (or the folder given).
#
# Each scenario is a pretend watch: app_config.json (the phone's settings) and
# a plan.json made so that the host's today falls where the scenario needs it
# (before the plan starts, a rest day, a session day, the plan over, ...). The
# simulator reads the host's clock and zone. The service keeps a plan.json
# whose settings match the phone's, as on the watch.
#
# As sim_plan.sh: Xvfb for a display, xdotool for the buttons (keys 1..4 are
# L1, L2, R1, R2), ImageMagick's import for frames. Build the simulator first
# (its CMakeLists.txt). About 2 minutes.
#
# Usage:  UNA_SDK=/path/to/una-sdk ./capture_screens.sh [out-dir] [scenario ...]
#   scenarios: before rest session done finished noplan problem (default: all)
set -u
REPO=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:-$REPO/docs/screens}
shift || true
SCENARIOS=${*:-before rest session done finished noplan problem}
SOFT="$REPO/Software"
BIN="$SOFT/Apps/LVGL-GUI/simulator/build/bin"
DISP=:96
LOG=${SIM_LOG:-/tmp/x-training-capture.log}
[ -x "$BIN/HybridXTrainingSimulator" ] || { echo "Build the simulator first ($BIN)" >&2; exit 1; }
mkdir -p "$OUT"

TODAY=$(date +%Y%m%d)
DOW=$(( $(date +%u) - 1 ))                       # 0 Monday .. 6 Sunday
MONDAY=$(date -d "$(date +%F) -$DOW days" +%Y%m%d)
monday_weeks() { date -d "$(date +%F) -$DOW days $1 weeks" +%Y%m%d; }   # this Monday +/- n weeks

Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
SIMPID=
cleanup() { [ -n "$SIMPID" ] && kill -9 "$SIMPID" 2>/dev/null; kill $XPID 2>/dev/null; }
trap cleanup EXIT
sleep 2

# The phone's settings: plan, 5K, days mask (bit 0 Monday), long run day 1..7.
config() {
    local plan=$1 fivek=$2 days=$3 long=$4
    b() { [ $(( (days >> $1) & 1 )) = 1 ] && echo true || echo false; }
    cat > "$SOFT/Output/app_config.json" <<EOF
{"schema":1,"values":{"plan":$plan,"fiveK":"$fivek","fiveKRecent":true,"halfTime":"","raceDate":"","level":2,
"runMon":$(b 0),"runTue":$(b 1),"runWed":$(b 2),"runThu":$(b 3),"runFri":$(b 4),"runSat":$(b 5),
"runSun":$(b 6),"longRunDay":$long,"easyByHr":false}}
EOF
}
# plan.json for the 10K plan at level 2, 5K 22:00, from @start, with an optional log entry.
planfile() {
    local days=$1 long=$2 start=$3 log=${4:-}
    printf '{"v":1,"s":[1,2,%d,%d,0,0,0,1320,%s,1,0,0],"c":[0,%s,0,10],"log":[%s]}' \
        "$days" "$(( long - 1 ))" "$start" "$start" "$log" > "$SOFT/Output/plan.json"
}

start_sim() {
    DISPLAY=$DISP stdbuf -oL -eL "$BIN/HybridXTrainingSimulator" > "$LOG" 2>&1 & SIMPID=$!
    for _ in $(seq 60); do
        grep -q "Today: status" "$LOG" && grep -q "Mock.Buzzer::play" "$LOG" && break
        sleep 1
    done
    sleep 2
    WID=$(DISPLAY=$DISP xdotool search --name 'X Training' | head -1)
    [ -n "$WID" ] || { echo "FATAL: no simulator window" >&2; exit 1; }
}
stop_sim() { kill -9 "$SIMPID" 2>/dev/null; wait "$SIMPID" 2>/dev/null; SIMPID=; }
key()  { DISPLAY=$DISP xdotool key "$1"; sleep "${2:-1}"; }
snap() { DISPLAY=$DISP timeout 10 import -window "$WID" "$OUT/tp3-$1.png" 2>/dev/null; echo "  tp3-$1"; }
fresh() { rm -rf "$SOFT/Output" "$SOFT/SharedData"; mkdir -p "$SOFT/Output"; }

# Four run days with today among them: today, +2, +3 and +5 days on; the long run +5.
DAYS_TODAY=0
for k in 0 2 3 5; do DAYS_TODAY=$(( DAYS_TODAY | (1 << ((DOW + k) % 7)) )); done
LONG_TODAY=$(( (DOW + 5) % 7 + 1 ))
# The same days moved one on: today is a rest day.
DAYS_REST=0
for k in 1 3 4 6; do DAYS_REST=$(( DAYS_REST | (1 << ((DOW + k) % 7)) )); done
LONG_REST=$(( (DOW + 6) % 7 + 1 ))

cd "$BIN" || exit 1
for sc in $SCENARIOS; do
    echo "--- $sc"
    fresh
    case $sc in
    before)   # Jon's first install: the plan starts next Monday
        config 1 22:00 "$DAYS_TODAY" "$LONG_TODAY"
        planfile "$DAYS_TODAY" "$LONG_TODAY" "$(monday_weeks 1)"
        start_sim
        snap before-main; key 3; snap before-today ;;
    rest)
        config 1 22:00 "$DAYS_REST" "$LONG_REST"
        planfile "$DAYS_REST" "$LONG_REST" "$(monday_weeks -1)"
        start_sim
        snap rest-main; key 3; snap rest-today ;;
    session)  # week 5 of the 10K plan, today a quality day
        config 1 22:00 "$DAYS_TODAY" "$LONG_TODAY"
        planfile "$DAYS_TODAY" "$LONG_TODAY" "$(monday_weeks -4)"
        start_sim
        snap session-main; key 3; snap session-today
        key 2; snap session-step1; key 2; snap session-step2; key 2; snap session-step3
        key 1; key 1; key 1; key 3 2; snap session-countdown ;;
    done)
        config 1 22:00 "$DAYS_TODAY" "$LONG_TODAY"
        planfile "$DAYS_TODAY" "$LONG_TODAY" "$(monday_weeks -4)" "[$TODAY,7,1,0]"
        start_sim
        snap done-main; key 3; snap done-today ;;
    finished) # a 10-week plan that ended last week
        config 1 22:00 "$DAYS_TODAY" "$LONG_TODAY"
        planfile "$DAYS_TODAY" "$LONG_TODAY" "$(monday_weeks -11)"
        start_sim
        snap finished-main; key 3; snap finished-today ;;
    noplan)
        config 0 22:00 "$DAYS_TODAY" "$LONG_TODAY"
        start_sim
        snap noplan-main; key 3; snap noplan-today ;;
    problem)  # a plan, but no 5K
        config 1 "" "$DAYS_TODAY" "$LONG_TODAY"
        start_sim
        snap problem-main; key 3; snap problem-today ;;
    *) echo "unknown scenario $sc" >&2 ;;
    esac
    grep -E "Today:" "$LOG" | tail -1 | sed 's/^ *//'
    stop_sim
done
fresh
echo "--- screens in $OUT"
