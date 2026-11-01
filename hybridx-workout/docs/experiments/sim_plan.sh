#!/bin/bash
#
# One workout of each kind (W2) through the real service in the PC simulator,
# before the screens can choose one (W3): the plan is written into the app's
# settings.json (version 2) before each start, the workout is started and
# driven by key presses (R2 at the scripted seconds), and once the plan is done
# the action menu (W3: End, Discard, Back, opening on End) ends it by a held R1.
# The service also refuses a Resume after the plan (logged "Resume refused"),
# but the screens no longer offer one, so it is not driven here.
# Then: the log's cues, the .fit decoded (structure as fit_check.py's "sim",
# plus the sport, sub_sport, profile name, lap intensity and, for strength, the
# set messages), and act_workout.json's sport code for HybridX Streak.
#
# Short plans, so the whole run takes about 9 minutes (Tabata is fixed at 3:50):
#   strength  2 sets, 10 s rest: R2 at 15 s (set 1 ends), the rest runs out, R2 at 40 s
#   emom      2 minutes
#   amrap     a 1-minute cap, rounds at 20 and 40 s
#   tabata    8 x 20/10
#   custom    10 s work, 5 s rest, 3 rounds
#   circuit   1 round, 100 m run: R2 at 10 s (station), R2 at 20 s (the end)
#
# Needs Xvfb, xdotool and fitdecode (pip); build the simulator first.
# Usage:  UNA_SDK=/path/to/una-sdk ./sim_plan.sh [type ...]   (default: all six)
set -u
HERE=$(cd "$(dirname "$0")" && pwd)
REPO=$(cd "$HERE/../.." && pwd)
BIN="$REPO/Software/Apps/LVGL-GUI/simulator/build/bin"
FS="$REPO/Software/Output"
SHARED="$REPO/Software/SharedData/HybridX"
DISP=:96
LOG=${SIM_LOG:-/tmp/hybridx-workout-plan.log}
TYPES=${*:-strength emom amrap tabata custom circuit}

[ -x "$BIN/HybridXWorkoutSimulator" ] || { echo "Build the simulator first ($BIN)" >&2; exit 1; }
mkdir -p "$SHARED"

Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
SIMPID=""
cleanup() { [ -n "$SIMPID" ] && kill -9 $SIMPID 2>/dev/null; kill $XPID 2>/dev/null; }
trap cleanup EXIT
sleep 2

key()  { DISPLAY=$DISP xdotool key "$1"; sleep "${2:-1.0}"; }
down() { DISPLAY=$DISP xdotool keydown "$1"; }
up()   { DISPLAY=$DISP xdotool keyup "$1"; sleep 0.6; }

# settings.json for each type, and its R2 presses (seconds after Start), how long
# the plan runs, and what the file must say: type sport sub_sport profile link.
settings() {
    local base='"v":2,"autolap":0,"sets":2,"srest":10,"scap":300,"emom":2,"amrap":1,"work":10,"rest":5,"rounds":3,"crounds":1,"run":100'
    case $1 in
        strength) echo "{$base,\"type\":1}" ;;
        emom)     echo "{$base,\"type\":2}" ;;
        amrap)    echo "{$base,\"type\":3}" ;;
        tabata)   echo "{$base,\"type\":4}" ;;
        custom)   echo "{$base,\"type\":5}" ;;
        circuit)  echo "{$base,\"type\":6}" ;;
    esac
}
presses() { case $1 in strength) echo "15 40" ;; amrap) echo "20 40" ;; circuit) echo "10 20" ;; *) echo "" ;; esac; }
runfor()  { case $1 in strength) echo 42 ;; emom) echo 122 ;; amrap) echo 62 ;; tabata) echo 232 ;; custom) echo 42 ;; circuit) echo 22 ;; esac; }
expect()  {
    case $1 in
        strength) echo "Strength training strength_training 4 3 active,rest,active" ;;
        emom)     echo "EMOM hiit emom 5 2 active,active" ;;
        amrap)    echo "AMRAP hiit amrap 5 3 active,active,active" ;;
        tabata)   echo "Tabata hiit tabata 5 15 $(python3 -c 'print(",".join(["active","rest"]*7+["active"]))')" ;;
        custom)   echo "Intervals hiit hiit 5 5 active,rest,active,rest,active" ;;
        circuit)  echo "Circuit training cardio_training 6 2 active,active" ;;
    esac
}

