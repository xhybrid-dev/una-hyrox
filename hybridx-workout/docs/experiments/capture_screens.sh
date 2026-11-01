#!/bin/bash
#
# Walk every screen of HybridX Workout in the PC simulator by key presses,
# capture each, and check what the saved workout leaves behind: the .fit
# (decoded), its sidecar, act_workout.json for HybridX Streak, and the Streak
# line on the Saved screen.
#
# Xvfb for a display, xdotool for the buttons (keys 1-4 are L1, L2, R1, R2),
# ImageMagick's import for frames, fitdecode (pip) for the file. Build the
# simulator first.
#
# Before it starts, a pretend HybridX Streak summary (streak_now.json: this
# week 2 of 4 done, a 7-week streak) goes in the shared folder, and the app's
# settings are cleared.
# Workout 1: the main menu; Auto lap to 1 min; Start; the four faces; a lap by
# R2 (its toast); an auto lap a minute in (its toast); the action menu, a pause
# and a resume; End by holding R1 (a frame mid-hold); Saved, with the Streak
# line ("3 of 4 this week", "7-week streak"); the summary pages; back to the menu.
# Workout 2: Discard by holding R1; Discarded; the menu; exit on R2.
# The simulator's heart rate is random.
#
# Usage:  UNA_SDK=/path/to/una-sdk ./capture_screens.sh [out-dir]
#   out-dir defaults to docs/screens (files w0-*.png). Takes about 2.5 minutes.
set -u
HERE=$(cd "$(dirname "$0")" && pwd)
REPO=$(cd "$HERE/../.." && pwd)
OUT=${1:-$REPO/docs/screens}
BIN="$REPO/Software/Apps/LVGL-GUI/simulator/build/bin"
FS="$REPO/Software/Output"
SHARED="$REPO/Software/SharedData/HybridX"
DISP=:97
LOG=${SIM_LOG:-/tmp/hybridx-workout-capture.log}

[ -x "$BIN/HybridXWorkoutSimulator" ] || { echo "Build the simulator first ($BIN)" >&2; exit 1; }
mkdir -p "$OUT" "$SHARED"
rm -rf "$FS/Activity" "$FS/settings.json" "$FS/settings.json.bak" "$SHARED/act_workout.json" "$SHARED/act_workout.json.bak"

