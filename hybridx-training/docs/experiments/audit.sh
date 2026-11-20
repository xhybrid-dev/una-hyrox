#!/bin/bash
#
# The text-fit audit (TP3): run the simulator with XTRAINING_AUDIT=1, which hands the
# plan's screens their worst cases (every kind of session with the widest "what" the
# three plans make at any level, paces as wide as a 59:59 5K's per mile, the longest
# problems, notes and Saved lines) and checks every label of ours with LVGL's own text
# measure: inside its box, clear of the bezel arcs, and not cut with an ellipsis
# (Gui/Audit.hpp). Prints a line per screen and exits 1 if anything fails. Needs Xvfb.
# Build the simulator first. Takes about 50 seconds. As HybridX Workout's.
#
# Usage:  UNA_SDK=/path/to/una-sdk ./audit.sh
set -u
HERE=$(cd "$(dirname "$0")" && pwd)
BIN="$HERE/../../Software/Apps/LVGL-GUI/simulator/build/bin"
DISP=:97

[ -x "$BIN/HybridXTrainingSimulator" ] || { echo "Build the simulator first ($BIN)" >&2; exit 1; }

Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
trap 'kill $XPID 2>/dev/null' EXIT
sleep 2

cd "$BIN" || exit 1
OUT=$(DISPLAY=$DISP XTRAINING_AUDIT=1 timeout 150 ./HybridXTrainingSimulator 2>&1)
STATUS=$?
echo "$OUT" | grep -E "AUDIT|Audit::check"
exit $STATUS
