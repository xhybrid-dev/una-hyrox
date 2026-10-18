#!/bin/bash
#
# Play stage 1 from a new game to the stage clear in the PC simulator, and say
# whether it got there: an end-to-end check of the whole game through the real
# service and screens.
#
# The beat is turned off (every turn counts as Good, as menu-only play does)
# and R1 does everything: the first way at each fork, the skill in every duel,
# yes to every offer. Then it leaves (R2 held), starts the simulator again and
# checks the game picks up from the save.
#
# Xvfb and xdotool. Build the simulator first. The save in Software/Output is
# cleared first.
#
# Usage:  ./sim_play.sh [seed]        exit status 0 if the stage was cleared
set -u
REPO=$(cd "$(dirname "$0")/../.." && pwd)
BIN="$REPO/Software/Apps/LVGL-GUI/simulator/build/bin"
SAVES="$REPO/Software/Output"
DISP=:98
LOG=${SIM_LOG:-/tmp/trailquest-play.log}
SEED=${1:-7}

[ -x "$BIN/TrailQuestSimulator" ] || { echo "Build the simulator first ($BIN)" >&2; exit 1; }
rm -f "$SAVES"/save.txt* "$SAVES"/settings.txt*

Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
SIMPID=""
cleanup() { [ -n "$SIMPID" ] && kill -9 $SIMPID 2>/dev/null; kill $XPID 2>/dev/null; }
trap cleanup EXIT
sleep 2

start() {
    ( cd "$BIN" && exec env DISPLAY=$DISP TRAILQUEST_SEED=$SEED stdbuf -oL -eL ./TrailQuestSimulator >> "$LOG" 2>&1 ) & SIMPID=$!
    sleep 3
}
key() { DISPLAY=$DISP xdotool key "$1"; sleep "${2:-0.9}"; }

: > "$LOG"
start
key 2 0.5          # the beat off
cleared=""
for i in $(seq 1 300); do
    key 3
    if grep -aq 'step: .* phase 4 ' "$LOG"; then cleared=$i; break; fi
done

echo "=== steps ==="
grep -a 'step:' "$LOG" | sed 's/^.*step: /  /'
if [ -z "$cleared" ]; then
    echo "FAIL: no stage clear after 300 presses" >&2
    exit 1
fi
echo "Stage 1 cleared after $cleared presses."

# Leave (R2 held), start again, and check the save is picked up.
DISPLAY=$DISP xdotool keydown 4; sleep 2; DISPLAY=$DISP xdotool keyup 4; sleep 1
kill -9 $SIMPID 2>/dev/null; SIMPID=""
start
if grep -aq 'resumed: stage 0, place 8' "$LOG"; then
    echo "The save was picked up at the stage clear."
else
    echo "FAIL: the save was not picked up" >&2
    grep -aE 'resumed|no saved|unusable' "$LOG" | tail -3 >&2
    exit 1
fi