# This week as Streak saw it: today is in it, two sessions counted (none of ours).
python3 - "$SHARED/streak_now.json" <<'EOF'
import sys, time
today = int(time.time() // 86400)            # the simulator's local day (the container runs in UTC)
ws = today - (today + 3) % 7                 # Monday (1970-01-01 was a Thursday)
mask = 0
for d in range(min(2, today - ws + 1)):
    mask |= 1 << d
open(sys.argv[1], "w").write(
    '{"v":1,"day":%d,"ws":%d,"t":4,"min":0,"sc":255,"opd":0,"pend":0,"n":2,"days":%d,"st":7,"k":[]}' % (today, ws, mask))
EOF

Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
SIMPID=""
cleanup() { [ -n "$SIMPID" ] && kill -9 $SIMPID 2>/dev/null; kill $XPID 2>/dev/null; }
trap cleanup EXIT
sleep 2

: > "$LOG"
( cd "$BIN" && exec env DISPLAY=$DISP stdbuf -oL -eL ./HybridXWorkoutSimulator >> "$LOG" 2>&1 ) & SIMPID=$!
sleep 4
WID=$(DISPLAY=$DISP xdotool search --name 'HybridX Workout' | tail -1)
[ -n "$WID" ] || { echo "FATAL: no simulator window" >&2; exit 1; }

key()  { DISPLAY=$DISP xdotool key "$1"; sleep "${2:-1.0}"; }   # 1 s: a wheel slide takes 0.4 s, and Xvfb is slow
snap() { DISPLAY=$DISP timeout 10 import -window "$WID" "$OUT/w0-$1.png" 2>/dev/null; echo "  $1"; }
down() { DISPLAY=$DISP xdotool keydown "$1"; }
up()   { DISPLAY=$DISP xdotool keyup "$1"; sleep 0.6; }

# -- Workout 1 ---------------------------------------------------------------
snap 01-main-start
key 2; snap 02-main-autolap-off
key 3; snap 03-main-autolap-1min        # R1 on Auto lap: 1 min (saved)
key 1
key 3 1.5                               # start
sleep 3
snap 04-live-main
BEFORE=$(grep -c "Backlight::on" "$LOG")
DISPLAY=$DISP xdotool key 5; sleep 0.8       # the simulator's wrist raise
AFTER=$(grep -c "Backlight::on" "$LOG")
[ "$AFTER" -gt "$BEFORE" ] && echo "  ok   a wrist raise lights the backlight" || echo "  FAIL a wrist raise did not light the backlight"
key 2; snap 05-live-totals
key 2; snap 06-live-zones
key 2; snap 07-live-clock
key 2; sleep 0.3                        # round to the main face
key 4 0.7; snap 08-lap-toast            # R2: lap 1 ends
sleep 3; snap 09-live-lap2
# The auto lap: a minute of lap 2 (the timer runs from the R2 above).
sleep 54
for _ in $(seq 20); do grep -qs "Auto lap" "$LOG" && break; sleep 0.5; done
sleep 0.3; snap 10-auto-lap-toast
sleep 3
key 3 1; snap 11-action-resume
key 2; snap 12-action-pause
key 3 1.5                               # pause
snap 13-action-resume-timer
key 4 1; snap 14-live-paused
key 3 1; key 2; key 3 1.5               # menu (opens on Resume), Pause row: resume
key 4 1                                 # the menu stays open after a pause or resume: back
key 3 1; key 2; key 2; snap 15-action-end
down 3; sleep 0.8; snap 16-hold-mid; sleep 1.0; up 3
sleep 0.4; snap 17-saved-streak-line
sleep 3.6; snap 18-summary-overview
key 2; snap 19-summary-zones
key 2; snap 20-summary-heart
key 2 1; snap 21-summary-laps-1
key 4 1; snap 22-main-after

# -- Workout 2: discard ---------------------------------------------------------
key 3 1.5
sleep 3
key 3 1; key 2; key 2; key 2; snap 23-action-discard
down 3; sleep 2.0; up 3
sleep 0.4; snap 24-discarded
sleep 2.6; snap 25-main-after-discard
key 4 3

kill -0 $SIMPID 2>/dev/null && echo "simulator still running after R2" || echo "simulator exited on R2"
grep -E "Workout (started|saved|discarded)|Auto lap|Streak line|Recovered|Failed|ERROR|Weight" "$LOG" | head -20

FITS=$(find "$FS/Activity" -name '*.fit')
[ -n "$FITS" ] || { echo "FATAL: no .fit under $FS/Activity" >&2; exit 1; }
[ "$(echo "$FITS" | wc -l)" -eq 1 ] || { echo "FAIL: expected one .fit (the discarded workout writes none)" >&2; echo "$FITS" >&2; }
FIT=$(echo "$FITS" | head -1)
echo "activity: $FIT ($(wc -c < "$FIT") bytes)"
SIDE="${FIT%.fit}.json"
[ -f "$SIDE" ] && echo "sidecar: $(cat "$SIDE")" || echo "FAIL: no .json sidecar"
echo "settings: $(cat "$FS/settings.json" 2>/dev/null || echo none)"
cp "$FIT" "$OUT/w0-workout.fit"
python3 -I "$HERE/fit_check.py" "$FIT" sim
rc=$?

# What Streak will read: one entry, ours, sport 5 (workout), the .fit's start and timer time.
python3 -I - "$SHARED/act_workout.json" "$FIT" "$LOG" <<'EOF' || rc=1
import json, sys, fitdecode
act = json.load(open(sys.argv[1]))
with fitdecode.FitReader(sys.argv[2]) as r:
    sess = next(f for f in r if isinstance(f, fitdecode.FitDataMessage) and f.name == "session")
start = int(sess.get_value("start_time").timestamp())
timer = int(sess.get_value("total_timer_time"))
ok = True
def check(c, what):
    global ok
    print(("  ok   " if c else "  FAIL ") + what)
    ok &= bool(c)
check(act.get("app") == "HybridXWorkout", f"act_workout.json app {act.get('app')}")
check(len(act.get("a", [])) == 1, f"one entry ({len(act.get('a', []))})")
e = act["a"][0]
check(e[0] == start, f"entry start {e[0]} == the .fit session's {start}")
check(e[2] == timer, f"entry duration {e[2]} == the .fit's timer time {timer}")
check(e[6] == 5, f"entry sport {e[6]} is 5 (workout)")
log = open(sys.argv[3]).read()
check('Streak line: "3 of 4 this week" "7-week streak"' in log, "the Streak line read 3 of 4 this week, 7-week streak")
sys.exit(0 if ok else 1)
EOF
exit $rc
