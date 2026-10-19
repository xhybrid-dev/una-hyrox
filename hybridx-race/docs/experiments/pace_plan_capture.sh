#!/bin/bash
#
# Drive the simulator through a pace plan. Race 1 (a Half, rounds 1 to 4, every segment 6 s) is run
# and saved; race 2 is then run against it, fast at first (ahead of plan) and then slow (behind),
# and the screens are captured. Same method as capture_screens.sh: Xvfb, xdotool for the four
# buttons (keys 1..4 are L1, L2, R1, R2), ImageMagick's `import` for the frame.
#
# Usage:  UNA_SDK=/path/to/una-sdk ./pace_plan_capture.sh [output-dir] [last|target]
#   last   (default)  race 2 follows "Last race": each segment's target is race 1's time, 6 s.
#   target            race 2 follows "Target", with a 1-minute target set the way the phone sets
#                     it (the values file): race 1's shape scaled from 48 s to 60 s, so 7.5 s a
#                     segment.
#
# Build the simulator first. About three minutes: the split lockout is 3 s, so every press waits.
set -u

REPO=$(cd "$(dirname "$0")/../.." && pwd)
BIN="$REPO/Software/Apps/LVGL-GUI/simulator/build/bin"
OUT=${1:-$REPO/docs/screens}
PLAN=${2:-last}
DISP=:97

[ -x "$BIN/HybridXRaceSimulator" ] || { echo "No simulator at $BIN -- build it first." >&2; exit 1; }

mkdir -p "$OUT"
rm -rf "$REPO/Software/Output"          # start from no saved race
mkdir -p "$REPO/Software/Output"
# The phone's side of the Target plan: a target finish of 1 minute (the field is in minutes).
printf '{"schema":1,"values":{"targetFinishMin":1}}' > "$REPO/Software/Output/app_config.json"

Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
sleep 2
cd "$BIN"
DISPLAY=$DISP ./HybridXRaceSimulator > /tmp/hybridx-pace.log 2>&1 & SIMPID=$!
sleep 5

WID=$(DISPLAY=$DISP xdotool search --name 'HybridX Race' | head -1)
if [ -z "$WID" ]; then
    echo "FATAL: no simulator window on $DISP -- is another capture running?" >&2
    kill $SIMPID $XPID 2>/dev/null; exit 1
fi

key()   { DISPLAY=$DISP xdotool key "$1"; sleep "${2:-1}"; }
snap()  { DISPLAY=$DISP import -window "$WID" "$OUT/$1.png" 2>/dev/null; }
split() { DISPLAY=$DISP xdotool key 4; }

# --- Race 1: a Half A of eight 6 s segments, saved. It is the plan for race 2. ------------------
snap pace-01-main
key 2; key 3 1                                       # Format row, cycle to Half: 1 to 4
key 2; snap pace-02-pace-row-no-saved-race           # Pace plan row: nothing saved yet
key 3 1; snap pace-03-pace-row-press-does-nothing    # Off is the only usable mode
key 1; key 1                                         # back to Start race
key 3 1; key 3 2                                     # start, confirm
for _ in $(seq 1 8); do sleep 6; split; done
sleep 3; DISPLAY=$DISP xdotool key 3; sleep 3        # save
snap pace-04-summary
key 4 2                                              # leave the summary

# --- Choose the plan ---------------------------------------------------------------------------
key 2; key 2; snap pace-05-pace-row-off              # Format, Pace plan
key 3 1; snap pace-06-pace-row-last-race
key 3 1; snap pace-07-pace-row-target
key 3 1; snap pace-08-pace-row-off-again
key 3 1                                              # Last race
[ "$PLAN" = target ] && key 3 1                      # Target
snap pace-09-chosen
key 1; key 1                                         # back to Start race

# --- Race 2: ahead of plan, then behind ---------------------------------------------------------
# Each snapshot waits 3 s after a split: the toast that confirms the split stays up for about 2 s.
key 3 1; key 3 2                                     # start, confirm
sleep 2; snap pace-10-race-start                     # 2 s into the first segment
for _ in $(seq 1 3); do sleep 4; split; done         # 4 s a segment: ahead of 6 s (or 7.5 s)
sleep 3; snap pace-11-race-ahead                     # 3 s into the next segment, toast gone
sleep 5; snap pace-12-race-segment-over-target       # 8 s into it: past its target
split                                                # close it
sleep 13; split                                      # a 13 s segment: well behind
sleep 3; snap pace-13-race-behind
DISPLAY=$DISP xdotool key 1; sleep 1; snap pace-14-status-face-projection   # status face (L1)

kill $SIMPID 2>/dev/null; sleep 1; kill $XPID 2>/dev/null
echo "captured to $OUT"
