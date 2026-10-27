#!/bin/bash
#
# The store previews for Knitting: four simulator screens, 480x480, written to
# Resources/previews/ (the folder the packager puts in the zip).
#
# A made-up project is seeded first (6 rows, Set 3, 22 rows in all) so the
# numbers look like a real one. The four screens:
#   1-counting     the counter as it is opened
#   2-undo         holding R1: "Hold to undo", the bar part-way
#   3-reset        holding L2: "Hold to reset", the bar part-way
#   4-next-set     after L1: Set 4, no rows yet
#
# The simulator's keys: 1-4 are L1, L2, R1, R2; a key held down is a press held
# (hold_check.sh explains). Xvfb for a display, xdotool for the buttons,
# ImageMagick's import for frames. Build the simulator first.
#
# Usage:  UNA_SDK=/path/to/una-sdk ./store_previews.sh [out-dir]
#   out-dir defaults to Resources/previews.
set -u
REPO=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:-$REPO/Resources/previews}
BIN="$REPO/Software/Apps/LVGL-GUI/simulator/build/bin"
APPDIR="$REPO/Software/Output"
DISP=:94
LOG=${SIM_LOG:-/tmp/knitting-previews.log}

[ -x "$BIN/KnittingSimulator" ] || { echo "Build the simulator first ($BIN)" >&2; exit 1; }
mkdir -p "$OUT"
rm -f "$OUT"/*.png
rm -rf "$APPDIR"; mkdir -p "$APPDIR"
printf 'knitting 1\nrows=6\nset=3\ntotal=22\n' > "$APPDIR/state.txt"

Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
SIMPID=""
cleanup() { [ -n "$SIMPID" ] && kill -9 $SIMPID 2>/dev/null; kill $XPID 2>/dev/null; rm -rf "$APPDIR"; }
trap cleanup EXIT
sleep 2
: > "$LOG"

( cd "$BIN" && exec env DISPLAY=$DISP stdbuf -oL -eL ./KnittingSimulator >> "$LOG" 2>&1 ) & SIMPID=$!
sleep 4
WID=$(DISPLAY=$DISP xdotool search --name 'Knitting' | tail -1)
[ -n "$WID" ] || { echo "FATAL: no simulator window" >&2; exit 1; }

snap() { DISPLAY=$DISP timeout 10 import -window "$WID" "$OUT/$1.png" 2>/dev/null; echo "  $1"; }

snap 1-counting

# Hold R1 part-way and let go: it is a slow tap, so it counts a row (7, total 23).
DISPLAY=$DISP xdotool keydown 3; sleep 0.45
snap 2-undo
DISPLAY=$DISP xdotool keyup 3; sleep 1.9

# Hold L2 part-way and let go: nothing is reset, a hint shows.
DISPLAY=$DISP xdotool keydown 2; sleep 0.4
snap 3-reset
DISPLAY=$DISP xdotool keyup 2; sleep 1.9

# L1: the next set.
DISPLAY=$DISP xdotool keydown 1; sleep 0.1; DISPLAY=$DISP xdotool keyup 1; sleep 0.6
snap 4-next-set
