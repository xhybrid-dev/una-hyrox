#!/bin/bash
#
# The HybridX family on one pretend watch: Run, Race and Trail each record and
# save an activity in their own simulator, all three leaving their summaries in
# ONE shared folder, then HybridX Streak opens (twice) and must find every one,
# once. It checks the activity link end to end with the apps' real writers and
# Streak's real reader (docs/ACTIVITY_LINK.md, "Family check").
#
# The simulators keep their files in <cwd>/../../../../../Output, so each app
# runs from its own <out>/<App>/a/b/c/d/e and its ../SharedData is a symlink to
# the one <out>/SharedData, as every app's is the same folder on a watch:
#
#   <out>/SharedData/HybridX/act_*.json        the summaries
#   <out>/<App>/Output/                        each app's own folder
#   <out>/<App>/SharedData -> ../SharedData
#
# Streak cannot list other apps' folders on a watch, and here it finds none
# either (no app folder is its sibling), so it sees only the summaries.
#
# Quick (default): about 80 s of recording each, the three at once, with
# Streak's minimum set to 0 minutes so each short session counts.
# LONG=1: 11 minutes each, with Streak's default 10-minute minimum.
#
# Needs: the four simulators built (each app's Software/Apps/LVGL-GUI/simulator/
# build/bin), Xvfb, xdotool, python3 with fitdecode. Exits non-zero on a FAIL.
# Usage:  family_check.sh [out-dir]
set -u
ROOT=$(cd "$(dirname "$0")/../../.." && pwd)
OUT=${1:-/tmp/hybridx-family}
LONG=${LONG:-0}
if [ "$LONG" = 1 ]; then SECS=660; else SECS=80; fi

