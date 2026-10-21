#!/bin/bash
#
# Capture R1's pick list in a boss duel in the PC simulator: a hand-written
# save at the foot of the Ram King (stage 3) with a pack of three (Red Fox,
# Brown Bear, Ruby Beetle). It goes through the boss page and the help cards
# to the choice, opens the list with R1, steps through each creature's move
# with L1, and uses one (the needle). The save in Software/Output is replaced.
#
# Xvfb, xdotool and ImageMagick. Build the simulator first.
# Usage:  ./capture_skill.sh [out-dir]
set -u
REPO=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:-$(mktemp -d)}
BIN="$REPO/Software/Apps/LVGL-GUI/simulator/build/bin"
SAVES="$REPO/Software/Output"
DISP=:95
LOG=${SIM_LOG:-/tmp/trailquest-skill.log}

[ -x "$BIN/TrailQuestSimulator" ] || { echo "Build the simulator first ($BIN)" >&2; exit 1; }
mkdir -p "$OUT" "$SAVES"
WORK=$(mktemp -d)
rm -f "$SAVES"/save.txt* "$SAVES"/settings.txt*
cat > "$SAVES/save.txt" <<EOT
tq=1
rng=12345
lv=10
xp=0
st=76
pc=3
p0=0
p1=1
p2=2
bag=3,1,1,0,0
stage=2
node=6
hut=3
min=540
wx=0
lap=0
boss=0
cache=0
met=0
beat=0
won=0
lost=0
perf=0
offer=255
EOT

Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
SIMPID=""
cleanup() { [ -n "$SIMPID" ] && kill -9 $SIMPID 2>/dev/null; kill $XPID 2>/dev/null; rm -rf "$WORK"; }
trap cleanup EXIT
sleep 2
: > "$LOG"
( cd "$BIN" && exec env DISPLAY=$DISP TRAILQUEST_SEED=7 stdbuf -oL -eL ./TrailQuestSimulator >> "$LOG" 2>&1 ) & SIMPID=$!
for _ in $(seq 1 100); do grep -aq 'GUI is now running' "$LOG" && break; sleep 0.2; done
sleep 2
WID=$(DISPLAY=$DISP xdotool search --name 'Trail Quest' | tail -1)
[ -n "$WID" ] || { echo "FATAL: no simulator window" >&2; exit 1; }
key()  { DISPLAY=$DISP xdotool key "$1"; sleep "${2:-0.7}"; }
snap() {
    DISPLAY=$DISP timeout 10 import -window "$WID" "$WORK/raw.png" 2>/dev/null
    local w; w=$(identify -format %w "$WORK/raw.png")
    convert "$WORK/raw.png" \( -size ${w}x${w} xc:black -fill white \
        -draw "circle $((w/2)).5,$((w/2)).5 $((w/2)).5,0" \) \
        -alpha off -compose CopyOpacity -composite "$OUT/$1.png"
    echo "  $1"
}
key 3 1.0; key 3 0.5                   # past the title and "welcome back"
key 3 1.8                              # the boss ahead (first sight)
key 3 1.0; key 3 0.5; key 3 0.5; key 3 0.5; key 3 0.5; key 3 0.7   # intro, then the four help cards
snap skill-0-choose
key 3 0.8; snap skill-1-list-best      # R1 opens the list, on the best pick
key 1 0.6; snap skill-2-next
key 1 0.6; snap skill-3-next
key 1 0.6; snap skill-4-next
key 1 0.6; snap skill-5-wrapped        # round to the best again
key 4 0.6; snap skill-6-back           # R2: back to the choice
key 3 0.8; key 1 0.6; key 3 1.0; snap skill-7-needle   # open, next, use: the needle
grep -a 'step:' "$LOG" | sed 's/^.*step: /  /' | tail -3
echo "frames in $OUT"
