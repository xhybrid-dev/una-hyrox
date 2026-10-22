#!/bin/bash
#
# The text-fit audit: run the simulator with XSAUNA_AUDIT=1, which hands every
# screen its worst case (99:59:59, 65535 bpm, a recovery of +32768, the longest
# phase name, 255 rounds) and checks every label of ours with LVGL's own text
# measure: inside its box, and clear of the bezel arcs (Gui/Audit.hpp). Prints a
# line per screen and exits 1 if anything overflows. Needs Xvfb. Build the
# simulator first. Takes about 10 seconds.
#
# Usage:  UNA_SDK=/path/to/una-sdk ./audit.sh
set -u
HERE=$(cd "$(dirname "$0")" && pwd)
BIN="$HERE/../../Software/Apps/LVGL-GUI/simulator/build/bin"
DISP=:95

[ -x "$BIN/XSaunaSimulator" ] || { echo "Build the simulator first ($BIN)" >&2; exit 1; }

Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
trap 'kill $XPID 2>/dev/null' EXIT
sleep 2

cd "$BIN" || exit 1
OUT=$(DISPLAY=$DISP XSAUNA_AUDIT=1 timeout 60 ./XSaunaSimulator 2>&1)
STATUS=$?
echo "$OUT" | grep -E "AUDIT|Audit::check"
exit $STATUS
