#!/bin/bash
#
# Capture the timing needle's three shapes in a boss duel in the PC simulator:
# a hand-written save at the foot of the Ram King (stage 3). Surge (fast, tight),
# then Settle (slow, wide), then Surge again after the boss's warning: the
# target sits somewhere different each time. The save in Software/Output is replaced.
#
# Xvfb, xdotool and ImageMagick. Build the simulator first.
# Usage:  ./capture_beats.sh [out-dir]
set -u
REPO=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:-$(mktemp -d)}
BIN="$REPO/Software/Apps/LVGL-GUI/simulator/build/bin"
SAVES="$REPO/Software/Output"
DISP=:94
LOG=${SIM_LOG:-/tmp/trailquest-beats.log}

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
pc=0
p0=255
p1=255
p2=255
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
key 1 0.5; snap beat-1-surge           # Surge: the needle for a boss's fight
key 3 8.0                              # strike (any button), and let the turn play out
key 2 0.5; snap beat-2-settle          # Settle: slower and wider
key 3 8.0
key 3 1.0                              # the warning card (turn 3)
key 1 0.5; snap beat-3-surge-again     # Surge again: the target has moved
key 3 1.0
echo "frames in $OUT"