rc=0
for T in $TYPES; do
    echo "== $T"
    rm -rf "$FS/Activity" "$FS/settings.json.bak" "$SHARED/act_workout.json" "$SHARED/act_workout.json.bak"
    mkdir -p "$FS"
    settings "$T" > "$FS/settings.json"
    : > "$LOG"
    ( cd "$BIN" && exec env DISPLAY=$DISP stdbuf -oL -eL ./HybridXWorkoutSimulator >> "$LOG" 2>&1 ) & SIMPID=$!
    sleep 4
    key 3 0.2                                   # Start (the main menu opens on it)
    T0=$(date +%s.%N)
    for p in $(presses "$T"); do
        python3 -c "import time; time.sleep(max(0, $T0 + $p - time.time()))"
        key 4 0.2                               # R2
    done
    python3 -c "import time; time.sleep(max(0, $T0 + $(runfor "$T") + 3 - time.time()))"
    key 3 1                                     # the done menu opens on End workout
    down 3; sleep 1.8; up 3                     # hold R1 to end
    sleep 4
    kill -9 $SIMPID 2>/dev/null; wait $SIMPID 2>/dev/null; SIMPID=""
    grep -E "Workout (started|saved)|Plan done|Resume refused|Streak line|ERROR|Failed" "$LOG" | sed 's/^/  log  /'

    FIT=$(find "$FS/Activity" -name '*.fit' 2>/dev/null | head -1)
    if [ -z "$FIT" ]; then echo "  FAIL no .fit"; rc=1; continue; fi
    python3 -I "$HERE/fit_check.py" "$FIT" "sim-$T" > "$LOG.check" 2>&1 || { rc=1; grep FAIL "$LOG.check"; }
    grep -q "Plan done" "$LOG" && echo "  ok   the plan ran to its end" || { echo "  FAIL the plan did not end"; rc=1; }
    python3 -I - "$FIT" "$SHARED/act_workout.json" $(expect "$T") <<'EOF' || rc=1
import json, sys, fitdecode
fit, act, name, sport, sub, link, nlaps, inten = sys.argv[1:9]
msgs = {}
with fitdecode.FitReader(fit) as r:
    for f in r:
        if isinstance(f, fitdecode.FitDataMessage):
            msgs.setdefault(f.name, []).append({x.name: x.value for x in f.fields})
s = msgs["session"][0]
laps = msgs.get("lap", [])
ok = True
def check(c, what):
    global ok
    print(("  ok   " if c else "  FAIL ") + what)
    ok &= bool(c)
check(s.get("sport") == sport and s.get("sub_sport") == sub, f"sport {s.get('sport')} / {s.get('sub_sport')} == {sport} / {sub}")
check(s.get("sport_profile_name") == name, f"profile name {s.get('sport_profile_name')}")
check(len(laps) == int(nlaps), f"{len(laps)} laps, want {nlaps}")
got = ",".join(str(l.get("intensity")) for l in laps)
check(got == inten, f"lap intensity {got}")
ev = [e.get("event_type") for e in msgs.get("event", [])]
check(all(ev[i] != ev[i + 1] for i in range(len(ev) - 1)) and ev[-1] == "stop", f"timer events pair ({ev})")
sets = msgs.get("set", [])
if name == "Strength":
    check(len(sets) == len(laps) and [x.get("set_type") for x in sets] == got.split(","), f"{len(sets)} sets, one a lap")
else:
    check(not sets, "no set messages")
e = json.load(open(act))["a"][0]
check(e[6] == int(link), f"act_workout.json sport {e[6]} == {link}")
check(e[2] == int(s["total_timer_time"]), f"act_workout.json duration {e[2]} == the .fit's {int(s['total_timer_time'])}")
sys.exit(0 if ok else 1)
EOF
done
rm -f "$FS/settings.json"   # leave the simulator on its defaults
echo "$([ $rc -eq 0 ] && echo 'ALL PLANS PASSED' || echo 'SOME PLANS FAILED')"
exit $rc
