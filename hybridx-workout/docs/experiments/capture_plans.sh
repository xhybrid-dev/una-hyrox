#!/bin/bash
#
# The W3 screens in the PC simulator, by key presses, captured into
# docs/screens/w3-*.png: choosing a kind and its settings on the main menu, a
# whole strength workout (sets, a rest that runs out, a rest skipped, the end
# of the plan, the done menu, End, the summary and its laps), a glance at EMOM,
# AMRAP and Tabata (each discarded), and a whole circuit. Then the two files
# are checked (sport, laps, sets) and act_workout.json (Strength 4, Hybrid 6).
#
# The plan's numbers start short (settings.json: 2 sets with 15 s rests, 1 round
# of 100 m), so the run takes about 4 minutes; the kind is chosen on screen.
# Needs Xvfb, xdotool, ImageMagick and fitdecode; build the simulator first.
# Usage:  UNA_SDK=/path/to/una-sdk ./capture_plans.sh [out-dir]
set -u
HERE=$(cd "$(dirname "$0")" && pwd)
REPO=$(cd "$HERE/../.." && pwd)
OUT=${1:-$REPO/docs/screens}
BIN="$REPO/Software/Apps/LVGL-GUI/simulator/build/bin"
FS="$REPO/Software/Output"
SHARED="$REPO/Software/SharedData/HybridX"
DISP=:94
LOG=${SIM_LOG:-/tmp/hybridx-workout-plans.log}

[ -x "$BIN/HybridXWorkoutSimulator" ] || { echo "Build the simulator first ($BIN)" >&2; exit 1; }
mkdir -p "$OUT" "$SHARED" "$FS"
rm -rf "$FS/Activity" "$FS/settings.json.bak" "$SHARED/act_workout.json" "$SHARED/act_workout.json.bak"
echo '{"v":2,"autolap":0,"type":0,"sets":2,"srest":15,"scap":300,"emom":2,"amrap":1,"work":10,"rest":5,"rounds":2,"crounds":1,"run":100}' \
    > "$FS/settings.json"

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

key()  { DISPLAY=$DISP xdotool key "$1"; sleep "${2:-1.0}"; }
snap() { DISPLAY=$DISP timeout 10 import -window "$WID" "$OUT/w3-$1.png" 2>/dev/null; echo "  $1"; }
down() { DISPLAY=$DISP xdotool keydown "$1"; }
up()   { DISPLAY=$DISP xdotool keyup "$1"; sleep 0.6; }
discard() { key 3 1; key 2; key 2; key 2; down 3; sleep 2.0; up 3; sleep 3.5; }   # menu, Discard, hold

# -- The main menu: the kind and its settings ----------------------------------
snap 01-main-free
key 2; snap 02-main-type-free
key 3; snap 03-main-type-strength                # R1 on Type: Strength
key 2; snap 04-main-settings-strength            # the plan in a line
key 3 1.5; snap 05-settings-sets                 # R1: the Strength settings
key 3; snap 06-settings-sets-stepped             # R1 steps Sets 2 to 3
key 2; snap 07-settings-rest
key 2; snap 08-settings-cap
key 4 1.5; snap 09-main-after-settings           # R2: back, the hint updated

# -- A strength workout: 3 sets, 15 s rests ---------------------------------------
key 1; key 1                                     # up to Start
key 3 1.5; sleep 2; snap 10-strength-set1
sleep 4
key 4 0.6; snap 11-toast-rest                    # R2: set 1 done, the rest begins
sleep 3; snap 12-strength-rest-countdown
sleep 9.5; snap 13-strength-rest-last-seconds    # amber in its last three seconds
sleep 3.5; snap 14-toast-set2                    # the rest ran out: set 2
sleep 4
key 4 0.6                                        # set 2 done: rest
sleep 3
key 4 0.6; snap 15-toast-rest-skipped            # R2 in a rest: set 3 at once
sleep 5
key 4 0.6; snap 16-toast-done                    # the last set: the plan is over
sleep 3; snap 17-strength-done
key 3 1; snap 18-action-done                     # the done menu opens on End
down 3; sleep 0.8; snap 19-hold-end; sleep 1.0; up 3
sleep 0.4; snap 20-saved
sleep 3.8; snap 21-summary-strength
key 2; key 2; key 2 1.5; snap 22-summary-strength-laps
key 4 1.5

