#!/bin/bash
#
# Check the two holds in the PC simulator: hold R1 to undo a row, hold L2 to
# reset, and that a tap on either does the right thing.
#
# The simulator follows the kernel (una-sdk Libs/Source/Simulator/LVGL/
# LvglHost.cpp): keys 1-4 are L1, L2, R1, R2. Pressing one sends the press
# code, and letting go sends the click code (only if it was down for under
# 500 ms) and then the release code. So a tap, a slow press and a hold are all
# just "key down, wait, key up" here, as on the watch.
#
# Steps, with what must be true after each (read from state.txt):
#   1. tap L2                      -> a hint only; nothing changes
#   2. five taps on R1             -> 5 rows
#   3. hold L2 0.6 s, let go       -> prompt and bar, then cancelled: still 5
#   4. hold R1 1.1 s               -> undo fires at 0.8 s: 4 rows, total 4,
#                                     and nothing more is counted on letting go
#   5. tap R1                      -> 5 rows
#   6. hold R1 0.65 s              -> a slow press, no click is sent: it still
#                                     counts, on the release: 6 rows
#   7. hold L2 1.9 s               -> reset fires at 1.5 s: 0 rows, Set 1, total 0
#   8. close (R2), reopen          -> still 0, Set 1
#
# Xvfb for a display, xdotool for the buttons, ImageMagick's import for
# frames. Build the simulator first.
#
# Usage:  UNA_SDK=/path/to/una-sdk ./hold_check.sh [out-dir]
#   out-dir defaults to docs/screens (files k2-*.png).
set -u
REPO=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:-$REPO/docs/screens}
BIN="$REPO/Software/Apps/LVGL-GUI/simulator/build/bin"
APPDIR="$REPO/Software/Output"
DISP=:95
LOG=${SIM_LOG:-/tmp/knitting-hold.log}
FAIL=0

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
    kill -0 $SIMPID 2>/dev/null && kill -9 $SIMPID
    wait $SIMPID 2>/dev/null; SIMPID=""
    for _ in 1 2 3 4 5 6 7 8 9 10; do
        DISPLAY=$DISP xdotool search --name 'Knitting' >/dev/null 2>&1 || break
        sleep 0.5
    done
}
key()  { DISPLAY=$DISP xdotool key "$1"; sleep "${2:-0.3}"; }
down() { DISPLAY=$DISP xdotool keydown "$1"; }
up()   { DISPLAY=$DISP xdotool keyup "$1"; sleep "${2:-0.3}"; }
tap()  { down "$1"; sleep 0.1; up "$1" 0.3; }
snap() { DISPLAY=$DISP timeout 10 import -window "$WID" "$OUT/$1.png" 2>/dev/null; echo "  $1"; }
state() { [ -f "$APPDIR/state.txt" ] && tr '\n' ' ' < "$APPDIR/state.txt" | sed 's/knitting 1 //'; }
expect() {   # expect "<what>" "<rows> <set> <total>"
    local got; got=$(state | sed 's/rows=//; s/set=//; s/total=//; s/ *$//')
    if [ "$got" = "$2" ]; then echo "  ok   $1: $got"; else echo "  FAIL $1: wanted '$2', got '$got'"; FAIL=1; fi
}

start_sim
snap k2-1-start

echo "1. tap L2: a hint, nothing changes"
tap 2; snap k2-2-tap-hint
sleep 1.4
expect "after a tap on L2 (nothing saved yet)" ""

echo "2. five taps on R1"
for _ in 1 2 3 4 5; do tap 3; done
sleep 0.5; expect "five rows" "5 1 5"

echo "3. hold L2 0.6 s then let go"
down 2; sleep 0.6; snap k2-3-reset-hold-bar
up 2 0.4; snap k2-3b-reset-hold-cancelled
sleep 1.4; expect "a short L2 hold changed nothing" "5 1 5"

echo "4. hold R1 1.1 s (undo fires at 0.8 s)"
down 3; sleep 0.4; snap k2-4-undo-hold-bar
sleep 0.7; snap k2-5-undone
up 3 0.4
expect "undo took one row back and nothing was added on release" "4 1 4"

echo "5. tap R1"
tap 3; sleep 0.4; expect "a tap counts again" "5 1 5"

echo "6. a slow press on R1 (0.65 s, so no click is sent)"
down 3; sleep 0.65; up 3 0.4
sleep 0.4; expect "a slow press counts, once" "6 1 6"

echo "7. hold L2 1.9 s (reset fires at 1.5 s)"
down 2; sleep 0.7; snap k2-6-reset-mid
sleep 1.2; snap k2-7-reset-done
up 2 0.4
sleep 0.4; expect "reset" "0 1 0"
snap k2-8-after-reset

echo "8. close and reopen"
close_sim
start_sim
snap k2-9-reopened
expect "still reset after reopening" "0 1 0"
close_sim

echo "service log (warnings, errors):"
grep -E -- "-W-|-E-" "$LOG" | grep -v "DualAppComm::clear" | head
[ $FAIL -eq 0 ] && echo "ALL OK" || { echo "FAILED"; exit 1; }
