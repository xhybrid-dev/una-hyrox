#!/bin/bash
#
# Drive the simulator through the whole app and capture every screen.
#
# The SDK's simulator has no screenshot capability of its own (LV_USE_SNAPSHOT
# is 0), so this drives it from outside: Xvfb for a display, xdotool for the
# four buttons, ImageMagick's `import` for the frame. The keys 1..4 are L1, L2,
# R1, R2 -- the mapping the simulator's own README documents.
#
# Usage:  UNA_SDK=/path/to/una-sdk ./capture_screens.sh [output-dir]
#
# Build the simulator first. The captures are 480x480 because the simulator
# draws at 2x; the watch is 240x240.
set -u

REPO=$(cd "$(dirname "$0")/../.." && pwd)
BIN="$REPO/Software/Apps/LVGL-GUI/simulator/build/bin"
OUT=${1:-$REPO/docs/screens}
DISP=:98

if [ ! -x "$BIN/HybridXRaceSimulator" ]; then
    echo "No simulator at $BIN -- build it first." >&2
    exit 1
fi

mkdir -p "$OUT"
rm -rf "$REPO/Software/Output"          # start from no saved race

Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
sleep 2
cd "$BIN"
DISPLAY=$DISP ./HybridXRaceSimulator > /tmp/hybridx-capture.log 2>&1 & SIMPID=$!
sleep 5

WID=$(DISPLAY=$DISP xdotool search --name 'HybridX Race' | head -1)
if [ -z "$WID" ]; then
    # A second capture on the same display kills the first one's Xvfb, and the
    # orphaned simulator then writes blank frames quite happily. Fail loudly.
    echo "FATAL: no simulator window on $DISP -- is another capture running?" >&2
    kill $SIMPID $XPID 2>/dev/null; exit 1
fi

key()  { DISPLAY=$DISP xdotool key "$1"; sleep "${2:-1}"; }
snap() {
    DISPLAY=$DISP import -window "$WID" "$OUT/$1.png" 2>/dev/null
    local size
    size=$(stat -c %s "$OUT/$1.png" 2>/dev/null || echo 0)
    [ "$size" -lt 500 ] && echo "WARN: $1 captured $size bytes" >&2
    return 0
}

snap phase4-01-main
key 2; snap phase4-02-main-format
key 2; snap phase4-03-main-lastrace
key 2; snap phase4-04-main-settings
key 3 1; snap phase4-05-settings
key 2; snap phase4-06-settings-lockout
key 4 1                                 # back to main, saving settings
key 1; key 1; key 1                     # round to "Start race"
snap phase4-07-main-again
key 3 1; snap phase4-08-start-confirm
key 3 2; snap phase4-09-race-run

# The split lockout defaults to 3 s (Settings::kLockoutDefaultSec), so every
# split press waits 4 s or the service swallows it and nothing changes.
sleep 3; DISPLAY=$DISP xdotool key 4; sleep 1; snap phase4-10-split-toast
sleep 3; snap phase4-11-race-station
key 3 1; snap phase4-12-action-menu
key 2; snap phase4-13-action-undo
key 2; snap phase4-14-action-pause
key 2; snap phase4-20-action-end
key 4 1                                 # close the menu

# Splits 2..16 of a Full race with Roxzone off: the sixteenth press finishes it.
for _ in $(seq 2 16); do sleep 4; DISPLAY=$DISP xdotool key 4; done
sleep 4; snap phase4-15-finished
DISPLAY=$DISP xdotool key 3; sleep 0.5; snap phase4-16-saved
sleep 2; snap phase4-17-summary
key 2 1; snap phase4-18-summary-splits
key 2 1; snap phase4-19-summary-splits2

kill $SIMPID 2>/dev/null; sleep 1; kill $XPID 2>/dev/null
echo "captured to $OUT"
