#!/bin/bash
#
# The store previews for Trail Quest: six simulator screens, 480x480, written to
# Resources/previews/ (the folder the packager puts in the zip). Three short runs from
# hand-written saves (the save in Software/Output is replaced):
#   1-title    the title, a game in progress (Level 9)
#   2-map      the second stage's map at a fork: the hut, or straight on
#   3-duel     a boss duel at the choice: Surge, Skill, Settle, Fuel
#   4-skill    R1's pick list: a pack creature's move, its damage and effect
#   5-needle   the timing needle after a pick
#   6-finale   the summit
#
# Xvfb, xdotool and ImageMagick. Build the simulator first.
# Usage:  ./store_previews.sh [out-dir]       out-dir defaults to Resources/previews.
set -u
REPO=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:-$REPO/Resources/previews}
BIN="$REPO/Software/Apps/LVGL-GUI/simulator/build/bin"
SAVES="$REPO/Software/Output"
DISP=:92
LOG=${SIM_LOG:-/tmp/trailquest-previews.log}

[ -x "$BIN/TrailQuestSimulator" ] || { echo "Build the simulator first ($BIN)" >&2; exit 1; }
mkdir -p "$OUT" "$SAVES"
rm -f "$OUT"/*.png

Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
SIMPID=""
stop_sim() { [ -n "$SIMPID" ] && kill -9 $SIMPID 2>/dev/null; SIMPID=""; sleep 1; }
cleanup() { stop_sim; kill $XPID 2>/dev/null; }
trap cleanup EXIT
sleep 2

# write_save stage node level stamina bossbits lap won lost
write_save() {
    rm -f "$SAVES"/save.txt* "$SAVES"/settings.txt*
    cat > "$SAVES/save.txt" <<EOT
tq=1
rng=12345
lv=$3
xp=0
st=$4
pc=3
p0=0
p1=1
p2=2
bag=3,1,1,0,0
stage=$1
node=$2
hut=3
min=600
wx=0
lap=$6
boss=$5
cache=0
met=0
beat=0
won=$7
lost=$8
perf=40
offer=255
EOT
}

start_sim() {
    : > "$LOG"
    ( cd "$BIN" && exec env DISPLAY=$DISP TRAILQUEST_SEED=7 stdbuf -oL -eL ./TrailQuestSimulator >> "$LOG" 2>&1 ) & SIMPID=$!
    for _ in $(seq 1 100); do grep -aq 'GUI is now running' "$LOG" && break; sleep 0.2; done
    sleep 2
    WID=$(DISPLAY=$DISP xdotool search --name 'Trail Quest' | tail -1)
    [ -n "$WID" ] || { echo "FATAL: no simulator window" >&2; exit 1; }
}
key()  { DISPLAY=$DISP xdotool key "$1"; sleep "${2:-0.7}"; }
snap() { DISPLAY=$DISP timeout 10 import -window "$WID" "$OUT/$1.png" 2>/dev/null; echo "  $1"; }

# Run 1: the title, then the map at a fork.
write_save 1 1 9 60 0 0 12 2
start_sim
snap 1-title
key 3 1.0; key 3 0.8      # past the title and the welcome card
snap 2-map
stop_sim

# Run 2: at the foot of the Ram King: the duel, the pick list, the needle.
write_save 2 6 10 76 0 0 15 3
start_sim
key 3 1.0; key 3 0.5      # title, welcome
key 3 1.8                 # the boss ahead
key 3 1.0; key 3 0.5; key 3 0.5; key 3 0.5; key 3 0.5; key 3 0.7   # intro, then the four help cards
snap 3-duel
key 3 0.8; snap 4-skill   # R1 opens the list, on the best pick
key 1 0.6                 # L1: the next creature
key 3 1.0; snap 5-needle  # use it: the needle
stop_sim

# Run 3: the summit's finale.
write_save 4 8 17 96 31 0 31 6
start_sim
key 3 1.8                 # the title: the saved game opens on the finale
snap 6-finale
echo "previews in $OUT"
