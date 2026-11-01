#!/bin/bash
#
# Walk every screen of HybridX Ride in the PC simulator by key presses,
# capture each, and check what the saved rides leave behind: the .fit files
# (decoded), their sidecars, act_ride.json for HybridX Streak, and the Streak
# line on the Saved screen.
#
# Xvfb for a display, xdotool for the buttons (keys 1-4 are L1, L2, R1, R2),
# ImageMagick's import for frames, fitdecode (pip) for the files. Build the
# simulator first. The simulator's GPS finds its fix about 4 s after the
# screen opens, then rides a loop at 20 to 30 km/h; its heart rate is random.
#
# Before it starts, a pretend HybridX Streak summary (streak_now.json: this
# week 2 of 4 done, a 7-week streak) goes in the shared folder, and the app's
# settings are cleared.
# Ride 1 (outdoor, saved): the main menu rows; Auto lap to 1 min; Start with
# the GPS ready; the five faces; a lap by R2 (its toast); an auto lap a minute
# in (its toast); the action menu, a pause and a resume; End by holding R1;
# Saved, with the Streak line ("3 of 4 this week", "7-week streak"); the
# summary pages including Ride (speed and climb).
# Ride 2 (outdoor, discarded): Discard by holding R1.
# (The no-GPS warning is shown at the very start: R1 on Start before the fix says "No GPS: R1 again".)
# Ride 3 (indoor, saved): Ride row to Indoor; Start; the four faces; End.
#
# Usage:  UNA_SDK=/path/to/una-sdk ./capture_screens.sh [out-dir]
#   out-dir defaults to docs/screens (files r1-*.png). Takes about 3.5 minutes.
set -u
HERE=$(cd "$(dirname "$0")" && pwd)
REPO=$(cd "$HERE/../.." && pwd)
OUT=${1:-$REPO/docs/screens}
BIN="$REPO/Software/Apps/LVGL-GUI/simulator/build/bin"
FS="$REPO/Software/Output"
SHARED="$REPO/Software/SharedData/HybridX"
DISP=:97
LOG=${SIM_LOG:-/tmp/hybridx-ride-capture.log}

[ -x "$BIN/HybridXRideSimulator" ] || { echo "Build the simulator first ($BIN)" >&2; exit 1; }
mkdir -p "$OUT" "$SHARED"
rm -rf "$FS/Activity" "$FS/settings.json" "$FS/settings.json.bak" "$SHARED/act_ride.json" "$SHARED/act_ride.json.bak"

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
( cd "$BIN" && exec env DISPLAY=$DISP stdbuf -oL -eL ./HybridXRideSimulator >> "$LOG" 2>&1 ) & SIMPID=$!
sleep 1.5
WID=$(DISPLAY=$DISP xdotool search --name 'HybridX Ride' | tail -1)
[ -n "$WID" ] || { echo "FATAL: no simulator window" >&2; exit 1; }

key()  { DISPLAY=$DISP xdotool key "$1"; sleep "${2:-1.0}"; }   # 1 s: a wheel slide takes 0.4 s, and Xvfb is slow
N=0
snap() { N=$((N + 1)); local f; f=$(printf 'r1-%02d-%s' "$N" "$1"); DISPLAY=$DISP timeout 10 import -window "$WID" "$OUT/$f.png" 2>/dev/null; echo "  $f"; }
down() { DISPLAY=$DISP xdotool keydown "$1"; }
up()   { DISPLAY=$DISP xdotool keyup "$1"; sleep 0.6; }

# -- Ride 1: outdoor, saved -------------------------------------------------------
snap main-start-finding-gps          # the screen has just opened: the GPS is still looking
key 3 0.8; snap main-no-gps-warning     # R1 straight away only warns ("No GPS: R1 again")
sleep 6
snap main-start-gps-ready               # the fix has come: the warning is gone
key 2; snap main-ride-row
key 2; snap main-heart-rate-row
key 2; snap main-autolap-off
key 3; snap main-autolap-1min        # R1 on Auto lap: 1 min (saved)
key 1; key 1; key 1                     # back to Start
key 3 1.5                               # start
sleep 4
snap live-main-outdoor
BEFORE=$(grep -c "Backlight::on" "$LOG")
DISPLAY=$DISP xdotool key 5; sleep 0.8       # the simulator's wrist raise
AFTER=$(grep -c "Backlight::on" "$LOG")
[ "$AFTER" -gt "$BEFORE" ] && echo "  ok   a wrist raise lights the backlight" || echo "  FAIL a wrist raise did not light the backlight"
key 2 1.5; snap live-lap-face
key 2; snap live-totals-outdoor
key 2; snap live-zones
key 2; snap live-clock
key 2; sleep 0.3                        # round to the main face
key 4 0.7; snap lap-toast-outdoor    # R2: lap 1 ends
sleep 3; snap live-lap2
# The auto lap: a minute of lap 2 (the timer runs from the R2 above).
sleep 54
for _ in $(seq 20); do grep -qs "Auto lap" "$LOG" && break; sleep 0.5; done
sleep 0.3; snap auto-lap-toast
sleep 3
key 3 1; snap action-resume
key 2; snap action-pause
key 3 1.5                               # pause
snap action-resume-timer
key 4 1; snap live-paused
key 3 1; key 2; key 3 1.5               # menu (opens on Resume), Pause row: resume
key 4 1                                 # the menu stays open after a pause or resume: back
key 3 1; key 2; key 2; snap action-end
down 3; sleep 0.8; snap hold-mid; sleep 1.0; up 3
sleep 0.4; snap saved-streak-line
sleep 3.6; snap summary-overview
key 2; snap summary-ride
key 2; snap summary-zones
key 2; snap summary-heart
key 2 1; snap summary-laps-1
key 4 1; snap main-after

