#!/bin/bash
#
# A guided walkthrough of the REAL HybridX Streak app (not the design demo):
# the simulator opens onto a pretend watch -- real FIT files from several apps
# (sim_fixtures.sh) and eleven weeks of history with last week missed
# (make_state) -- and plays what the service found, then tours the menus.
# Filmed, cut to the round face, captioned.
#
# Same tools as capture_screens.sh: Xvfb, xdotool (keys 1..4 = L1, L2, R1, R2),
# ffmpeg (x11grab to film, drawtext for the captions).
#
# Usage:  ./walkthrough_real.sh [output.mp4]
# Build first: the real simulator (Software/Apps/LVGL-GUI/simulator/build) and
# the host tools (build-tests: make_fit, make_state).
set -u

APP=$(cd "$(dirname "$0")/../.." && pwd)
BIN="$APP/Software/Apps/LVGL-GUI/simulator/build/bin"
OUT=${1:-$APP/docs/screens/streak-real-walkthrough.mp4}
DISP=:93
FONT=/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf
FONT_BOLD=/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf

if [ ! -x "$BIN/HybridXStreakSimulator" ]; then
    echo "No simulator at $BIN -- build it first." >&2
    exit 1
fi
WORK=$(mktemp -d)

# The pretend watch, and a history: 11 weeks achieved (2 shields), last week missed.
TREE="$WORK/tree"
bash "$APP/docs/experiments/sim_fixtures.sh" "$TREE" >/dev/null
"$APP/build-tests/make_state" "$TREE/Output/state.json" 11 1 >/dev/null

Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
sleep 2
cd "$TREE/a/b/c/d/e"
DISPLAY=$DISP "$BIN/HybridXStreakSimulator" > "$WORK/sim.log" 2>&1 & SIMPID=$!
sleep 1
WID=$(DISPLAY=$DISP xdotool search --name 'HybridX Streak' | head -1)
if [ -z "$WID" ]; then
    echo "FATAL: no simulator window on $DISP" >&2
    kill $SIMPID $XPID 2>/dev/null; exit 1
fi
eval "$(DISPLAY=$DISP xdotool getwindowgeometry --shell "$WID")"

ffmpeg -y -loglevel error -use_wallclock_as_timestamps 1 -f x11grab -draw_mouse 0 -framerate 25 -video_size "${WIDTH}x${HEIGHT}" \
    -i "$DISP+$X,$Y" -c:v libx264 -preset ultrafast -pix_fmt yuv420p -fps_mode vfr "$WORK/raw.mp4" & FFPID=$!
T0=$(date +%s.%N)

# cap "<text>": the caption from now until the next cap (or the end).
N=0
STARTS=()
cap() {
    STARTS+=("$(echo "$(date +%s.%N) - $T0" | bc)")
    printf '%s' "$1" > "$WORK/cap_$N.txt"
    N=$((N + 1))
}
key() { DISPLAY=$DISP xdotool key "$1"; }
wait_s() { sleep "$1"; }

cap "Open HybridX Streak. It reads the
activities every app on the watch
recorded, and judges past weeks."
wait_s 2.6
cap "Last week fell short: 1 of 3.
You have earned 2 shields.
Spend one to keep 11 weeks?"
wait_s 4
key 3
cap "R1: streak saved. Then this
week's news, found in three
different apps' files."
wait_s 3.4
cap "A run from Running, a hybrid
session from HybridX Race, a ride
from Cycling: all counted."
wait_s 6
cap "Three of three: week complete,
one step up the mountain... and
that step is the top of Snowdon."
wait_s 4.5
key 3; wait_s 1.5
cap "Next, Ben Nevis. R1 (or L1/L2)
opens the menu."
key 3; wait_s 2
cap "This week: each session, where
it came from, and why it counts.
A 6-minute walk is under 10."
key 3; wait_s 2; key 2; wait_s 1.2; key 2; wait_s 1.2; key 2; wait_s 2
key 4; wait_s 1.2; key 2; wait_s 1; key 2; wait_s 1.2
cap "The trophy case: the summits,
badges for 10, 50, 100 and 250
sessions, and your bests."
key 3; wait_s 2; key 2; wait_s 1.5; key 2; wait_s 1.5
key 4; wait_s 1.2; key 2; wait_s 1.2
cap "Settings: target, week start,
what counts, shortest session.
Also editable from the phone."
key 3; wait_s 2.5; key 2; wait_s 1.5; key 2; wait_s 1.5; key 2; wait_s 2
key 4; wait_s 1; key 4; wait_s 2.5

END=$(echo "$(date +%s.%N) - $T0" | bc)
kill -INT $FFPID 2>/dev/null; wait $FFPID 2>/dev/null
DISPLAY=$DISP xdotool key Escape; sleep 1
kill $SIMPID 2>/dev/null; kill $XPID 2>/dev/null

# The round face, as capture_screens.sh draws it.
convert -size 520x520 xc:'#1b1f22' -fill black -draw "circle 259.5,259.5 259.5,20" -transparent black \
    -fill none -stroke '#3a4044' -strokewidth 5 -draw "circle 259.5,259.5 259.5,17" "$WORK/bezel.png"

# 720x1040 portrait: a header, the face at 640 px, the caption below it.
FILTER="[0:v]pad=520:520:20:20:black[p];[p][1:v]overlay=0:0,scale=640:640:flags=lanczos,"
FILTER+="pad=720:1040:40:78:color=0x1b1f22,"
FILTER+="drawtext=fontfile=$FONT_BOLD:text='HybridX Streak':fontcolor=0xaaff00:fontsize=34:x=(w-text_w)/2:y=22,"
FILTER+="drawtext=fontfile=$FONT:text='real app · simulator · pretend watch':fontcolor=0x9aa0a4:fontsize=20:x=(w-text_w)/2:y=62"
for ((i = 0; i < N; i++)); do
    FROM=${STARTS[$i]}
    if [ $((i + 1)) -lt $N ]; then TO=${STARTS[$((i + 1))]}; else TO=$END; fi
    FILTER+=",drawtext=fontfile=$FONT:textfile=$WORK/cap_$i.txt:expansion=none:fontcolor=white:fontsize=31"
    FILTER+=":line_spacing=12:text_align=C:x=(w-text_w)/2:y=760:enable='between(t,$FROM,$TO)'"
done
FILTER+=",fps=25,format=yuv420p"

ffmpeg -y -loglevel error -i "$WORK/raw.mp4" -i "$WORK/bezel.png" -filter_complex "$FILTER" \
    -c:v libx264 -crf 26 -preset slow -movflags +faststart "$OUT"
echo "Walkthrough: $OUT ($(echo "$END" | cut -d. -f1) s, $N captions)"
rm -rf "$WORK"
