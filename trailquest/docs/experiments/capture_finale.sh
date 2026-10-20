#!/bin/bash
#
# Capture the summit's finale and the start of the second lap in the PC
# simulator, from a hand-written save at the beaten Summit Wolf (stage 5,
# place 8, every boss beaten). R1 picks the save up, which lands on the
# finale; R1 again ("Again") starts lap 2 at the Valley Path (its story pages,
# then the map). The save in Software/Output is replaced.
#
# Xvfb, xdotool and ImageMagick. Build the simulator first.
# Usage:  ./capture_finale.sh [out-dir]
set -u
REPO=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:-$(mktemp -d)}
BIN="$REPO/Software/Apps/LVGL-GUI/simulator/build/bin"
SAVES="$REPO/Software/Output"
DISP=:96
LOG=${SIM_LOG:-/tmp/trailquest-finale.log}

[ -x "$BIN/TrailQuestSimulator" ] || { echo "Build the simulator first ($BIN)" >&2; exit 1; }
mkdir -p "$OUT" "$SAVES"
WORK=$(mktemp -d)
rm -f "$SAVES"/save.txt* "$SAVES"/settings.txt*
cat > "$SAVES/save.txt" <<EOT
tq=1
rng=12345
lv=17
xp=0
st=96
pc=3
p0=0
p1=10
p2=16
bag=2,1,0,0,0
stage=4
node=8
hut=3
min=1700
wx=3
lap=${LAP:-0}
boss=31
cache=0
met=0
beat=0
won=${WON:-31}
lost=${LOST:-6}
perf=40
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
snap finale-0-title
key 3 1.5; snap finale-1-welcome
key 3 1.5; snap finale-2-summit        # the finale
key 3 2.0; snap finale-3-lap2-story    # "Again": lap 2's first story page
key 3 0.7; key 3 0.7; key 3 0.7        # the stage's two pages
snap finale-4-lap2-map
grep -a 'step:' "$LOG" | sed 's/^.*step: /  /'
echo "frames in $OUT"
