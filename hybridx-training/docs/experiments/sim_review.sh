#!/bin/bash
#
# X Training in the PC simulator: the session review's suggestion and the Form page (TP8), on pretend
# watches whose reviews.json is seeded (a real session takes half its planned time to count: sim_plan.sh
# REVIEW=1 runs one). Four short runs of the app:
#
#   1. two sessions too slow in a row            -> Today asks nothing (one or two bad runs are never a suggestion)
#   2. three too slow, tired, then "Yes, hold it" -> Today asks; yes: the plan grows by a week (plan.json),
#                                                   reviews.json remembers the answer; not asked again
#   3. the same, then "Not now"                   -> the plan is unchanged, the answer is remembered
#   4. eight reviewed sessions                    -> Plan, L2: Form (a capture)
#
# Needs Xvfb, xdotool, ImageMagick's import, and build-tests/review_print (the host tests' build).
# Build the simulator first. Takes about 2 minutes.
#
# Usage:  UNA_SDK=/path/to/una-sdk ./sim_review.sh [shots-dir]
set -u
SHOTS=${1:-}
REPO=$(cd "$(dirname "$0")/../.." && pwd)
SOFT="$REPO/Software"
BIN="$SOFT/Apps/LVGL-GUI/simulator/build/bin"
RP="$REPO/build-tests/review_print"
DISP=:93
LOG=${SIM_LOG:-/tmp/x-training-review.log}
FAILED=0

[ -x "$BIN/HybridXTrainingSimulator" ] || { echo "Build the simulator first ($BIN)" >&2; exit 1; }
[ -x "$RP" ] || { echo "Build the host tests first (build-tests/review_print)" >&2; exit 1; }

TODAY=$(date +%Y%m%d)
DOW=$(( $(date +%u) - 1 ))
LONG=$(( (DOW + 5) % 7 ))           # today is the first quality day, as sim_plan.sh has it
DAYS=0
for k in 0 2 3 5; do DAYS=$(( DAYS | (1 << ((LONG + k) % 7)) )); done
WEEK=4
START=$(date -d "$(date +%F) -$DOW days -$(( WEEK * 7 )) days" +%Y%m%d)
D1=$(date -d "-1 day" +%Y%m%d); D2=$(date -d "-2 days" +%Y%m%d); D3=$(date -d "-3 days" +%Y%m%d)
D4=$(date -d "-4 days" +%Y%m%d); D5=$(date -d "-5 days" +%Y%m%d); D6=$(date -d "-6 days" +%Y%m%d)
D7=$(date -d "-7 days" +%Y%m%d); D8=$(date -d "-8 days" +%Y%m%d)

bool() { [ $(( (DAYS >> $1) & 1 )) = 1 ] && echo true || echo false; }
pretend_watch() {
    rm -rf "$SOFT/Output" "$SOFT/SharedData"; mkdir -p "$SOFT/Output"
    cat > "$SOFT/Output/app_config.json" <<EOF
{"schema":1,"values":{"plan":1,"fiveK":"22:00","fiveKRecent":true,"halfTime":"","raceDate":"","level":2,
"runMon":$(bool 0),"runTue":$(bool 1),"runWed":$(bool 2),"runThu":$(bool 3),"runFri":$(bool 4),"runSat":$(bool 5),
"runSun":$(bool 6),"longRunDay":$(( LONG + 1 )),"easyByHr":false}}
EOF
    printf '{"v":1,"s":[1,2,%d,%d,0,0,0,1320,%s,1,0,0],"c":[0,%s,0,10],"log":[],"st":%s}' \
        "$DAYS" "$LONG" "$START" "$START" "$TODAY" > "$SOFT/Output/plan.json"
}

