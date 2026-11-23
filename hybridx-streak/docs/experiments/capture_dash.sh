#!/bin/bash
#
# Screenshots of the family dashboard (NOTES "S7") in the REAL HybridX Streak app,
# cut to the watch's round face, into docs/screens/real/ (s7-*.png).
#
# Pretend watches from sim_fixtures_dash.sh:
#   A  the whole family: a week of five apps' sessions, X Training's session today,
#      X Run's VO2max, three weeks of X Weigh-in; the menu, By sport and Your numbers;
#   B  the same with today's session done, in stones and pounds: "Done", "Next";
#   C  only the activity link: Your numbers names the app each row needs;
#   D  a banked week with X Training's session to do: Home's coach line names it.
# Each run's log is checked for the service's "Family:" line.
#
# Build first: the real simulator (Software/Apps/LVGL-GUI/simulator/build).
# Needs Xvfb, xdotool and ImageMagick. About two minutes.
# Usage:  ./capture_dash.sh [output-dir]
set -u

APP=$(cd "$(dirname "$0")/../.." && pwd)
BIN="$APP/Software/Apps/LVGL-GUI/simulator/build/bin/HybridXStreakSimulator"
OUT=${1:-$APP/docs/screens/real}
DISP=:93
WORK=$(mktemp -d)
mkdir -p "$OUT"
FAIL=0

convert -size 520x520 xc:'#1b1f22' -fill black -draw "circle 259.5,259.5 259.5,20" -transparent black \
    -fill none -stroke '#3a4044' -strokewidth 5 -draw "circle 259.5,259.5 259.5,17" "$WORK/bezel.png"

Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
sleep 2

run() {
    cd "$1/a/b/c/d/e"
    DISPLAY=$DISP "$BIN" > "$1/sim.log" 2>&1 & SIMPID=$!
    sleep 0.8
    WID=$(DISPLAY=$DISP xdotool search --name 'HybridX Streak' | head -1)
}
stop() { DISPLAY=$DISP xdotool key Escape; sleep 1; kill $SIMPID 2>/dev/null; }
key() { DISPLAY=$DISP xdotool key "$1"; sleep "${2:-1.1}"; }
snap() {
    DISPLAY=$DISP import -window "$WID" "$WORK/raw.png" 2>/dev/null
    convert "$WORK/raw.png" -background black -gravity center -extent 520x520 "$WORK/bezel.png" -composite "$OUT/$1.png"
}
expect() {   # expect <log> <pattern> <what>
    if grep -q "$2" "$1"; then echo "PASS  $3"; else echo "FAIL  $3 (no \"$2\" in $1)"; FAIL=1; fi
}

# Keys: 1 L1 (up), 2 L2 (down), 3 R1 (select), 4 R2 (back).
# The menu: Training load, This week, By sport, Your numbers, Log a session, Trophy case, Settings.

# -- A: the whole family -----------------------------------------------------------------
A="$WORK/a"
bash "$APP/docs/experiments/sim_fixtures_dash.sh" "$A" >/dev/null
run "$A"
sleep 17                          # the first open's five arrivals and the week's step up
key 3;      key 2;  key 2;  snap s7-01-menu-by-sport   # past Training load and This week
key 2;      snap s7-02-menu-your-numbers
key 1;      key 3;  snap s7-03-by-sport-week
key 2;      snap s7-04-by-sport-distance
key 2;      snap s7-05-by-sport-run
key 2;      snap s7-06-by-sport-ride
key 2;      snap s7-07-by-sport-strength
key 2;      snap s7-08-by-sport-hybrid
key 2;      snap s7-09-by-sport-last-week
key 4;      key 2;  key 3;  snap s7-10-numbers-plan
key 2;      snap s7-11-numbers-pace
key 2;      snap s7-12-numbers-vo2max
key 2;      snap s7-13-numbers-weight
key 2;      snap s7-14-numbers-trend
key 4; key 4
stop
expect "$A/sim.log" "Family: plan yes (state 7), VO2max 523" "A: the plan (a session today) and the VO2max are read"
expect "$A/sim.log" "Summaries: 5 files" "A: five apps' summaries"

# -- B: today's session done, stones and pounds ---------------------------------------
B="$WORK/b"
TODAY_STATE=done UNITS=1 bash "$APP/docs/experiments/sim_fixtures_dash.sh" "$B" >/dev/null
run "$B"
sleep 17
key 3; key 2; key 2; key 2; key 3; snap s7-20-numbers-done
key 2;      snap s7-21-numbers-next
key 2; key 2; snap s7-22-numbers-weight-st
key 2;      snap s7-23-numbers-trend-lb
key 4; key 4
stop
expect "$B/sim.log" "Family: plan yes (state 8)" "B: today's session read as done"

# -- C: the activity link alone --------------------------------------------------------
C="$WORK/c"
NO_FAMILY=1 bash "$APP/docs/experiments/sim_fixtures_dash.sh" "$C" >/dev/null
run "$C"
sleep 17
key 3; key 2; key 2; key 2; snap s7-30-menu-numbers-empty
key 3;      snap s7-31-numbers-needs-training
key 2;      snap s7-32-numbers-needs-run
key 2;      snap s7-33-numbers-needs-weighin
key 4; key 4
stop
expect "$C/sim.log" "Family: plan no (state 0), VO2max 0" "C: no family files, nothing read"

# -- D: a banked week (3 of 3) with X Training's session still to do: Home names it ----------
D="$WORK/d"
BANKED=1 bash "$APP/docs/experiments/sim_fixtures_dash.sh" "$D" >/dev/null
run "$D"
sleep 14
snap s7-40-home-banked-plan
stop
expect "$D/sim.log" "Streak line: 3 of 3 this week" "D: the week is banked at exactly its target"

kill $XPID 2>/dev/null
echo "Screens in $OUT (s7-*.png); logs in $WORK"
exit $FAIL
