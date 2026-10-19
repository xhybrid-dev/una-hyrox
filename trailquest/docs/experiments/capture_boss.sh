#!/bin/bash
#
# Capture a stage's boss fight in the PC simulator: its page, its duel, the
# warning card on a turn it tells, and the twist's line in the playback. It
# starts from a hand-written save at the place before the boss (place 6, whose
# first exit leads there in every stage), so it needs no walk through the stage.
#
# Xvfb, xdotool and ImageMagick. Build the simulator first. The save in
# Software/Output is replaced.
#
# Usage:  ./capture_boss.sh [stage 0-4] [out-dir]
#   stage defaults to 1 (the Great Stag, who charges); 2 is the Ram King (a
#   guard); 0 the Old Tortoise (a guard on every 4th turn, which a good fight
#   may not reach). out-dir defaults to a temporary folder, printed.
#   LEVEL_ADJ (default 1) sets the runner's level against the stage's par.
set -u
REPO=$(cd "$(dirname "$0")/../.." && pwd)
STAGE=${1:-1}
OUT=${2:-$(mktemp -d)}
BIN="$REPO/Software/Apps/LVGL-GUI/simulator/build/bin"
SAVES="$REPO/Software/Output"
DISP=:97
LOG=${SIM_LOG:-/tmp/trailquest-boss.log}

[ -x "$BIN/TrailQuestSimulator" ] || { echo "Build the simulator first ($BIN)" >&2; exit 1; }
mkdir -p "$OUT" "$SAVES"
WORK=$(mktemp -d)

# The runner at the stage's par level plus one, no partners, a few gels.
PAR=(2 5 9 12 15)
LEVEL=$(( ${PAR[$STAGE]} + ${LEVEL_ADJ:-1} ))   # LEVEL_ADJ=-2: a weak runner, so the fight outlasts the tell
STAMINA=$(( 40 + 4 * (LEVEL - 1) ))
rm -f "$SAVES"/save.txt* "$SAVES"/settings.txt*
cat > "$SAVES/save.txt" <<EOF
tq=1
rng=12345
lv=$LEVEL
xp=0
st=$STAMINA
pc=0
p0=255
p1=255
p2=255
bag=3,1,0,0,0
stage=$STAGE
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
EOF

Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
SIMPID=""
cleanup() { [ -n "$SIMPID" ] && kill -9 $SIMPID 2>/dev/null; kill $XPID 2>/dev/null; rm -rf "$WORK"; }
trap cleanup EXIT
sleep 2

: > "$LOG"
( cd "$BIN" && exec env DISPLAY=$DISP TRAILQUEST_SEED=${SEED:-7} stdbuf -oL -eL ./TrailQuestSimulator >> "$LOG" 2>&1 ) & SIMPID=$!
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
steps() { grep -ac 'step:' "$LOG"; }
last()  { grep -a 'step:' "$LOG" | tail -1; }
await() { local n=$1; for _ in $(seq 1 75); do [ "$(steps)" -gt "$n" ] && return 0; sleep 0.2; done; return 1; }

# Play: R1 on the title, past "welcome back", go to the boss, its page, the
# intro and the help pages.
key 3 1.0; snap boss-0-welcome
key 3 0.5
key 3 1.8; snap boss-1-page        # the boss ahead (first sight)
key 3 1.0; snap boss-2-intro
key 3 0.5; key 3 0.5; key 3 0.5    # the three help pages
snap boss-3-choose

TELLS=0
SNAPPED=""
for _ in $(seq 1 14); do
    last | grep -q 'cause 6 ' && break
    n=$(steps); t=$(grep -ac 'tell:' "$LOG")
    if [ "$t" -gt "$TELLS" ]; then
        TELLS=$t
        [ -z "$SNAPPED" ] && snap boss-4-tell
        key 3 0.6
        key 3 1.3; key 3 0.2
        if [ -z "$SNAPPED" ]; then sleep 1.7; snap boss-5-twist; sleep 1.2; snap boss-6-twist-next; SNAPPED=1; fi
    else
        key 3 1.3; key 3 0.2
    fi
    await "$n"; sleep 5
done
snap boss-7-end
echo "=== tells ==="
grep -a 'tell:' "$LOG" | sed 's/^ *[0-9]* //'
echo "=== steps ==="
grep -a 'step:' "$LOG" | sed 's/^.*step: /  /'
echo "frames in $OUT"
