#!/bin/bash
# The Energy Probe in the simulator, by key presses: every page, R1 on Tonight, Night window
# and Daytime, and a whole 2-minute sensor check, with screenshots (x-energy/docs/NOTES.md,
# "Gate SL0"). About 2.5 minutes. Needs Xvfb, xdotool and ImageMagick, and the simulator built:
#   cmake -S x-energy/Tools/Probe/Software/Apps/LVGL-GUI/simulator -B <that>/build -G Ninja
# Usage: capture_screens.sh [out-dir]   (WAIT=125 the check, AFTER=3, DISP=92 the X display)
set -u
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"   # x-energy/
SIM="$ROOT/Tools/Probe/Software/Apps/LVGL-GUI/simulator/build/bin"
OUT=${1:-$ROOT/docs/screens}
mkdir -p "$OUT"
rm -rf "$ROOT/Tools/Probe/Software/Output"   # the simulator's watch: starts empty
Xvfb :${DISP:-92} -screen 0 800x600x24 >/dev/null 2>&1 &
XPID=$!
sleep 1
export DISPLAY=:${DISP:-92}
cd "$SIM"
./EnergyProbeSimulator > "$OUT/sim.log" 2>&1 &
SPID=$!
sleep 4
WID=$(xdotool search --name "Energy Probe" | head -1)
shot() { timeout 10 import -window "$WID" "$OUT/$1.png"; }
key() { timeout 5 xdotool key --window "$WID" "$1"; sleep 0.8; }
shot p1-now
key 2; shot p2-nights
key 2; shot p3-tonight
key 3; sleep 1; shot p3-tonight-r1
key 2; shot p4-window
key 3; sleep 1; shot p4-window-r1
key 3; key 3; key 3; sleep 1
key 2; shot p5-daytime
key 2; shot p6-check
key 3; sleep 2; shot p6-check-running
sleep ${WAIT:-125}
shot p6-check-done
key 3; shot p6-check-more
key 2; shot p1-now-again
key 4; sleep 2
echo "GUI closed; service should keep logging" >> "$OUT/sim.log"
sleep ${AFTER:-70}
kill $SPID 2>/dev/null; sleep 1; kill -9 $SPID 2>/dev/null
kill $XPID 2>/dev/null