XPID=""
SIMPID=""
cd "$BIN"
launch() {
    rm -f "$LOG"
    Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
    sleep 2
    DISPLAY=$DISP stdbuf -oL -eL ./HybridXTrainingSimulator > "$LOG" 2>&1 & SIMPID=$!
    for _ in $(seq 60); do grep -q "Mock.Buzzer::play" "$LOG" 2>/dev/null && break; sleep 1; done
    sleep 2
    WID=$(DISPLAY=$DISP xdotool search --name 'X Training' | head -1)
    [ -n "$WID" ] || { echo "FATAL: no simulator window" >&2; exit 1; }
}
stop() {
    [ -n "$SIMPID" ] && kill -9 $SIMPID 2>/dev/null
    [ -n "$XPID" ] && kill $XPID 2>/dev/null
    wait 2>/dev/null
    SIMPID=""; XPID=""
    sleep 1
}
trap 'stop' EXIT
key()  { DISPLAY=$DISP xdotool key "$1"; sleep "${2:-1}"; }
snap() { [ -n "$SHOTS" ] && mkdir -p "$SHOTS" && DISPLAY=$DISP timeout 10 import -window "$WID" "$SHOTS/$1.png" 2>/dev/null; return 0; }
ok()   { echo "PASS $1"; }
bad()  { echo "FAIL $1"; FAILED=1; }
status_is() {   # the service's last "Today: status N" line
    local got
    got=$(grep -oE "Today: status [0-9]+" "$LOG" | tail -1 | grep -oE "[0-9]+$")
    [ "$got" = "$1" ] && ok "Today's status is $1 ($2)" || bad "Today's status is ${got:-none}, wanted $1 ($2)"
}
weeks_are() {
    local got
    got=$(python3 -c "import json;print(json.load(open('$SOFT/Output/plan.json'))['c'][3])")
    [ "$got" = "$1" ] && ok "the plan has $1 weeks ($2)" || bad "the plan has $got weeks, wanted $1 ($2)"
}
rp() { out=$("$RP" "$SOFT/Output/reviews.json" "$@"); echo "$out" | sed 's/^/  /'; echo "$out" | grep -q "^FAIL" && FAILED=1; return 0; }

echo "=== 1. two sessions too slow: nothing is asked"
pretend_watch
"$RP" --make "$SOFT/Output/reviews.json" "$D2:30:0:4" "$D1:20:0:4"
launch
status_is 1 "a session, no offer"
snap r1-today-no-offer
stop

echo "=== 2. three too slow and tired: Today asks; yes holds the plan"
pretend_watch
"$RP" --make "$SOFT/Output/reviews.json" "$D3:30:0:4" "$D2:20:0:4" "$D1:10:0:5"
launch
status_is 9 "an offer"
grep -E "Today: status 9" "$LOG" | tail -1 | sed 's/^ *//'
snap r2-today-asks
key 3 1                                  # R1: Today's page, with the offer on it
snap r2-today-page
key 3 1                                  # R1: the question
snap r2-offer
key 3 2                                  # R1: "Yes, hold it" (the first choice)
snap r2-answered
grep -E "The plan is held|Plan action" "$LOG" | sed 's/^ *//'
weeks_are 11 "one more than before"
status_is 1 "a session again once it is answered"
stop
rp --expect-offer no --expect-handled "$D1"
# Not asked again for the same sessions: the app opened once more.
launch
status_is 1 "not asked again"
stop

echo "=== 3. the same, and \"Not now\""
pretend_watch
"$RP" --make "$SOFT/Output/reviews.json" "$D3:30:0:4" "$D2:20:0:4" "$D1:10:0:5"
launch
key 3 1                                  # R1: Today's page
key 3 1                                  # R1: the question
key 2 1                                  # L2: "Not now"
snap r3-not-now
key 3 2
weeks_are 10 "unchanged"
status_is 1 "a session again"
stop
rp --expect-offer no --expect-handled "$D1"

echo "=== 4. eight reviewed sessions: Plan, then Form (L2)"
pretend_watch
"$RP" --make "$SOFT/Output/reviews.json" "$D8:90:5:2" "$D7:80:15:3" "$D6:0:0:0" "$D5:50:50:4" "$D4:30:0:5" "$D3:20:0:4" \
    "$D2:75:10:1" "$D1:100:0:2"
launch
for _ in 1 2 3 4; do key 2 0.5; done     # Today -> ... -> Plan
key 3 2                                  # R1: Plan
snap r4-plan
key 2 2                                  # L2: Form
snap r4-form
grep -E "Today: status" "$LOG" | tail -1 | sed 's/^ *//'
key 1 1                                  # L1: back to Plan
snap r4-plan-again
stop
rp --expect-count 8

echo
[ "$FAILED" = 0 ] && echo "ALL PASS" || { echo "SOME FAILED"; exit 1; }
