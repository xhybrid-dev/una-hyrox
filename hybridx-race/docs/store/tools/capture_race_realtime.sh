#!/bin/bash
# Store screenshots from a real-time full race in the simulator (about an hour).
set -u
OUT=$1; DISP=:94
REPO=/home/user/una-hyrox/hybridx-race; BIN=$REPO/Software/Apps/LVGL-GUI/simulator/build/bin
mkdir -p "$OUT"; rm -rf "$REPO/Software/Output"
Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XP=$!; sleep 2; cd "$BIN"
DISPLAY=$DISP stdbuf -oL ./HybridXRaceSimulator > "$OUT/sim.log" 2>&1 & SP=$!
for i in $(seq 1 40); do WID=$(DISPLAY=$DISP xdotool search --name 'HybridX Race' 2>/dev/null|head -1); [ -n "$WID" ] && break; sleep 0.5; done
[ -z "$WID" ] && { echo "no window"; kill $SP $XP; exit 1; }
sleep 3
k(){ DISPLAY=$DISP xdotool key $1; sleep ${2:-0.8}; }
s(){ DISPLAY=$DISP import -window $WID "$OUT/$1.png" 2>/dev/null; echo "$(date +%T) $1" >> "$OUT/progress"; }
split(){ DISPLAY=$DISP xdotool key 4; }
s 01-main; k 3 1.2; s 02-on-your-marks; k 3 1
#    R1  SKI  R2  PUSH R3 PULL R4 BBJ  R5 ROW  R6 FARM R7 LUNG R8 WB
SEG=(248 222 256 128 261 166 259 208 263 226 266 96 270 199 268 262)
for i in $(seq 0 15); do
  t=${SEG[$i]}
  case $i in
    4)  sleep 134; s 03-race-run; sleep $((t-134)) ;;
    5)  sleep 1; s 04-split-toast; sleep 95; s 05-race-station; sleep $((t-96)) ;;
    8)  sleep 70; k 3 0.6; s 06-action-menu; k 4 0.5; sleep $((t-71)) ;;
    10) sleep 80; k 1 0.6; s 07-status; k 1 0.5; sleep $((t-81)) ;;
    *)  sleep $t ;;
  esac
  split
done
sleep 1.5; s 08-finished
k 3 0.4; s 09-saved; sleep 2.5; s 10-summary
k 2 0.8; s 11-splits-1; k 2 0.8; s 12-splits-2; k 2 0.8; s 13-splits-3; k 2 0.8; s 14-splits-4
sleep 1; kill $SP 2>/dev/null; sleep 1; kill $XP
echo done >> "$OUT/progress"
