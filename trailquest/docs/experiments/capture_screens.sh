#!/bin/bash
#
# Capture Trail Quest's screens in the PC simulator, cut to the watch's round
# face: a scripted walk through stage 1 (Q2), with a fixed seed so the same
# spirits turn up each time.
#
# Xvfb for a display, xdotool for the buttons (keys 1-4 are L1, L2, R1, R2),
# ImageMagick for the frames and the round mask. Build the simulator first.
# The save in Software/Output is cleared first, so the run starts from nothing.
#
# The timing beat: a strike 1.3 s after choosing lands in the perfect zone
# here (the needle starts when the button's click code arrives, about 0.2 s
# after it goes down), so the scripted runner plays well.
#
# Usage:  ./capture_screens.sh [out-dir]
#   out-dir defaults to docs/screens (files q2-*.png).
set -u
REPO=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:-$REPO/docs/screens}
BIN="$REPO/Software/Apps/LVGL-GUI/simulator/build/bin"
SAVES="$REPO/Software/Output"
DISP=:97
LOG=${SIM_LOG:-/tmp/trailquest-capture.log}

[ -x "$BIN/TrailQuestSimulator" ] || { echo "Build the simulator first ($BIN)" >&2; exit 1; }
mkdir -p "$OUT"
rm -f "$SAVES"/save.txt* "$SAVES"/settings.txt*
WORK=$(mktemp -d)

Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
SIMPID=""
cleanup() { [ -n "$SIMPID" ] && kill -9 $SIMPID 2>/dev/null; kill $XPID 2>/dev/null; rm -rf "$WORK"; }
trap cleanup EXIT
sleep 2

: > "$LOG"
( cd "$BIN" && exec env DISPLAY=$DISP TRAILQUEST_SEED=${SEED:-7} stdbuf -oL -eL ./TrailQuestSimulator >> "$LOG" 2>&1 ) & SIMPID=$!
sleep 3
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
steps()  { grep -ac 'step:' "$LOG"; }
last()   { grep -a 'step:' "$LOG" | tail -1; }
# Wait (up to 15 s) for the service to log another step.
await()  { local n=$1; for _ in $(seq 1 75); do [ "$(steps)" -gt "$n" ] && return 0; sleep 0.2; done; return 1; }
# One duel turn with the skill (R1) and a well-timed strike; waits for the
# playback to finish (a turn's events take up to about 5 s).
turn()   { local n; n=$(steps); key 3 1.3; key 3 0.2; await "$n"; sleep 5; }
# Fight on with the skill until the duel is over (cause 6, "duel over").
fight()  { for _ in $(seq 1 14); do last | grep -q 'cause 6 ' && return 0; turn; done; }

snap q2-01-title
key 3 1.2;          snap q2-02-stage-card
key 3;              snap q2-03-map
key 1;              snap q2-04-map-other-way
key 1
key 3 1.6;          snap q2-05-duel-intro          # the woods path: a spirit
key 3;              snap q2-06-duel-choose
key 4;              snap q2-07-duel-fuel
key 4
key 3 0.5;          snap q2-08-duel-beat          # a capture takes about 0.35 s
sleep 0.45; key 3 0.25; snap q2-09-duel-verdict
sleep 1.4;          snap q2-10-duel-playback
sleep 4
fight
sleep 0.5;          snap q2-11-duel-won
key 3;              snap q2-12-offer
key 3 1.0                                           # yes: it joins
key 2;              snap q2-13-pack-you
key 3;              snap q2-14-pack-partners
key 3;              snap q2-15-pack-bag
key 2
key 3 1.6;          snap q2-16-hut                  # on to the hut
key 3
key 3 1.6;          snap q2-17-cache                # the old track to the old gate
key 3
key 3 1.6                                           # the foot of the hill: a spirit
key 3
fight
key 3 1.0
last | grep -q 'phase 3 ' && key 2 1.0              # an offer: no, thanks
key 1;              snap q2-18-map-to-the-bog
key 3 1.6;          snap q2-19-obstacle
key 3 0.5;          snap q2-20-obstacle-beat
sleep 0.45; key 3 1.0; snap q2-21-obstacle-result
key 3
key 3 1.6;          snap q2-22-boss-intro
key 3;              snap q2-23-boss-choose
fight
sleep 0.5;          snap q2-24-boss-beaten
key 3 1.2;          snap q2-25-stage-clear
DISPLAY=$DISP xdotool keydown 4; sleep 0.9; snap q2-26-leaving; DISPLAY=$DISP xdotool keyup 4

echo "=== steps ==="
grep -a 'step:' "$LOG" | sed 's/^.*step: /  /'
echo "=== beats, pool ==="
grep -aE 'beat:|LVGL pool' "$LOG" | sed 's/^ *[0-9]* //; s/^/  /' | sort | uniq -c | sort -rn | head -20
