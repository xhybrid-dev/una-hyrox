#!/bin/bash
#
# Run HybridX Trail in the PC simulator with one of the simulator routes
# (Tools/TestRoutes/make_sim_routes.py), start an activity, let the simulated
# runner lap its stadium track for a while, then stop and save, and report
# what the service logged: the route, the navigation events, the FIT file.
#
# The SDK simulator has no screenshot capability of its own, so this drives
# it from outside: Xvfb for a display, xdotool for the buttons (keys 1..4 are
# L1, L2, R1, R2), ImageMagick's import for frames. Build the simulator first.
#
# Usage:  UNA_SDK=/path/to/una-sdk ./sim_run.sh <route.gpx> <seconds> [shots-dir]
#   e.g.  ./sim_run.sh sim-wrong-turn.gpx 80
set -u
ROUTE=${1:-sim-wrong-turn.gpx}
SECS=${2:-80}
REPO=$(cd "$(dirname "$0")/../.." && pwd)
SOFT="$REPO/Software"
BIN="$SOFT/Apps/LVGL-GUI/simulator/build/bin"
SHOTS=${3:-}
DISP=:94
LOG=/tmp/hybridx-trail-sim.log

[ -x "$BIN/HybridXTrailSimulator" ] || { echo "Build the simulator first ($BIN)" >&2; exit 1; }

# A clean pretend watch: the simulator routes, and this one chosen.
rm -rf "$SOFT/Output"
mkdir -p "$SOFT/Output/Routes"
python3 "$REPO/Tools/TestRoutes/make_sim_routes.py" "$SOFT/Output/Routes" >/dev/null
printf "%s" "$ROUTE" > "$SOFT/Output/route.sel"

Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
sleep 2
cd "$BIN"
DISPLAY=$DISP ./HybridXTrailSimulator > "$LOG" 2>&1 & SIMPID=$!
cleanup() { kill -9 $SIMPID 2>/dev/null; kill $XPID 2>/dev/null; }
trap cleanup EXIT
sleep 8                                   # the simulated GPS finds a fix after ~5 s

WID=$(DISPLAY=$DISP xdotool search --name 'HybridX Trail' | head -1)
[ -n "$WID" ] || { echo "FATAL: no simulator window" >&2; exit 1; }
key()  { DISPLAY=$DISP xdotool key "$1"; sleep "${2:-1}"; }
hold() { DISPLAY=$DISP xdotool keydown "$1"; sleep "$2"; DISPLAY=$DISP xdotool keyup "$1"; sleep 1; }
snap() { [ -n "$SHOTS" ] && mkdir -p "$SHOTS" && DISPLAY=$DISP import -window "$WID" "$SHOTS/$1.png" 2>/dev/null; return 0; }

snap 01-start
key 3 2                                    # R1: start the activity
snap 02-running
sleep "$SECS"
snap 03-after
key 3 1                                    # R1: action menu (pauses)
key 2; key 2                               # L2 twice: Save
hold 3 3                                   # hold R1: save
sleep 3
snap 04-saved

echo "--- route and navigation (service log)"
grep -E "Route:|Navigation:" "$LOG"
echo "--- activity files"
find "$SOFT/Output" -name "*.fit" -printf "%p %s bytes\n"
