#!/bin/bash
#
# A guided walkthrough video of the HybridX Streak design demo: the simulator
# driven through every scenario, filmed, cut to the watch's round face, with a
# caption under the face saying what is happening and why.
#
# Same tools as capture_screens.sh: Xvfb, xdotool (keys 1..4 = L1, L2, R1, R2),
# ffmpeg (x11grab to film, drawtext for the captions).
#
# Usage:  ./walkthrough.sh [output.mp4]
# Build the design-demo simulator first:
#   cmake -S Software/Apps/LVGL-GUI/simulator -B Software/Apps/LVGL-GUI/simulator/build-demo -DHYBRIDXSTREAK_DEMO=ON
#   cmake --build Software/Apps/LVGL-GUI/simulator/build-demo
set -u

APP=$(cd "$(dirname "$0")/../.." && pwd)
BIN="$APP/Software/Apps/LVGL-GUI/simulator/build-demo/bin"
OUT=${1:-$APP/docs/screens/streak-walkthrough.mp4}
DISP=:95
FONT=/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf
FONT_BOLD=/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf

if [ ! -x "$BIN/HybridXStreakSimulator" ]; then
    echo "No simulator at $BIN -- build it first." >&2
    exit 1
fi
WORK=$(mktemp -d)

Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
sleep 2
cd "$BIN"
DISPLAY=$DISP ./HybridXStreakSimulator > "$WORK/sim.log" 2>&1 & SIMPID=$!
sleep 4
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

wait_s 1
cap "Every activity your watch records,
from any app, counts towards a
weekly target. Here: 3 a week."
wait_s 4.5
cap "It's week one: a free trial week.
Each ring is a session to do.
Arthur's Seat is the first climb."
wait_s 4.5
cap "You record a run in any app.
Open Streak: it has been counted.
A buzz, and the first ring fills."
key 3; wait_s 4

key 2
cap "Weeks later, on Snowdon.
A 7-week streak, 2 of 3 done.
The coach says what's left."
wait_s 4.5
cap "A strength session completes
the week: the climber moves one
step up. Lime = achieved."
key 3; wait_s 6.5

key 2
cap "Target already hit this week?
Extra sessions still count,
as blue bonus rings."
key 3; wait_s 4.5

key 2
cap "Running out of days, the coach
turns amber: 2 more in 2 days.
No nagging, just the numbers."
wait_s 4.5

key 2
cap "One week from the top of
Ben Nevis. Watch what happens
when this week completes..."
wait_s 3
key 3; wait_s 4.2
cap "Summit! Confetti, a waving flag
and a big buzz. Then the next,
taller mountain: Mont Blanc."
wait_s 4
key 3; wait_s 2.5

key 2
cap "The climbs: Arthur's Seat,
Snowdon, Ben Nevis, Mont Blanc,
Everest. 104 weeks to the top."
wait_s 5
cap "Progress counts every week you
have ever hit, so a missed week
never takes you back down."
wait_s 4.5

key 2
cap "Missed a week? Life happens.
You earn a shield every 4 weeks
(hold up to 2) to save a streak."
wait_s 5
cap "R1 spends one: streak saved.
Your 9 weeks climb on."
key 3; wait_s 4.5

key 2
cap "Or no shield left: a fresh start.
The streak restarts, but your
place on the mountain is kept."
wait_s 5
cap "A new streak starts today.
Every week you hit is a step up.
HybridX Streak."
key 3; wait_s 4.5

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
FILTER+="drawtext=fontfile=$FONT:text='design demo · simulator':fontcolor=0x9aa0a4:fontsize=20:x=(w-text_w)/2:y=62"
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
