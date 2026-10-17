#!/bin/bash
#
# Capture Trail Quest's screens in the PC simulator, cut to the watch's round
# face, and print what the service and the GUI logged (beat judgements, the
# frame-time line, the LVGL pool).
#
# Xvfb for a display, xdotool for the buttons (keys 1-4 are L1, L2, R1, R2),
# ImageMagick for the frames and the round mask. Build the simulator first.
#
# Q0 run: the probe screen at rest, a press on the beat, the next feedback
# pattern, and a second press.
#
# Usage:  UNA_SDK=/path/to/una-sdk ./capture_screens.sh [out-dir]
#   out-dir defaults to docs/screens (files q0-*.png).
set -u
REPO=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:-$REPO/docs/screens}
BIN="$REPO/Software/Apps/LVGL-GUI/simulator/build/bin"
DISP=:97
LOG=${SIM_LOG:-/tmp/trailquest-capture.log}

[ -x "$BIN/TrailQuestSimulator" ] || { echo "Build the simulator first ($BIN)" >&2; exit 1; }
mkdir -p "$OUT"
WORK=$(mktemp -d)

Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
SIMPID=""
cleanup() { [ -n "$SIMPID" ] && kill -9 $SIMPID 2>/dev/null; kill $XPID 2>/dev/null; rm -rf "$WORK"; }
trap cleanup EXIT
sleep 2

: > "$LOG"
( cd "$BIN" && exec env DISPLAY=$DISP stdbuf -oL -eL ./TrailQuestSimulator >> "$LOG" 2>&1 ) & SIMPID=$!
sleep 3
WID=$(DISPLAY=$DISP xdotool search --name 'Trail Quest' | tail -1)
[ -n "$WID" ] || { echo "FATAL: no simulator window" >&2; exit 1; }

key()  { DISPLAY=$DISP xdotool key "$1"; sleep "${2:-0.6}"; }
snap() {
    DISPLAY=$DISP timeout 10 import -window "$WID" "$WORK/raw.png" 2>/dev/null
    local w; w=$(identify -format %w "$WORK/raw.png")
    convert "$WORK/raw.png" \( -size ${w}x${w} xc:black -fill white \
        -draw "circle $((w/2)).5,$((w/2)).5 $((w/2)).5,0" \) \
        -alpha off -compose CopyOpacity -composite "$OUT/$1.png"
    echo "  $1"
}

snap q0-1-start
key 3 1.2; snap q0-2-press-1
key 2 0.3; key 1 0.3; snap q0-3-next-pattern
sleep 0.9; key 3 1.2; snap q0-4-press-2
key 4 1

echo "=== log ==="
grep -E "beat|render|pool|key '" "$LOG" | sed 's/^ *[0-9]* //; s/^/  /' | head -40
