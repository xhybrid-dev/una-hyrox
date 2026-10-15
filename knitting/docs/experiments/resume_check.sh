#!/bin/bash
#
# Check in the PC simulator that Knitting resumes where it was.
#
# Run 1: 8 rows, next set, 3 rows, then R2 closes the app.
# Run 2: start again; the screen must show 3 rows, Set 2, Total 11.
#        Reset (L2), close again.
# Run 3: start again; the screen must show 0 rows, Set 1, Total 0.
# The state file's text is printed after each close. A damaged file is then
# put in place, with the backup beside it, to check the fallback.
#
# Xvfb for a display, xdotool for the buttons (keys 1-4 are L1, L2, R1, R2),
# ImageMagick's import for frames. Build the simulator first.
#
# Usage:  UNA_SDK=/path/to/una-sdk ./resume_check.sh [out-dir]
#   out-dir defaults to docs/screens (files k1-*.png).
set -u
REPO=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:-$REPO/docs/screens}
BIN="$REPO/Software/Apps/LVGL-GUI/simulator/build/bin"
# The simulator's mock file system: the app's folder is Software/Output.
APPDIR="$REPO/Software/Output"
DISP=:96
LOG=${SIM_LOG:-/tmp/knitting-resume.log}

[ -x "$BIN/KnittingSimulator" ] || { echo "Build the simulator first ($BIN)" >&2; exit 1; }
mkdir -p "$OUT"
rm -rf "$APPDIR"; mkdir -p "$APPDIR"

Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
SIMPID=""
cleanup() { [ -n "$SIMPID" ] && kill -9 $SIMPID 2>/dev/null; kill $XPID 2>/dev/null; }
trap cleanup EXIT
sleep 2
: > "$LOG"

start_sim() {
    ( cd "$BIN" && exec env DISPLAY=$DISP stdbuf -oL -eL ./KnittingSimulator >> "$LOG" 2>&1 ) & SIMPID=$!
    sleep 4
    WID=$(DISPLAY=$DISP xdotool search --name 'Knitting' | tail -1)
    [ -n "$WID" ] || { echo "FATAL: no simulator window" >&2; exit 1; }
}
close_sim() {
    DISPLAY=$DISP xdotool key 4; sleep 1.5
    kill -0 $SIMPID 2>/dev/null && { echo "  (still running after R2: killing)"; kill -9 $SIMPID; }
    wait $SIMPID 2>/dev/null; SIMPID=""
    for _ in 1 2 3 4 5 6 7 8 9 10; do
        DISPLAY=$DISP xdotool search --name 'Knitting' >/dev/null 2>&1 || break
        sleep 0.5
    done
}
key()  { DISPLAY=$DISP xdotool key "$1"; sleep "${2:-0.4}"; }
snap() { DISPLAY=$DISP timeout 10 import -window "$WID" "$OUT/$1.png" 2>/dev/null; echo "  $1"; }
show() { echo "  state.txt:"; sed 's/^/    /' "$APPDIR/state.txt" 2>/dev/null || echo "    (absent)"; }

echo "Run 1: count"
start_sim
snap k1-1-fresh
for _ in 1 2 3 4 5 6 7 8; do key 3; done
key 1
for _ in 1 2 3; do key 3; done
snap k1-2-before-close
close_sim; show

echo "Run 2: reopen (expect 3 rows, Set 2, Total 11)"
start_sim
snap k1-3-reopened
key 2; snap k1-4-reset
close_sim; show

echo "Run 3: reopen after a reset (expect 0, Set 1, Total 0)"
start_sim
snap k1-5-reopened-after-reset
key 3; key 3; key 3
close_sim; show

echo "Run 4: a damaged state file, backup beside it (expect the backup: 2 rows, 3 total)"
printf 'not a state file' > "$APPDIR/state.txt"
printf 'knitting 1\nrows=2\nset=1\ntotal=3\n' > "$APPDIR/state.txt.bak"
start_sim
snap k1-6-from-backup
close_sim
echo "service log:"; grep -i "resumed\|fresh\|backup\|could not save" "$LOG"