# -- Ride 2: outdoor, discarded ---------------------------------------------------------
# (The simulator's GPS holds its fix once found, so a start with no fix cannot be shown again.)
key 3 1.5                               # start
sleep 3
snap live-main-outdoor-2
key 3 1; key 2; key 2; key 2; snap action-discard
down 3; sleep 2.0; up 3
sleep 0.4; snap discarded
sleep 2.6; snap main-after-discard

# -- Ride 3: indoor, saved ------------------------------------------------------------------
key 2; key 3 1; snap main-ride-indoor      # Ride row: R1 flips to Indoor
key 1; snap main-start-indoor
key 3 1.5                               # start
sleep 12
snap live-main-indoor
key 2; snap live-totals-indoor
key 2; snap live-zones-indoor
key 2; snap live-clock-indoor
key 3 1; key 2; key 2; snap action-end-indoor
down 3; sleep 2.0; up 3
sleep 0.4; snap saved-indoor
sleep 3.6; snap summary-indoor-overview
key 4 1                                 # back to the menu
key 4 3

kill -0 $SIMPID 2>/dev/null && echo "simulator still running after R2" || echo "simulator exited on R2"
grep -E "Ride (started|saved|discarded)|Auto lap|Streak line|Recovered|Failed|ERROR|Weight" "$LOG" | head -20

FITS=$(find "$FS/Activity" -name '*.fit' | sort)
[ -n "$FITS" ] || { echo "FATAL: no .fit under $FS/Activity" >&2; exit 1; }
[ "$(echo "$FITS" | wc -l)" -eq 2 ] || { echo "FAIL: expected two .fit files (the discarded ride writes none)" >&2; echo "$FITS" >&2; }
OUTDOOR_FIT=$(echo "$FITS" | head -1)
INDOOR_FIT=$(echo "$FITS" | tail -1)
rc=0
for pair in "outdoor:$OUTDOOR_FIT" "indoor:$INDOOR_FIT"; do
    kind=${pair%%:*}; fit=${pair#*:}
    echo "activity ($kind): $fit ($(wc -c < "$fit") bytes)"
    side="${fit%.fit}.json"
    [ -f "$side" ] && echo "sidecar: $(cat "$side")" || { echo "FAIL: no .json sidecar"; rc=1; }
    cp "$fit" "$OUT/r1-ride-$kind.fit"
    python3 -I "$HERE/fit_check.py" "$fit" "sim-$kind" || rc=1
done
echo "settings: $(cat "$FS/settings.json" 2>/dev/null || echo none)"

# What Streak will read: two entries, ours, sport 2 (ride), each .fit's start and timer time,
# the outdoor one with its distance and the indoor one with none.
python3 -I - "$SHARED/act_ride.json" "$OUTDOOR_FIT" "$INDOOR_FIT" "$LOG" <<'EOF' || rc=1
import json, sys, fitdecode
act = json.load(open(sys.argv[1]))
def session(path):
    with fitdecode.FitReader(path) as r:
        return next(f for f in r if isinstance(f, fitdecode.FitDataMessage) and f.name == "session")
ok = True
def check(c, what):
    global ok
    print(("  ok   " if c else "  FAIL ") + what)
    ok &= bool(c)
check(act.get("app") == "HybridXRide", f"act_ride.json app {act.get('app')}")
check(len(act.get("a", [])) == 2, f"two entries ({len(act.get('a', []))})")
for e, path, label in zip(act["a"], sys.argv[2:4], ("outdoor", "indoor")):
    s = session(path)
    start = int(s.get_value("start_time").timestamp())
    timer = int(s.get_value("total_timer_time"))
    check(e[0] == start, f"{label}: entry start {e[0]} == the .fit session's {start}")
    check(e[2] == timer, f"{label}: entry duration {e[2]} == the .fit's timer time {timer}")
    check(e[6] == 2, f"{label}: entry sport {e[6]} is 2 (ride)")
    if label == "outdoor":
        want = round(s.get_value("total_distance"))
        check(e[3] == want and e[3] > 0, f"outdoor: entry distance {e[3]} m == the .fit's {want} m")
    else:
        check(e[3] == 0, f"indoor: entry distance {e[3]} is 0")
log = open(sys.argv[4]).read()
check('Streak line: "3 of 4 this week" "7-week streak"' in log, "the first Streak line read 3 of 4 this week, 7-week streak")
sys.exit(0 if ok else 1)
EOF
exit $rc
