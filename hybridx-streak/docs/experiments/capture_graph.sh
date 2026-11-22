#!/bin/bash
#
# Screenshots of the training load graphs (NOTES "S8") in the REAL HybridX Streak app, cut to
# the watch's round face, into docs/screens/real/ (s8-*.png).
#
# Pretend watches (sim_fixtures_dash.sh, plus make_state's history):
#   A  sixteen weeks of a training block (make_state ... block) and this week's sessions: the
#      Training load screen, then R1 into each graph and L2 through all three, R2 back;
#   B  a first week: the line graphs say "Learning", the bars show;
#   C  the same history as A with its "H" taken out (a file from before S8, as for anyone
#      updating): the lines start with the two weeks still open, plus the days since the save.
# Each run's log is checked for the service's "Series:" line.
#
# Build first: the real simulator (Software/Apps/LVGL-GUI/simulator/build) and build-tests'
# make_state. Needs Xvfb, xdotool and ImageMagick. About a minute and a half.
# Usage:  ./capture_graph.sh [output-dir]
set -u

APP=$(cd "$(dirname "$0")/../.." && pwd)
BIN="$APP/Software/Apps/LVGL-GUI/simulator/build/bin/HybridXStreakSimulator"
OUT=${1:-$APP/docs/screens/real}
DISP=:94
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
# The menu: This week, By sport, Your numbers, Log a session, Trophy case, Training load, Settings.
to_load() { key 3; key 2; key 2; key 2; key 2; key 2; key 3; }   # Home -> menu -> Training load

# -- A: sixteen weeks of a block ---------------------------------------------------------------
A="$WORK/a"
bash "$APP/docs/experiments/sim_fixtures_dash.sh" "$A" >/dev/null
"$APP/build-tests/make_state" "$A/Output/state.json" 16 0 block >/dev/null
run "$A"
sleep 17                         # this week's arrivals
to_load;    snap s8-01-load-form
key 2;      snap s8-02-load-fitness
key 3 1.5;  snap s8-03-graph-fitness-fatigue
key 2;      snap s8-04-graph-form
key 2;      snap s8-05-graph-daily-load
key 2;      snap s8-06-graph-wraps-round
key 4;      snap s8-07-back-to-load
key 4; key 4
stop
expect "$A/sim.log" "Series: 84 days to" "A: twelve weeks of days sent to the graphs"

# -- B: a first week ---------------------------------------------------------------------------
B="$WORK/b"
bash "$APP/docs/experiments/sim_fixtures_dash.sh" "$B" >/dev/null
run "$B"
sleep 17
to_load; key 2; key 3 1.5; snap s8-10-graph-learning
key 2; key 2; snap s8-11-graph-daily-load-first-week
key 4; key 4; key 4
stop
expect "$B/sim.log" "Series: 1 days to" "B: a first week has its one day"

# -- C: a history from before S8 (no "H") ------------------------------------------------------
C="$WORK/c"
bash "$APP/docs/experiments/sim_fixtures_dash.sh" "$C" >/dev/null
"$APP/build-tests/make_state" "$C/Output/state.json" 16 0 block >/dev/null
python3 - "$C/Output/state.json" <<'PY'
import re, sys
p = sys.argv[1]
s = open(p).read()
s2 = re.sub(r',"H":\[1,\d+,\[[^\]]*\],\[[^\]]*\],\[[^\]]*\]\]', '', s)
assert s2 != s, "no history to take out"
open(p, "w").write(s2)
PY
run "$C"
sleep 17
to_load; key 2; key 3 1.5; snap s8-20-graph-updated-athlete
key 4; key 4; key 4
stop
# The open itself folds the days since the last save into a fresh history: the two open weeks
# plus those days (one to seven: make_state saves on a Sunday), never the months before.
expect "$C/sim.log" "history 1[0-9][0-9] days, [1-7] kept" "C: an older file starts the lines at the open weeks"

kill $XPID 2>/dev/null
echo "Screens in $OUT (s8-*.png); logs in $WORK"
exit $FAIL
