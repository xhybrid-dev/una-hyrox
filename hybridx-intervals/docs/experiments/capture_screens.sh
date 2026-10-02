#!/bin/bash
#
# Walk HybridX Intervals through its screens in the PC simulator and capture
# each: the start screen, the workout list (every kind of entry), a preview,
# the countdown, then a run of "Sim short" on the simulator's stadium track
# (warm-up, the 200 m reps with their pace target, the cues, the rests, the
# open cool-down ended with R2), and saving it.
#
# Needs Xvfb, xdotool and ImageMagick. Keys 1..4 are L1, L2, R1, R2. The
# captures are the simulator's 2x frames (480 x 480), masked round like the
# watch face. As Trail's capture_screens.sh.
#
# Usage:  UNA_SDK=/path/to/una-sdk ./capture_screens.sh [output-dir] [stop-after]
#   stop-after: "list" to stop before the run (a quick look at the menus).
set -u
REPO=$(cd "$(dirname "$0")/../.." && pwd)
SOFT="$REPO/Software"
BIN="$SOFT/Apps/LVGL-GUI/simulator/build/bin"
OUT=${1:-$REPO/docs/screens}
STOP=${2:-}
DISP=:93
LOG=${SIM_LOG:-/tmp/hybridx-intervals-capture.log}

[ -x "$BIN/HybridXIntervalsSimulator" ] || { echo "Build the simulator first ($BIN)" >&2; exit 1; }
mkdir -p "$OUT"

# A pretend watch: the test workouts, none chosen yet, and one with a long,
# non-ASCII name, for the name fitting (the watch fonts are ASCII).
rm -rf "$SOFT/Output"
mkdir -p "$SOFT/Output/Workouts"
cp "$REPO"/Tools/TestWorkouts/*.json "$SOFT/Output/Workouts/"
sed 's|"name": "6 x 400 m"|"name": "Llyn y Fan – hill reps"|' \
    "$REPO/Tools/TestWorkouts/6 x 400 m.json" > "$SOFT/Output/Workouts/long-name.json"

Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
sleep 2
cd "$BIN"
DISPLAY=$DISP stdbuf -oL -eL ./HybridXIntervalsSimulator > "$LOG" 2>&1 & SIMPID=$!
cleanup() { kill -9 $SIMPID 2>/dev/null; kill $XPID 2>/dev/null; }
trap cleanup EXIT
sleep 8

WID=$(DISPLAY=$DISP xdotool search --name 'HybridX Intervals' | head -1)
[ -n "$WID" ] || { echo "FATAL: no simulator window" >&2; exit 1; }
key()  { DISPLAY=$DISP xdotool key "$1"; sleep "${2:-1.5}"; }
hold() { DISPLAY=$DISP xdotool keydown "$1"; sleep "$2"; DISPLAY=$DISP xdotool keyup "$1"; sleep 1.5; }
# Wait up to $2 seconds for $1 in the service log, after line $3.
waitlog() { for _ in $(seq "$2"); do tail -n +"${3:-1}" "$LOG" | grep -q "$1" && return 0; sleep 1; done; echo "timed out: $1" >&2; }
mark() { wc -l < "$LOG"; }
snap() {
    DISPLAY=$DISP import -window "$WID" "$OUT/$1.png" 2>/dev/null
    # Round, like the watch: black outside the display's circle.
    local w h
    read -r w h < <(identify -format "%w %h" "$OUT/$1.png")
    convert "$OUT/$1.png" \( -size "${w}x${h}" xc:black -fill white \
        -draw "circle $((w / 2)),$((h / 2)) $((w / 2)),0" \) \
        -alpha off -compose CopyOpacity -composite -background black -alpha remove -alpha off "$OUT/$1.png"
    echo "captured $1"
}

snap 01-start
key 2;            snap 02-start-intervals-none    # L2: Intervals, no workout chosen yet
key 3;            snap 03-list-no-workout         # R1: the list, on "No workout"
key 2;            snap 04-list-5x1km              # sorted by name
key 2;            snap 05-list-6x400
key 2;            snap 06-list-broken             # a file that can't be read: in red
key 2;            snap 07-list-long-name          # folded to ASCII and fitted
key 2;            snap 08-list-sim-short
key 2
key 2;            snap 09-list-bike               # Zone 2 spin: bike, not yet
key 2;            snap 10-list-add
key 1; key 1; key 1; key 1; key 1; key 1         # back up to 6 x 400 m
key 3 3;          snap 11-preview-6x400           # R1: its preview (chooses it)
key 2; key 2; key 2
                  snap 12-preview-6x400-scrolled  # DOWN scrolls the steps
key 4 3                                           # R2: back, nothing chosen again
key 2; key 2; key 2                               # to Sim short
key 3 3;          snap 13-preview-sim-short
key 4 3
key 4 2;          snap 14-start-intervals-none-again
[ "$STOP" = "list" ] && exit 0

# The run: Sim short (20 s warm-up, 3 x (200 m at 4:00-4:20/km, 20 s rest),
# open cool-down). The simulated runner is far too fast for that band.
key 3                                             # R1: the list
key 2; key 2; key 2; key 2; key 2                 # to Sim short
key 3 3                                           # R1: its preview (chooses it)
key 3 1;          snap 15-countdown               # R1: start: the countdown
M=$(mark)
sleep 6;          snap 16-warm-up                 # the warm-up first, so the run face
waitlog "Lap_1 saved" 40 "$M"
sleep 1;          snap 17-alert-first-rep         # the first rep's alert: 200 m, its target
sleep 6;          snap 18-rep-settling            # back on the run face: white while settling
M=$(mark)
waitlog "over target" 40 "$M"
sleep 0.5;        snap 19-cue-slow-down           # the cue: a banner over every face
sleep 5;          snap 20-rep-over                # pace in red: too fast
key 2;            snap 21-face-total              # DOWN: RunLVGL's other faces
key 2;            snap 22-face-lap
key 2;            snap 23-face-status
key 2                                             # and round to the intervals face
M=$(mark)
waitlog "Lap_2 saved" 60 "$M"
sleep 1;          snap 24-alert-rest              # the rest's alert
sleep 6;          snap 25-rest
M=$(mark)
waitlog "Lap_7 saved" 200 "$M"
sleep 7;          snap 26-cool-down               # the open cool-down, counting up
key 4 1;          snap 27-completed               # R2: ends it: "Workout completed"
sleep 6;          snap 28-after-workout           # the normal faces again, still recording
key 3;            snap 29-action-menu             # R1: pause, action menu
key 2; key 2
hold 3 3;         snap 30-saved                   # hold R1 on Save
sleep 3;          snap 31-summary

echo "--- service log"
grep -E "Workouts:|Workout \"|Intervals:|Lap_[0-9]* saved|Track stopped" "$LOG" | sed 's/^ *[0-9]* -.- //'
echo "--- LVGL pool, highest peak"
grep -o "peak [0-9]*%" "$LOG" | sort -k2 -n | tail -1
