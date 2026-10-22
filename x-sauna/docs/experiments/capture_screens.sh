#!/bin/bash
#
# Run a whole sauna session in the PC simulator by key presses, capture each
# view, and decode the activity file it writes.
#
# Xvfb for a display, xdotool for the buttons (keys 1-4 are L1, L2, R1, R2),
# ImageMagick's import for frames, fitdecode (pip) for the file. Build the
# simulator first.
#
# The run: the menu; Rest on (L2); start Sauna + plunge (R1); about 6 s of
# sauna; R1 (plunge); R1 (rest); pause (L1) and resume; hold R2 (the stop
# question); save (R1); the summary; exit (R2). The simulator has no
# temperature sensor, so the service makes one up (Core/Header/SimTemp.hpp), and
# its heart rate is random. Then the file is decoded: sauna, plunge and rest
# laps, the pause as a gap, the developer fields, the .json sidecar.
#
# Usage:  UNA_SDK=/path/to/una-sdk ./capture_screens.sh [out-dir]
#   out-dir defaults to docs/screens (files sa2-*.png). Takes about 50 seconds.
set -u
HERE=$(cd "$(dirname "$0")" && pwd)
REPO=$(cd "$HERE/../.." && pwd)
OUT=${1:-$REPO/docs/screens}
BIN="$REPO/Software/Apps/LVGL-GUI/simulator/build/bin"
FS="$REPO/Software/Output"
DISP=:97
LOG=${SIM_LOG:-/tmp/x-sauna-capture.log}

[ -x "$BIN/XSaunaSimulator" ] || { echo "Build the simulator first ($BIN)" >&2; exit 1; }
mkdir -p "$OUT"
rm -rf "$FS/Activity"

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

key()  { DISPLAY=$DISP xdotool key "$1"; sleep "${2:-0.5}"; }
snap() { DISPLAY=$DISP timeout 10 import -window "$WID" "$OUT/$1.png" 2>/dev/null; echo "  $1"; }
hold() { DISPLAY=$DISP xdotool keydown "$1"; sleep "$2"; DISPLAY=$DISP xdotool keyup "$1"; sleep 0.6; }

snap sa2-1-menu
key 2; snap sa2-2-menu-rest-on
key 3 1.5                       # start: Sauna + plunge, with rest
sleep 5
snap sa2-3-sauna
key 3 1; sleep 3                # plunge
snap sa2-4-plunge
key 3 1; sleep 2                # rest
key 1 1; sleep 3                # pause
snap sa2-5-paused
key 1 1; sleep 3                # resume
hold 4 0.4                      # a tap on R2 only hints
snap sa2-6-hint
hold 4 1.7                      # hold R2: the stop question
snap sa2-7-confirm
key 3 2                         # save
snap sa2-8-done
key 4 3

kill -0 $SIMPID 2>/dev/null && echo "simulator still running after R2" || echo "simulator exited on R2"
grep -E "Session (started|saved)|Recovered|Failed|NO LOG" "$LOG" | head

FIT=$(find "$FS/Activity" -name '*.fit' | head -1)
[ -n "$FIT" ] || { echo "FATAL: no .fit under $FS/Activity" >&2; exit 1; }
echo "activity: $FIT ($(wc -c < "$FIT") bytes)"
SIDE="${FIT%.fit}.json"
[ -f "$SIDE" ] && echo "sidecar: $(cat "$SIDE")" || echo "FAIL: no .json sidecar"
cp "$FIT" "$OUT/sa2-session.fit"
python3 -I "$HERE/fit_check.py" "$FIT" sim
