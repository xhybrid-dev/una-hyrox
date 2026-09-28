#!/bin/bash
#
# Walk HybridX Trail through its screens in the PC simulator and capture each:
# the start screen, the route list, a route preview, then a run on the
# simulator's stadium track with the wrong-turn route (the maps, the
# navigation face, the off-course banner), and saving it.
#
# Needs Xvfb, xdotool and ImageMagick. Keys 1..4 are L1, L2, R1, R2.
# The captures are the simulator's 2x frames (480 x 480), masked round like
# the watch face.
#
# Usage:  UNA_SDK=/path/to/una-sdk ./capture_screens.sh [output-dir]
set -u
REPO=$(cd "$(dirname "$0")/../.." && pwd)
SOFT="$REPO/Software"
BIN="$SOFT/Apps/LVGL-GUI/simulator/build/bin"
OUT=${1:-$REPO/docs/screens}
DISP=:93
LOG=/tmp/hybridx-trail-capture.log

[ -x "$BIN/HybridXTrailSimulator" ] || { echo "Build the simulator first ($BIN)" >&2; exit 1; }
mkdir -p "$OUT"

# A pretend watch: the two simulator routes, none chosen yet.
rm -rf "$SOFT/Output"
mkdir -p "$SOFT/Output/Routes"
python3 "$REPO/Tools/TestRoutes/make_sim_routes.py" "$SOFT/Output/Routes" >/dev/null
# And a long, non-ASCII name, for the name fitting (the watch fonts are ASCII).
sed 's|<name>Stadium 3 laps</name>|<name>Llyn y Fan Fach – Picws Du ridge \&amp; back</name>|' \
    "$SOFT/Output/Routes/sim-3-laps.gpx" > "$SOFT/Output/Routes/long-name.gpx"

Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
sleep 2
cd "$BIN"
DISPLAY=$DISP stdbuf -oL -eL ./HybridXTrailSimulator > "$LOG" 2>&1 & SIMPID=$!
cleanup() { kill -9 $SIMPID 2>/dev/null; kill $XPID 2>/dev/null; }
trap cleanup EXIT
sleep 8

WID=$(DISPLAY=$DISP xdotool search --name 'HybridX Trail' | head -1)
[ -n "$WID" ] || { echo "FATAL: no simulator window" >&2; exit 1; }
key()  { DISPLAY=$DISP xdotool key "$1"; sleep "${2:-1.5}"; }
hold() { DISPLAY=$DISP xdotool keydown "$1"; sleep "$2"; DISPLAY=$DISP xdotool keyup "$1"; sleep 1.5; }
# Wait up to $2 seconds for $1 in the service log.
waitlog() { for _ in $(seq "$2"); do grep -q "$1" "$LOG" && return 0; sleep 1; done; echo "timed out: $1" >&2; }
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
key 2;            snap 02-start-route-item        # L2: Route (no route yet)
key 3;            snap 03-route-list               # R1: the list, on "No route"
key 2;            snap 04-route-list-long-name     # sorted by name: Llyn..., Stadium...
key 2;            snap 05-route-list-3-laps
key 2;            snap 06-route-list-wrong-turn
key 2;            snap 07-route-list-add
key 1; key 1; key 1                                # back up to the long name
key 3 3;          snap 08-route-preview-long-name  # R1: preview (loads it)
key 4 3                                            # R2: not this one; back to the list, on it
key 2; key 2
key 3 3;          snap 09-route-preview            # R1: preview the wrong turn (loads it)
key 3 6;          snap 10-map-300m                 # R1: start the run on it
sleep 4;          snap 11-map-300m-running
key 4;            snap 12-map-750m                 # R2 on the map: zoom
key 4;            snap 13-map-1500m
key 4;            snap 14-map-3km
key 4;            snap 15-map-whole-route
key 4                                              # round to 300 m again
key 2;            snap 16-nav                      # L2: navigation face
key 2;            snap 17-run                      # the Run face
key 2;            snap 18-lap                      # RunLVGL's lap face
key 2;            snap 19-status                   # and its status face
key 1; key 1; key 1; key 1                         # back to the map
# The runner leaves the route at the bend; the watch jumps to the map.
waitlog "Navigation: went off" 120
sleep 2;          snap 20-off-course
key 2;            snap 21-nav-off-course
waitlog "Navigation: back on" 120
sleep 1;          snap 22-back-on-course           # round the track and back onto the route
key 3;            snap 23-action-menu              # R1: pause, action menu
key 2; key 2
hold 3 3;         snap 24-saved                    # hold R1 on Save
sleep 3;          snap 25-summary

echo "--- service log"
grep -E "Route:|Navigation:" "$LOG"
echo "--- LVGL pool, highest peak"
grep -o "peak [0-9]*%" "$LOG" | sort -k2 -n | tail -1
