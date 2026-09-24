#!/bin/bash
#
# Drive the HybridX Streak design demo through every scenario and moment and
# capture each screen, as hybridx-race's capture_screens.sh does: Xvfb for a
# display, xdotool for the four buttons (keys 1..4 = L1, L2, R1, R2),
# ImageMagick's `import` for the frame.
#
# Each capture is saved twice:
#   raw/<name>.png    the simulator's 480x480 square frame (2x the watch)
#   <name>.png        the same, cut to the watch's round face on a dark bezel,
#                     because the simulator has no circular mask and the
#                     corners it shows are not on the watch.
#
# With RECORD=1 the whole run is also filmed (ffmpeg x11grab) and saved as
# <output-dir>/streak-demo.mp4, behind the same round bezel.
#
# Usage:  UNA_SDK=/path/to/una-sdk [RECORD=1] ./capture_screens.sh [output-dir]
# Build the simulator first (a HYBRIDXSTREAK_DEMO build, the default).
set -u

APP=$(cd "$(dirname "$0")/../.." && pwd)
BIN="$APP/Software/Apps/LVGL-GUI/simulator/build/bin"
OUT=${1:-$APP/docs/screens}
DISP=:97

if [ ! -x "$BIN/HybridXStreakSimulator" ]; then
    echo "No simulator at $BIN -- build it first." >&2
    exit 1
fi
mkdir -p "$OUT/raw"

Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
sleep 2
cd "$BIN"
DISPLAY=$DISP ./HybridXStreakSimulator > /tmp/hybridx-streak-capture.log 2>&1 & SIMPID=$!
sleep 4

WID=$(DISPLAY=$DISP xdotool search --name 'HybridX Streak' | head -1)
if [ -z "$WID" ]; then
    echo "FATAL: no simulator window on $DISP -- is another capture running?" >&2
    kill $SIMPID $XPID 2>/dev/null; exit 1
fi

# The round face: a dark bezel with a transparent 480 px hole, laid over the
# frame. (An opacity mask on the frame itself does not survive these indexed
# PNGs; an overlay does.)
BEZEL=$(mktemp --suffix=.png)
convert -size 520x520 xc:'#1b1f22' -fill black -draw "circle 259.5,259.5 259.5,20" -transparent black \
    -fill none -stroke '#3a4044' -strokewidth 5 -draw "circle 259.5,259.5 259.5,17" "$BEZEL"

if [ "${RECORD:-0}" = 1 ]; then
    eval "$(DISPLAY=$DISP xdotool getwindowgeometry --shell "$WID")"
    ffmpeg -y -loglevel error -f x11grab -draw_mouse 0 -framerate 20 -video_size "${WIDTH}x${HEIGHT}" -i "$DISP+$X,$Y" \
        -c:v libx264 -preset veryfast -pix_fmt yuv420p "$OUT/raw/demo.mp4" & FFPID=$!
    sleep 1
fi

key() { DISPLAY=$DISP xdotool key "$1"; sleep "${2:-1}"; }
snap() {
    local raw="$OUT/raw/$1.png"
    DISPLAY=$DISP import -window "$WID" "$raw" 2>/dev/null
    local size
    size=$(stat -c %s "$raw" 2>/dev/null || echo 0)
    [ "$size" -lt 500 ] && echo "WARN: $1 captured $size bytes" >&2
    convert "$raw" -background black -gravity center -extent 520x520 "$BEZEL" -composite "$OUT/$1.png"
}

# 1. First week (trial): a run arrives.
snap 01-first-week
key 3 0.6; snap 02-first-week-session
sleep 2.5; snap 03-first-week-after

# 2. Mid-week on Snowdon: the session that banks the week.
key 2 1; snap 04-mid-week
key 3 0.5; snap 05-session-toast
sleep 1.2; snap 06-week-complete-glide
sleep 1.3; snap 07-week-complete-burst
sleep 2.5; snap 08-week-banked

# 3. Ben Nevis, week already banked: a bonus ride.
key 2 1; snap 09-week-banked-scenario
key 3 0.6; snap 10-bonus-session
sleep 2.5

# 4. Mont Blanc, at risk.
key 2 1; snap 11-at-risk

# 5. Summit day on Ben Nevis.
key 2 1; snap 12-summit-day
key 3 0.4
sleep 3.6; snap 13-summit-screen
sleep 1.2; snap 14-summit-confetti
key 3 1.2; snap 15-next-climb

# 6. Everest, the long climb.
key 2 1.2; snap 16a-everest

# 7. A missed week: spend a shield.
key 2 1.2; snap 16-shield-offer
key 3 0.6; snap 17-shield-spent
sleep 2.6; snap 18-after-shield

# 8. Fresh start (scenario 8), with the sun rising.
key 2 0.4; snap 19-fresh-start-dawn
sleep 2; snap 20-fresh-start
key 3 1; snap 21-new-streak

# The other way out of the shield screen: decline it.
key 1 1.2; snap 22-shield-again
key 4 1.2; snap 23-shield-declined

if [ "${RECORD:-0}" = 1 ]; then
    kill -INT $FFPID 2>/dev/null; wait $FFPID 2>/dev/null
    ffmpeg -y -loglevel error -i "$OUT/raw/demo.mp4" -i "$BEZEL" \
        -filter_complex "[0:v]pad=520:520:20:20:black[p];[p][1:v]overlay=0:0,format=yuv420p" \
        -c:v libx264 -crf 30 -preset slow -movflags +faststart "$OUT/streak-demo.mp4"
fi

DISPLAY=$DISP xdotool key Escape; sleep 1
kill $SIMPID 2>/dev/null; kill $XPID 2>/dev/null
rm -f "$BEZEL"
echo "Captured to $OUT"
grep "LVGL pool" /tmp/hybridx-streak-capture.log | sort -t'k' -k2 | tail -3