# -- EMOM, AMRAP and Tabata, a look at each, discarded -----------------------------
key 2; key 3; key 1                              # Type: EMOM, back up to Start
key 3 1.5; sleep 3; snap 23-emom-live
discard
key 2; key 3; key 1                              # AMRAP
key 3 1.5; sleep 5
key 4 0.6; snap 24-toast-amrap-round
sleep 3; snap 25-amrap-live
discard
key 2; key 3; key 1                              # Tabata
key 3 1.5; sleep 4; snap 26-tabata-work
sleep 18; snap 27-tabata-rest
discard

# -- A circuit: 1 round, 100 m ---------------------------------------------------------
key 2; key 3; key 3; key 1                       # Intervals, then Circuit
key 3 1.5; sleep 3; snap 28-circuit-run
sleep 4
key 4 0.6; snap 29-toast-station                 # R2: the run done, the SkiErg begins
sleep 3; snap 30-circuit-station
sleep 4
key 4 0.6; sleep 3; snap 31-circuit-done
key 3 1; down 3; sleep 1.8; up 3                 # End
sleep 4.2; snap 32-summary-circuit
key 2; key 2; key 2 1.5; snap 33-summary-circuit-laps
key 4 1.5
key 2; key 3; key 1                              # Type round to Free, as it began
snap 34-main-free-again
key 4 3

kill -0 $SIMPID 2>/dev/null && echo "simulator still running after R2" || echo "simulator exited on R2"
grep -E "Workout (started|saved|discarded)|Plan done|ERROR|Failed" "$LOG" | sed 's/^.*: //' | head -20
echo "settings: $(cat "$FS/settings.json" 2>/dev/null)"

rc=0
python3 -I - "$FS/Activity" "$SHARED/act_workout.json" <<'EOF' || rc=1
import glob, json, os, sys, fitdecode
fits = sorted(glob.glob(os.path.join(sys.argv[1], "*", "*.fit")))
ok = True
def check(c, what):
    global ok
    print(("  ok   " if c else "  FAIL ") + what)
    ok &= bool(c)
check(len(fits) == 2, f"two files (strength and circuit; the three looks were discarded): {len(fits)}")
want = [("training", "strength_training", "Strength", 5), ("training", "cardio_training", "Circuit", 2)]
for fit, (sport, sub, name, nlaps) in zip(fits, want):
    msgs = {}
    with fitdecode.FitReader(fit) as r:
        for f in r:
            if isinstance(f, fitdecode.FitDataMessage):
                msgs.setdefault(f.name, []).append({x.name: x.value for x in f.fields})
    s = msgs["session"][0]
    check(s.get("sport") == sport and s.get("sub_sport") == sub and s.get("sport_profile_name") == name,
          f"{name}: {s.get('sport')} / {s.get('sub_sport')} / {s.get('sport_profile_name')}")
    check(len(msgs.get("lap", [])) == nlaps, f"{name}: {len(msgs.get('lap', []))} laps, want {nlaps}")
    if name == "Strength":
        kinds = [l.get("intensity") for l in msgs["lap"]]
        check(kinds == ["active", "rest", "active", "rest", "active"], f"Strength laps {kinds}")
        check(len(msgs.get("set", [])) == 5, f"Strength: {len(msgs.get('set', []))} set messages")
act = json.load(open(sys.argv[2]))
check([e[6] for e in act["a"]] == [4, 6], f"act_workout.json sports {[e[6] for e in act['a']]} == [4, 6]")
sys.exit(0 if ok else 1)
EOF
rm -f "$FS/settings.json" "$FS/settings.json.bak"   # leave the simulator on its defaults
exit $rc
