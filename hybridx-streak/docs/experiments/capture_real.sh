#!/bin/bash
#
# Screenshots of the REAL HybridX Streak app (not the design demo), one per
# screen and moment, cut to the watch's round face, into docs/screens/real/.
#
# Two pretend watches (sim_fixtures.sh + make_state):
#   A  a first week: three activities from three apps, a short walk, a file
#      being recorded, a broken one; then every menu screen;
#   B  eleven weeks achieved and last week missed: the shield offer, and a
#      week that reaches the top of Snowdon.
#
# Build first: the real simulator (Software/Apps/LVGL-GUI/simulator/build)
# and the host tools (build-tests: make_fit, make_state).
# Usage:  ./capture_real.sh [output-dir]
set -u

APP=$(cd "$(dirname "$0")/../.." && pwd)
BIN="$APP/Software/Apps/LVGL-GUI/simulator/build/bin/HybridXStreakSimulator"
OUT=${1:-$APP/docs/screens/real}
DISP=:92
WORK=$(mktemp -d)
mkdir -p "$OUT"

convert -size 520x520 xc:'#1b1f22' -fill black -draw "circle 259.5,259.5 259.5,20" -transparent black \
    -fill none -stroke '#3a4044' -strokewidth 5 -draw "circle 259.5,259.5 259.5,17" "$WORK/bezel.png"

Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
sleep 2

# run <tree>: start the simulator from the tree; sets WID and SIMPID.
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

# -- A: a first week, and every menu screen ---------------------------------------
A="$WORK/a"
bash "$APP/docs/experiments/sim_fixtures.sh" "$A" >/dev/null
run "$A"
sleep 0.9;  snap 01-first-run-found
sleep 4.7;  snap 02-week-complete
sleep 3.2;  snap 03-home-week-banked
key 3;      snap 04-menu
key 3;      snap 05-this-week
key 2; key 2; key 2; snap 06-this-week-too-short
key 3;      snap 07-leave-it-out
key 4; key 4
key 2; key 3; snap 08-log-what
key 3;      snap 09-log-when
key 4; key 4
key 2; key 3; snap 10-trophy-summits
key 2; key 2; key 2; key 2; key 2; snap 11-trophy-badges
key 4
key 2; key 3; snap 12-settings
key 3;      snap 13-weekly-target
key 4
key 2; key 2; key 2; key 2; snap 14-one-per-day
key 4; key 4
stop

# -- B: a missed week with shields, and a summit ------------------------------------
B="$WORK/b"
bash "$APP/docs/experiments/sim_fixtures.sh" "$B" >/dev/null
"$APP/build-tests/make_state" "$B/Output/state.json" 11 1 >/dev/null
run "$B"
sleep 0.9;  snap 20-last-week
sleep 2.2;  snap 21-shield-offer
key 3 0.8;  snap 22-streak-saved
sleep 11.2; snap 23-summit-snowdon
sleep 1.3;  snap 24-summit-confetti
key 3 1.5;  snap 25-next-ben-nevis
stop

kill $XPID 2>/dev/null
grep -h "LVGL pool" "$A/sim.log" "$B/sim.log" | sed 's/.*peak \([0-9]*\)%.*/\1/' | sort -n | tail -1 | xargs -I{} echo "LVGL pool peak {}%"
rm -rf "$WORK"
echo "Captured to $OUT"