bin() { echo "$ROOT/$1/Software/Apps/LVGL-GUI/simulator/build/bin/$2"; }
for pair in hybridx-run:HybridXRunSimulator hybridx-race:HybridXRaceSimulator \
            hybridx-trail:HybridXTrailSimulator hybridx-streak:HybridXStreakSimulator; do
    [ -x "$(bin "${pair%%:*}" "${pair##*:}")" ] || { echo "Build the simulator first: $(bin "${pair%%:*}" "${pair##*:}")" >&2; exit 1; }
done

rm -rf "$OUT"
mkdir -p "$OUT/SharedData/HybridX"
for app in Run Race Trail Streak; do
    mkdir -p "$OUT/$app/Output" "$OUT/$app/a/b/c/d/e"
    ln -s ../SharedData "$OUT/$app/SharedData"
done
# Run needs an age for its maximum heart rate; Trail needs a route.
printf '{"schema":1,"values":{"birthYear":1986,"birthMonth":5}}' > "$OUT/Run/Output/app_config.json"
mkdir -p "$OUT/Trail/Output/Routes"
python3 "$ROOT/hybridx-trail/Tools/TestRoutes/make_sim_routes.py" "$OUT/Trail/Output/Routes" >/dev/null
printf "sim-wrong-turn.gpx" > "$OUT/Trail/Output/route.sel"

PIDS=()
cleanup() { for p in "${PIDS[@]}"; do kill -9 "$p" 2>/dev/null; done; }
trap cleanup EXIT

# start <App> <display> <binary> [log]: an Xvfb and the simulator, from the app's own folder.
start() {
    rm -f "/tmp/.X$2-lock" "/tmp/.X11-unix/X$2"   # left by a display killed earlier
    Xvfb ":$2" -screen 0 800x800x24 >/dev/null 2>&1 & PIDS+=($!); disown $!
    sleep 2
    (cd "$OUT/$1/a/b/c/d/e" && DISPLAY=":$2" exec stdbuf -oL -eL "$3" > "$OUT/${4:-$1}.log" 2>&1) & PIDS+=($!); disown $!
}
key()  { DISPLAY=":$1" xdotool key "$2"; sleep "${3:-1}"; }
hold() { DISPLAY=":$1" xdotool keydown "$2"; sleep "$3"; DISPLAY=":$1" xdotool keyup "$2"; sleep 1; }
# Wait for the simulated GPS fix: RunLVGL's first-fix buzz is logged. A Start
# pressed before it opens the "no GPS" question instead.
gpsfix() { for _ in $(seq 60); do grep -q "Mock.Buzzer::play" "$OUT/$1.log" && break; sleep 1; done; sleep 2; }

# The key flows are the apps' own scripts' (Run and Trail sim_run.sh, Race
# capture_screens.sh). Keys 1-4 are L1, L2, R1, R2.
run_app() {   # Run: R1 start; R1 menu; L2 L2 Save; hold R1
    start Run 91 "$(bin hybridx-run HybridXRunSimulator)"; gpsfix Run
    key 91 3 2; sleep "$SECS"
    key 91 3 1; key 91 2; key 91 2; hold 91 3 3; sleep 4
    cleanup   # this function runs in a subshell: its own simulator and display
}
trail_app() { # Trail: the same as Run
    start Trail 92 "$(bin hybridx-trail HybridXTrailSimulator)"; gpsfix Trail
    key 92 3 2; sleep "$SECS"
    key 92 3 1; key 92 2; key 92 2; hold 92 3 3; sleep 4
    cleanup
}
race_app() {  # Race: R1 Start race, R1 confirm, 16 splits on R2 (a Full race), R1 save
    start Race 93 "$(bin hybridx-race HybridXRaceSimulator)"; sleep 5
    key 93 3 1; key 93 3 2
    local gap=$(( SECS / 16 )); [ "$gap" -lt 4 ] && gap=4   # the split lockout is 3 s
    for _ in $(seq 1 16); do sleep "$gap"; DISPLAY=:93 xdotool key 4; done
    sleep 4; key 93 3 4
    cleanup
}

echo "Recording: Run, Race and Trail at once, about $SECS s each..."
run_app & A=$!; trail_app & B=$!; race_app & C=$!
wait $A $B $C

if [ "$LONG" = 1 ]; then
    printf '{"schema":1,"values":{"weeklyTarget":7}}' > "$OUT/Streak/Output/app_config.json"
else
    printf '{"schema":1,"values":{"weeklyTarget":7,"minMinutes":0}}' > "$OUT/Streak/Output/app_config.json"
fi
open_streak() {   # open_streak <log name>: open, let the "+1" toasts play, close
    start Streak 94 "$(bin hybridx-streak HybridXStreakSimulator)" "$1"
    for _ in $(seq 20); do grep -q "Summaries:" "$OUT/$1.log" && break; sleep 1; done
    sleep 12
    if [ -n "${SHOTS:-}" ]; then
        mkdir -p "$SHOTS"
        DISPLAY=:94 import -window "$(DISPLAY=:94 xdotool search --name 'HybridX Streak' | head -1)" "$SHOTS/$1.png" 2>/dev/null
    fi
    cleanup; PIDS=()
    sleep 1
}
echo "Opening Streak twice..."
open_streak Streak-1
cp "$OUT/SharedData/HybridX/streak.json" "$OUT/streak-1.json" 2>/dev/null
open_streak Streak-2
cp "$OUT/SharedData/HybridX/streak.json" "$OUT/streak-2.json" 2>/dev/null

python3 - "$OUT" "$LONG" <<'EOF'
import glob, json, os, re, sys
import fitdecode

out, long_run = sys.argv[1], sys.argv[2] == "1"
rows, ok = [], True
def check(name, passed, detail):
    global ok
    ok &= passed
    rows.append(("PASS" if passed else "FAIL", name, detail))

# Each writer: one entry, its own name and sport, the .fit's start and timer time.
for app, slot, sport in (("Run", "run", 1), ("Race", "race", 6), ("Trail", "trail", 1)):
    fits = glob.glob(f"{out}/{app}/Output/Activity/*/*.fit") + glob.glob(f"{out}/{app}/Output/Activity/*.fit")
    path = f"{out}/SharedData/HybridX/act_{slot}.json"
    if not fits or not os.path.exists(path):
        check(f"{app} saved", False, f"{len(fits)} .fit, act_{slot}.json {'there' if os.path.exists(path) else 'missing'}")
        continue
    with fitdecode.FitReader(fits[0]) as r:
        sess = next(f for f in r if isinstance(f, fitdecode.FitDataMessage) and f.name == "session")
    start = int(sess.get_value("start_time").timestamp())
    timer = sess.get_value("total_timer_time")
    doc = json.load(open(path))
    entries = doc.get("a", [])
    e = entries[0] if entries else [0] * 7
    check(f"{app} summary", doc.get("v") == 1 and doc.get("app") == f"HybridX{app}" and len(entries) == 1
          and e[6] == sport, f"{doc.get('app')}, {len(entries)} entry, sport {e[6]}")
    check(f"{app} = its .fit", e[0] == start and abs(e[2] - timer) <= 1,
          f"start {e[0]} vs {start}, {e[2]} s vs {timer:.0f} s")

def summaries(log):
    try:
        text = open(f"{out}/{log}.log", errors="replace").read()
    except OSError:
        return None
    m = re.search(r"Summaries: (\d+) files, (\d+) entries, (\d+) new, (\d+) counted", text)
    return tuple(map(int, m.groups())) if m else None

first, second = summaries("Streak-1"), summaries("Streak-2")
check("Streak, first open", first is not None and first[:3] == (3, 3, 3), f"files, entries, new = {first[:3] if first else None}")
check("Streak, second open", second is not None and second[2] == 0 and second[3] == 3,
      f"new {second[2] if second else None}, already counted {second[3] if second else None}")

# What Streak counts, from its public state (StateCodec): goal "g" = [target, week
# start, scope, min minutes, one per day]; session "s" = [app key, local start,
# minutes, kind, app index, flags]; "a" = the app names. The rule is
# StreakModel::status (each test run is on its own day here, so one-per-day aside).
KINDS = ["Run", "Ride", "Walk", "Strength", "Workout", "Hybrid", "Row", "Other"]
def counted(name):
    try:
        st = json.load(open(f"{out}/{name}"))
    except (OSError, ValueError):
        return None
    g = st["g"]
    got = []
    for key, start, minutes, kind, app, flags in st["s"]:
        if flags & 0x02 or (g[2] != 255 and kind != g[2]) or (g[3] > 0 and minutes < g[3] and not flags & 0x01):
            continue
        got.append((st["a"][app] if app < len(st["a"]) else "?", KINDS[kind] if kind < len(KINDS) else "?"))
    return sorted(got)
want = sorted([("HybridXRace", "Hybrid"), ("HybridXRun", "Run"), ("HybridXTrail", "Run")])
c1, c2 = counted("streak-1.json"), counted("streak-2.json")
check("Streak counts each", c1 == want, ", ".join(f"{a} {k}" for a, k in c1) if c1 else "no state")
check("Streak, still once each", c2 == want, f"{len(c2) if c2 else 0} sessions after the second open")

# Trail's minutes for Run's VO2max: information only (the simulator's heart rate is not steady running).
vo2 = sorted(os.path.basename(p) for p in glob.glob(f"{out}/SharedData/HybridX/trail_vo2_*.json"))
rows.append(("info", "Trail -> Run VO2 windows", ", ".join(vo2) if vo2 else "none left (expected in the simulator)"))

w = max(len(r[1]) for r in rows)
for status, name, detail in rows:
    print(f"{status:5} {name:<{w}}  {detail}")
print("Streak's own copy after the first open:", f"{out}/streak-1.json")
sys.exit(0 if ok else 1)
EOF
