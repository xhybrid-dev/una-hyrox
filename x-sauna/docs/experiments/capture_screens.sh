#!/bin/bash
#
# Capture the heat probe in the PC simulator and check its buttons and its log.
#
# Xvfb for a display, xdotool for the buttons (keys 1-4 are L1, L2, R1, R2),
# ImageMagick's import for frames. Build the simulator first.
#
# The simulator has no temperature sensor, so the service makes a reading up
# (Core/Header/SimTemp.hpp: a room for 30 s, a climb to 85 over 5 minutes).
# The run: the start screen; two markers (R1) at about 10 s; a frame during
# the climb at about 75 s; clear the range (L1); exit (R2). Then the log is
# checked: a header, a run line, a sensors note, rows once a second, the two
# markers in the mark column, and a closing note.
#
# Usage:  UNA_SDK=/path/to/una-sdk ./capture_screens.sh [out-dir]
#   out-dir defaults to docs/screens (files sa0-*.png). Takes about 90 seconds.
set -u
REPO=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:-$REPO/docs/screens}
BIN="$REPO/Software/Apps/LVGL-GUI/simulator/build/bin"
FS="$REPO/Software/Output"
DISP=:97
LOG=${SIM_LOG:-/tmp/x-sauna-capture.log}

[ -x "$BIN/XSaunaSimulator" ] || { echo "Build the simulator first ($BIN)" >&2; exit 1; }
mkdir -p "$OUT"

# A clean log, so the run below is run 1.
find "$FS" -name heat-probe.csv -delete 2>/dev/null

Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
SIMPID=""
cleanup() { [ -n "$SIMPID" ] && kill -9 $SIMPID 2>/dev/null; kill $XPID 2>/dev/null; }
trap cleanup EXIT
sleep 2

: > "$LOG"
( cd "$BIN" && exec env DISPLAY=$DISP stdbuf -oL -eL ./XSaunaSimulator >> "$LOG" 2>&1 ) & SIMPID=$!
sleep 4
WID=$(DISPLAY=$DISP xdotool search --name 'X Sauna' | tail -1)
[ -n "$WID" ] || { echo "FATAL: no simulator window" >&2; exit 1; }

key()  { DISPLAY=$DISP xdotool key "$1"; sleep "${2:-0.4}"; }
snap() { DISPLAY=$DISP timeout 10 import -window "$WID" "$OUT/$1.png" 2>/dev/null; echo "  $1"; }

snap sa0-1-start
sleep 6
key 3 1; key 3 1          # two markers
snap sa0-2-two-marks
sleep 62                  # into the climb
snap sa0-3-climbing
key 1 2                   # clear the range
snap sa0-4-range-cleared
key 4 3

kill -0 $SIMPID 2>/dev/null && echo "simulator still running after R2" || echo "simulator exited on R2"

CSV=$(find "$FS" -name heat-probe.csv | head -1)
[ -n "$CSV" ] || { echo "FATAL: no heat-probe.csv under $FS" >&2; exit 1; }
echo "log: $CSV ($(wc -l < "$CSV") lines)"
head -6 "$CSV"
echo "..."
tail -3 "$CSV"
echo "rows with a mark: first at t_s=$(awk -F, '$1 ~ /^[0-9]+$/ && $3 >= 1 {print $2; exit}' "$CSV"), last mark value $(awk -F, '$1 ~ /^[0-9]+$/ {m=$3} END{print m}' "$CSV")"
