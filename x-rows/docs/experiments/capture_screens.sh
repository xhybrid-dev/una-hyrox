#!/bin/bash
#
# Capture the X Rows counter in the PC simulator and check the buttons.
#
# Xvfb for a display, xdotool for the buttons (keys 1-4 are L1, L2, R1, R2),
# ImageMagick's import for frames. Build the simulator first.
#
# The run: the start screen; 8 rows (R1); next set (L1); 3 more rows; a second
# L1 on a set with rows; then reset (L2). L1 on an empty set must do nothing.
#
# Usage:  UNA_SDK=/path/to/una-sdk ./capture_screens.sh [out-dir]
#   out-dir defaults to docs/screens (files k0-*.png).
set -u
REPO=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:-$REPO/docs/screens}
BIN="$REPO/Software/Apps/LVGL-GUI/simulator/build/bin"
DISP=:97
LOG=${SIM_LOG:-/tmp/x-rows-capture.log}

[ -x "$BIN/XRowsSimulator" ] || { echo "Build the simulator first ($BIN)" >&2; exit 1; }
mkdir -p "$OUT"

Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
SIMPID=""
cleanup() { [ -n "$SIMPID" ] && kill -9 $SIMPID 2>/dev/null; kill $XPID 2>/dev/null; }
trap cleanup EXIT
sleep 2

: > "$LOG"
( cd "$BIN" && exec env DISPLAY=$DISP stdbuf -oL -eL ./XRowsSimulator >> "$LOG" 2>&1 ) & SIMPID=$!
sleep 4
WID=$(DISPLAY=$DISP xdotool search --name 'X Rows' | tail -1)
[ -n "$WID" ] || { echo "FATAL: no simulator window" >&2; exit 1; }

key()  { DISPLAY=$DISP xdotool key "$1"; sleep "${2:-0.4}"; }
snap() { DISPLAY=$DISP timeout 10 import -window "$WID" "$OUT/$1.png" 2>/dev/null; echo "  $1"; }

snap k0-1-start
for _ in 1 2 3 4 5 6 7 8; do key 3; done
snap k0-2-eight-rows
key 1; snap k0-3-set-two
key 1; snap k0-3b-empty-set-ignored    # L1 again on an empty set: nothing changes
for _ in 1 2 3; do key 3; done
snap k0-4-three-in-set-two
key 2; snap k0-5-reset
key 4 1
echo "service log (counter lines):"; grep -c "GUI is now running" "$LOG"
kill -0 $SIMPID 2>/dev/null && echo "simulator still running after R2" || echo "simulator exited on R2"
