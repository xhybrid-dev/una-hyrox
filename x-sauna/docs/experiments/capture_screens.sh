#!/bin/bash
#
# Walk every screen of X Sauna in the PC simulator by key presses, capture
# each, and decode the activity file the session writes.
#
# Xvfb for a display, xdotool for the buttons (keys 1-4 are L1, L2, R1, R2),
# ImageMagick's import for frames, fitdecode (pip) for the file. Build the
# simulator first.
#
# Session 1: the main menu and its Mode and Rest choices (Just sauna locks
# Rest); start Sauna + plunge with Rest; the three faces; laps, with the lap
# toast; the action menu, a pause and a resume; End session by holding R1
# (a frame mid-hold); Saved; the summary pages; back to the menu.
# Session 2: Discard by holding R1; Discarded; the menu; exit on R2.
# The simulator has no temperature sensor, so the service makes one up
# (Core/Header/SimTemp.hpp), and its heart rate is random. Then the file the
# first session wrote is decoded.
#
# Usage:  UNA_SDK=/path/to/una-sdk ./capture_screens.sh [out-dir]
#   out-dir defaults to docs/screens (files sa3-*.png). Takes about 90 seconds.
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

key()  { DISPLAY=$DISP xdotool key "$1"; sleep "${2:-1.0}"; }   # 1 s: a wheel slide takes 0.4 s, and Xvfb is slow
snap() { DISPLAY=$DISP timeout 10 import -window "$WID" "$OUT/sa3-$1.png" 2>/dev/null; echo "  $1"; }
down() { DISPLAY=$DISP xdotool keydown "$1"; }
up()   { DISPLAY=$DISP xdotool keyup "$1"; sleep 0.6; }

# -- Session 1 ---------------------------------------------------------------
snap 01-main-start
key 2; snap 02-main-mode
key 3; snap 03-main-just-sauna          # R1 on Mode: Just sauna
key 2; snap 04-main-rest-locked         # Rest is always on, greyed
key 1; key 3                            # back to Mode, back to Sauna + plunge
key 2; key 3; snap 05-main-rest-on      # Rest on
key 1; key 1; snap 06-main-start-rest   # Start row, hint with rest
key 3 1.5                               # start
sleep 4
snap 07-live-sauna
BEFORE=$(grep -c "Backlight::on" "$LOG")
DISPLAY=$DISP xdotool key 5; sleep 0.8       # the simulator's wrist raise
AFTER=$(grep -c "Backlight::on" "$LOG")
[ "$AFTER" -gt "$BEFORE" ] && echo "  ok   a wrist raise lights the backlight" || echo "  FAIL a wrist raise did not light the backlight"
key 2; snap 08-live-totals
key 2; snap 09-live-clock
key 2; sleep 0.3                        # round to the main face
key 4 0.7; snap 10-lap-toast-plunge     # R2: lap
sleep 3; snap 11-live-plunge
key 4 0.7; snap 12-lap-toast-rest
sleep 3.5
key 4 0.7; snap 13-lap-toast-sauna2
sleep 6; snap 14-live-sauna2            # recovery counting down
key 3 1; snap 15-action-resume
key 2; snap 16-action-pause
key 3 1.5                               # pause
snap 17-action-resume-timer
key 4 1; snap 18-live-paused
key 3 1; key 2; key 3 1.5               # menu (opens on Resume), Pause row: resume
key 4 1
key 3 1; key 2; key 2; snap 19-action-end
down 3; sleep 0.8; snap 20-hold-mid; sleep 1.0; up 3
sleep 0.4; snap 21-saved
sleep 2.6; snap 22-summary-overview
key 2; snap 23-summary-time
key 2; snap 24-summary-heart
key 2 1; snap 25-summary-laps-1
key 2 1; snap 26-summary-laps-2
key 4 1; snap 27-main-after

# -- Session 2: discard ---------------------------------------------------------
key 3 1.5
sleep 3
key 3 1; key 2; key 2; key 2; snap 28-action-discard
down 3; sleep 2.0; up 3
sleep 0.4; snap 29-discarded
sleep 2.6; snap 30-main-after-discard
key 4 3

kill -0 $SIMPID 2>/dev/null && echo "simulator still running after R2" || echo "simulator exited on R2"
grep -E "Session (started|saved|discarded)|Recovered|Failed|NO LOG|ERROR" "$LOG" | head -20

FITS=$(find "$FS/Activity" -name '*.fit')
[ -n "$FITS" ] || { echo "FATAL: no .fit under $FS/Activity" >&2; exit 1; }
[ "$(echo "$FITS" | wc -l)" -eq 1 ] || { echo "FAIL: expected one .fit (the discarded session writes none)" >&2; echo "$FITS" >&2; }
FIT=$(echo "$FITS" | head -1)
echo "activity: $FIT ($(wc -c < "$FIT") bytes)"
SIDE="${FIT%.fit}.json"
[ -f "$SIDE" ] && echo "sidecar: $(cat "$SIDE")" || echo "FAIL: no .json sidecar"
cp "$FIT" "$OUT/sa3-session.fit"
python3 -I "$HERE/fit_check.py" "$FIT" sim
