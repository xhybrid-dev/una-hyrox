#!/bin/bash
#
# Run HybridX Intervals in the PC simulator with the test workouts
# (Tools/TestWorkouts), choose one, start an intervals activity, let the
# simulated runner lap its stadium track, then stop and save, and report what
# the service logged: the workouts it found, each step, the cues, the FIT file.
#
# As Trail's sim_run.sh: Xvfb for a display, xdotool for the buttons (keys
# 1..4 are L1, L2, R1, R2), ImageMagick's import for frames. Build the
# simulator first. The simulated runner does 15-25 km/h, so a 4:00-4:20/km
# pace band reads "too fast" and cues.
#
# Usage:  UNA_SDK=/path/to/una-sdk ./sim_run.sh [workout.json] [seconds] [shots-dir]
#   e.g.  ./sim_run.sh sim-short.json 200
set -u
WORKOUT=${1:-sim-short.json}
SECS=${2:-200}
REPO=$(cd "$(dirname "$0")/../.." && pwd)
SOFT="$REPO/Software"
BIN="$SOFT/Apps/LVGL-GUI/simulator/build/bin"
SHOTS=${3:-}
DISP=:95
LOG=${SIM_LOG:-/tmp/hybridx-intervals-sim.log}

[ -x "$BIN/HybridXIntervalsSimulator" ] || { echo "Build the simulator first ($BIN)" >&2; exit 1; }

# A clean pretend watch: the test workouts, and this one chosen. (Until the
# P3c screens exist, the choice is made by writing workout.sel, as the app
# itself does when a workout is picked.)
rm -rf "$SOFT/Output"
mkdir -p "$SOFT/Output/Workouts"
cp "$REPO"/Tools/TestWorkouts/*.json "$SOFT/Output/Workouts/"
printf "%s" "$WORKOUT" > "$SOFT/Output/workout.sel"

Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
sleep 2
cd "$BIN"
DISPLAY=$DISP ./HybridXIntervalsSimulator > "$LOG" 2>&1 & SIMPID=$!
cleanup() { kill -9 $SIMPID 2>/dev/null; kill $XPID 2>/dev/null; }
trap cleanup EXIT
sleep 8                                   # the simulated GPS finds a fix after ~5 s

WID=$(DISPLAY=$DISP xdotool search --name 'HybridX Intervals' | head -1)
[ -n "$WID" ] || { echo "FATAL: no simulator window" >&2; exit 1; }
key()  { DISPLAY=$DISP xdotool key "$1"; sleep "${2:-1}"; }
hold() { DISPLAY=$DISP xdotool keydown "$1"; sleep "$2"; DISPLAY=$DISP xdotool keyup "$1"; sleep 1; }
snap() { [ -n "$SHOTS" ] && mkdir -p "$SHOTS" && DISPLAY=$DISP import -window "$WID" "$SHOTS/$1.png" 2>/dev/null; return 0; }

snap 01-start
key 2                                      # L2: to Intervals
key 3                                      # R1: the intervals menu
key 3 6                                    # R1: Start (then the countdown)
snap 02-running
sleep "$SECS"
snap 03-after
key 4 2                                    # R2: next step (ends an open cool-down)
key 3 1                                    # R1: action menu (pauses)
key 2; key 2                               # L2 twice: Save
hold 3 3                                   # hold R1: save
sleep 3
snap 04-saved

echo "--- workouts and steps (service log)"
grep -E "Workouts|Workout \"|Intervals:|can't be read|phase change|Lap_" "$LOG" | sed 's/^ *//'
echo "--- activity files"
find "$SOFT/Output" -name "*.fit" -printf "%p %s bytes\n"
