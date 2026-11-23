#!/bin/bash
#
# The HybridX family on one pretend watch: X Training, Run, Race, Trail, Workout and Ride
# each record and save an activity in their own simulator, all six leaving their
# summaries in ONE shared folder, then HybridX Streak opens (twice) and must find every one,
# once. Workout saves three, one of each link sport it writes: a Free workout (Workout),
# a strength plan (Strength) and a circuit (Hybrid), so Streak must count eight, each as
# its own kind; the seventh save's Streak line meets the target of 7 (Streak's most) and starts
# the streak ("1-week streak"), and the eighth reads "8 of 7 this week" with it. It checks the activity link end to end with the apps' real writers and
# Streak's real reader (docs/ACTIVITY_LINK.md, "Family check").
#
# X Training (its TP4) runs today's planned session from a pretend plan (its sim_plan.sh's),
# for at least 400 s so that some minutes pass X Run's 5-minute warm-up, and leaves them for
# X Run's VO2max (training_vo2_*.json). X Run then opens once more and must read them once,
# behind its own mark for X Training.
#
# Form (Streak NOTES "S10"): after Streak's first open the script puts a mature load window in its
# summary, and each app's "Streak form:" must be what a separate copy of Streak's model makes of it
# plus the sessions saved so far.
#
# Training load (ACTIVITY_LINK.md, "Training load"): each summary's 8th number must be the
# Edwards load worked out again here from its own .fit's records (each record's heart rate in
# the watch's default zones, 95/114/133/152/171/190, a second each; within a point or 5 %, as
# the apps count their seconds slightly differently at the edges); Streak's state must keep
# their sum on today (its "L"), and the fitness, fatigue and form it shows must be what a
# separate reference of Banister's model makes of that day (42 and 7 days). Quick mode's
# 80-second sessions score little; LONG=1 gives loads in the tens.
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
# Quick (default): about 80 s of recording each (X Training 400 s), one after another, with
# Streak's minimum set to 0 minutes so each short session counts (about 20 minutes).
# LONG=1: 11 minutes each, with Streak's default 10-minute minimum.
# TREADMILL=1: Run records a treadmill run (no GPS; distance from cadence) and
# keeps its estimate on the distance screen (hybridx-run NOTES, "Treadmill").
#
# Needs: the seven simulators built (each app's Software/Apps/LVGL-GUI/simulator/
# build/bin), Xvfb, xdotool, python3 with fitdecode. Exits non-zero on a FAIL.
# Usage:  family_check.sh [out-dir]
set -u
ROOT=$(cd "$(dirname "$0")/../../.." && pwd)
OUT=${1:-/tmp/hybridx-family}
LONG=${LONG:-0}
if [ "$LONG" = 1 ]; then SECS=660; else SECS=80; fi
TSECS=$(( SECS > 400 ? SECS : 400 ))   # X Training: past X Run's 5-minute warm-up

bin() { echo "$ROOT/$1/Software/Apps/LVGL-GUI/simulator/build/bin/$2"; }
for pair in hybridx-run:HybridXRunSimulator hybridx-race:HybridXRaceSimulator \
            hybridx-trail:HybridXTrailSimulator hybridx-workout:HybridXWorkoutSimulator \
            hybridx-ride:HybridXRideSimulator hybridx-streak:HybridXStreakSimulator \
            hybridx-training:HybridXTrainingSimulator; do
    [ -x "$(bin "${pair%%:*}" "${pair##*:}")" ] || { echo "Build the simulator first: $(bin "${pair%%:*}" "${pair##*:}")" >&2; exit 1; }
done

rm -rf "$OUT"
mkdir -p "$OUT/SharedData/HybridX"
for app in Training Run Race Trail Workout Ride Streak; do
    mkdir -p "$OUT/$app/Output" "$OUT/$app/a/b/c/d/e"
    ln -s ../SharedData "$OUT/$app/SharedData"
done
# Run needs an age for its maximum heart rate, and a resting heart rate to score any run's
# minutes (the simulator's own watch figure comes too late: without it X Run leaves X Training's
# run waiting, "no VO2 profile yet"); Trail needs a route.
printf '{"schema":1,"values":{"birthYear":1986,"birthMonth":5,"restingHr":50}}' > "$OUT/Run/Output/app_config.json"
mkdir -p "$OUT/Trail/Output/Routes"
python3 "$ROOT/hybridx-trail/Tools/TestRoutes/make_sim_routes.py" "$OUT/Trail/Output/Routes" >/dev/null
printf "sim-wrong-turn.gpx" > "$OUT/Trail/Output/route.sel"
# X Training: a 10K plan at level 2 under way, week 5, today its first quality day (its
# sim_plan.sh's pretend watch: the phone's settings match the plan, so it is kept).
TDOW=$(( $(date +%u) - 1 )); TLONG=$(( (TDOW + 5) % 7 )); TDAYS=0
for k in 0 2 3 5; do TDAYS=$(( TDAYS | (1 << ((TLONG + k) % 7)) )); done
TSTART=$(date -d "$(date +%F) -$TDOW days -28 days" +%Y%m%d)
tday() { [ $(( (TDAYS >> $1) & 1 )) = 1 ] && echo true || echo false; }
printf '{"schema":1,"values":{"plan":1,"fiveK":"22:00","fiveKRecent":true,"halfTime":"","raceDate":"","level":2,"runMon":%s,"runTue":%s,"runWed":%s,"runThu":%s,"runFri":%s,"runSat":%s,"runSun":%s,"longRunDay":%d,"easyByHr":false}}' \
    "$(tday 0)" "$(tday 1)" "$(tday 2)" "$(tday 3)" "$(tday 4)" "$(tday 5)" "$(tday 6)" $(( TLONG + 1 )) \
    > "$OUT/Training/Output/app_config.json"
printf '{"v":1,"s":[1,2,%d,%d,0,0,0,1320,%s,1,0,0],"c":[0,%s,0,10],"log":[],"st":%s}' "$TDAYS" "$TLONG" "$TSTART" "$TSTART" "$(date +%Y%m%d)" \
    > "$OUT/Training/Output/plan.json"

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
gpsfix() { for _ in $(seq 60); do grep -qs "Mock.Buzzer::play" "$OUT/$1.log" && break; sleep 1; done; sleep 2; }

# The key flows are the apps' own scripts' (Run and Trail sim_run.sh, Race and
# Workout and Ride capture_screens.sh, X Training sim_plan.sh). Keys 1-4 are L1, L2, R1, R2.
training_app() {   # X Training: R1 Today, R1 start (the countdown); R1 menu; L2 x3 Save & End; hold R1
    # Mid-session the menu is Resume, End workout, Summary, Save & End, Discard: Save is third down.
    start Training 98 "$(bin hybridx-training HybridXTrainingSimulator)"; gpsfix Training
    key 98 3 1; key 98 3 6; sleep "$TSECS"
    key 98 3 1; key 98 2; key 98 2; key 98 2; hold 98 3 3; sleep 4
    cleanup
}
run_again() {   # X Run opens once more, after X Training's save: it reads X Training's minutes
    start Run 91 "$(bin hybridx-run HybridXRunSimulator)" Run-2; sleep 8
    cleanup; PIDS=()
}
run_app() {   # Run: R1 start; R1 menu; L2 L2 Save; hold R1
    start Run 91 "$(bin hybridx-run HybridXRunSimulator)"; gpsfix Run
    if [ "${TREADMILL:-0}" = 1 ]; then
        key 91 2; key 91 3; key 91 1        # L2 to the Treadmill switch, R1 on, L1 back to Start
    fi
    key 91 3 2; sleep "$SECS"
    key 91 3 1; key 91 2; key 91 2; hold 91 3 3; sleep 4
    if [ "${TREADMILL:-0}" = 1 ]; then
        key 91 4 3                          # the distance screen: R2 keeps the estimate
    fi
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

workout_one() {  # workout_one <free|strength|circuit>: one Workout session of that kind
    # The kind and its numbers go in the app's settings.json (version 2) before it opens, as
    # Workout's own sim_plan.sh does; the main menu opens on Start either way.
    local base='"v":2,"autolap":0,"sets":2,"srest":15,"scap":900,"emom":10,"amrap":12,"work":40,"rest":20,"rounds":6,"crounds":1,"run":100'
    case $1 in
        free)     printf '{%s,"type":0}' "$base" > "$OUT/Workout/Output/settings.json" ;;
        strength) printf '{%s,"type":1}' "$base" > "$OUT/Workout/Output/settings.json" ;;
        circuit)  printf '{%s,"type":6}' "$base" > "$OUT/Workout/Output/settings.json" ;;
    esac
    start Workout 96 "$(bin hybridx-workout HybridXWorkoutSimulator)" "Workout-$1"; sleep 5
    key 96 3 2
    case $1 in
        free)      # a lap on R2 halfway; R1 menu, L2 L2 End, hold R1
            sleep $(( SECS / 2 )); key 96 4 1; sleep $(( SECS / 2 ))
            key 96 3 1; key 96 2; key 96 2 ;;
        strength)  # 2 sets: R2 halfway (a 15 s rest, which runs out), R2 to end set 2: Done
            local rest=$(( SECS / 2 - 15 )); [ "$rest" -lt 5 ] && rest=5
            sleep $(( SECS / 2 )); key 96 4 1; sleep 15; sleep "$rest"; key 96 4 3
            key 96 3 1 ;;          # the done menu opens on End
        circuit)   # 1 round of 100 m: R2 halfway (the SkiErg), R2 again: Done
            sleep $(( SECS / 2 )); key 96 4 1; sleep $(( SECS / 2 )); key 96 4 3
            key 96 3 1 ;;
    esac
    hold 96 3 3; sleep 4
    cleanup; PIDS=()
}
workout_app() {  # Workout three times, one after another (they share one act_workout.json)
    workout_one free; workout_one strength; workout_one circuit
}

ride_app() {  # Ride (outdoor, the default): wait for the simulated GPS fix, R1 Start, R1 menu, L2 L2 End, hold R1
    start Ride 97 "$(bin hybridx-ride HybridXRideSimulator)"; sleep 9
    key 97 3 2; sleep "$SECS"
    key 97 3 1; key 97 2; key 97 2; hold 97 3 3; sleep 4
    cleanup
}

if [ "$LONG" = 1 ]; then
    printf '{"schema":1,"values":{"weeklyTarget":7}}' > "$OUT/Streak/Output/app_config.json"
else
    printf '{"schema":1,"values":{"weeklyTarget":7,"minMinutes":0}}' > "$OUT/Streak/Output/app_config.json"
fi
open_streak() {   # open_streak <log name>: open, let the "+1" toasts play, close
    start Streak 94 "$(bin hybridx-streak HybridXStreakSimulator)" "$1"
    for _ in $(seq 20); do grep -qs "Summaries:" "$OUT/$1.log" && break; sleep 1; done
    sleep 12
    if [ -n "${SHOTS:-}" ]; then
        mkdir -p "$SHOTS"
        DISPLAY=:94 import -window "$(DISPLAY=:94 xdotool search --name 'HybridX Streak' | head -1)" "$SHOTS/$1.png" 2>/dev/null
    fi
    cleanup; PIDS=()
    sleep 1
}
# Streak first, so it has left streak_now.json: each app then shows the Streak line after its
# save (hybridx-link StreakNow), counting what the apps before it saved.
echo "Opening Streak, to leave its summary of the week..."
open_streak Streak-0
# Form after each save (Streak NOTES "S10"): Streak's summary as one with two months of history
# would leave it, a mature load window and the moment of the save, so each app works out where
# form stands after its session. Streak's own state is not touched: its next open writes its own.
python3 - "$OUT/SharedData/HybridX/streak_now.json" <<'PY'
import json, sys, time
p = sys.argv[1]
d = json.load(open(p))
today = int(time.time() // 86400)   # the simulators' local day (the container runs in UTC)
d["at"] = int(time.time())
d["lw"] = [today - 60, today - 13, 4000, 4000, 1, [40] * 13 + [0]]
json.dump(d, open(p, "w"), separators=(",", ":"))
PY

if [ "$LONG" = 1 ]; then
    echo "Recording: X Training, Run, Race, Trail, Workout (three in turn) and Ride at once, about $SECS s each..."
    training_app & T=$!; run_app & A=$!; trail_app & B=$!; race_app & C=$!; workout_app & D=$!; ride_app & E=$!
    wait $T $A $B $C $D $E
else
    # One after another, so each Streak line counts the sessions saved before it.
    echo "Recording: X Training ($TSECS s), then Run, Race, Trail, Workout (Free, Strength, Circuit), then Ride, about $SECS s each..."
    training_app; run_app; race_app; trail_app; workout_app; ride_app
fi
echo "Opening X Run again, for X Training's minutes..."
run_again

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

# Each writer: its entries, its own name and sports, each the start and timer time of its .fit.
# Workout saves three (Free, Strength, Circuit), each with its own link sport: Workout 5,
# Strength 4, Hybrid 6 (Workout NOTES "W2"); the others one each.
# Edwards' TRIMP from a .fit's records, in the watch's default zones (hybridx-link TrainingLoad.hpp:
# above threshold i is zone i + 1, capped at 5).
TH = [95, 114, 133, 152, 171, 190]
def zone(hr):
    if not hr:
        return 0
    return min(5, sum(1 for t in TH if hr > t))
def fit_load(frames):
    secs = [0] * 6
    for f in frames:
        if f.name == "record" and f.has_field("heart_rate"):
            secs[zone(f.get_value("heart_rate"))] += 1
    weighted = sum(k * secs[k] for k in range(1, 6))
    return (weighted + 30) // 60, secs
loads = []   # every entry's load, for Streak's day

for app, slot, sports in (("Training", "training", [1]), ("Run", "run", [1]), ("Race", "race", [6]), ("Trail", "trail", [1]),
                          ("Workout", "workout", [5, 4, 6]), ("Ride", "ride", [2])):
    fits = sorted(glob.glob(f"{out}/{app}/Output/Activity/*/*.fit") + glob.glob(f"{out}/{app}/Output/Activity/*.fit"))
    path = f"{out}/SharedData/HybridX/act_{slot}.json"
    if not fits or not os.path.exists(path):
        check(f"{app} saved", False, f"{len(fits)} .fit, act_{slot}.json {'there' if os.path.exists(path) else 'missing'}")
        continue
    doc = json.load(open(path))
    entries = doc.get("a", [])
    check(f"{app} summary", doc.get("v") == 1 and doc.get("app") == f"HybridX{app}" and len(entries) == len(sports)
          and [e[6] for e in entries] == sports,
          f"{doc.get('app')}, {len(entries)} entr{'y' if len(entries) == 1 else 'ies'}, sport {[e[6] for e in entries]}")
    if len(fits) != len(entries):
        check(f"{app} = its .fit", False, f"{len(fits)} .fit files, {len(entries)} entries")
        continue
    for n, (fit, e) in enumerate(zip(fits, entries), start=1):
        with fitdecode.FitReader(fit) as r:
            frames = [f for f in r if isinstance(f, fitdecode.FitDataMessage)]
        sess = next(f for f in frames if f.name == "session")
        start = int(sess.get_value("start_time").timestamp())
        timer = sess.get_value("total_timer_time")
        label = f"{app} = its .fit" if len(entries) == 1 else f"{app} {n} = its .fit ({sess.get_value('sport')})"
        check(label, e[0] == start and abs(e[2] - timer) <= 1, f"start {e[0]} vs {start}, {e[2]} s vs {timer:.0f} s")
        want_load, secs = fit_load(frames)
        got_load = e[7] if len(e) > 7 else None
        loads.append(got_load or 0)
        lab = f"{app} load" if len(entries) == 1 else f"{app} {n} load"
        check(lab, got_load is not None and abs(got_load - want_load) <= max(1, want_load * 0.05),
              f"{got_load} vs {want_load} from the records (s in zones 1-5: {secs[1:]})")

def summaries(log):
    try:
        text = open(f"{out}/{log}.log", errors="replace").read()
    except OSError:
        return None
    m = re.search(r"Summaries: (\d+) files, (\d+) entries, (\d+) new, (\d+) counted", text)
    return tuple(map(int, m.groups())) if m else None

# The Streak line each app showed after its save (hybridx-link StreakNow). One after another (quick
# mode), each counts the sessions saved before it; at once (LONG=1), only its own is certain.
def streak_line(log):
    try:
        text = open(f"{out}/{log}.log", errors="replace").read()
    except OSError:
        return None
    m = re.findall(r'Streak line: "([^"]*)" "([^"]*)"', text)
    return m[-1] if m else None
# The seventh session meets the target of 7 (Streak's most), so its line, and the eighth's, also
# carry the streak: "1-week streak".
for n, app in enumerate(("Training", "Run", "Race", "Trail", "Workout-free", "Workout-strength", "Workout-circuit",
                         "Ride"), start=1):
    got = streak_line(app)
    if long_run:
        m = re.match(r"(\d) of 7 this week$", got[0]) if got else None
        check(f"{app}'s Streak line", m is not None and 1 <= int(m.group(1)) <= 8, f"{got}")
    else:
        want_line = (f"{n} of 7 this week", "1-week streak" if n >= 7 else "")
        check(f"{app}'s Streak line", got == want_line, f"{got}")

# Form after each save (Streak NOTES "S10"): the window put in Streak's summary above, plus every
# session saved so far (all today), replayed by a separate copy of Streak's model, in its words
# (LoadFormText.hpp formatFormNow). At once (LONG=1) only its shape is certain.
def form_step(avg, load, days):   # LoadHistory's integer step, as step() further down
    diff = load * 100 - avg
    half = days // 2
    delta = (diff + half) // days if diff >= 0 else -((-diff + half) // days)
    return max(0, avg + delta)
def form_word(fit, fat):
    if fit == 0 and fat == 0:
        return ""
    form, base = fit - fat, (fit if fit else 1)
    if form * 100 > 10 * base: return "fresh"
    if form * 100 >= -10 * base: return "steady"
    if form * 100 >= -30 * base: return "building"
    return "ease off"
def form_text(f, g, max_chars):   # Workout and Ride word it within 18 characters, the rest 21
    x = f - g
    whole = (x + 50) // 100 if x >= 0 else -((-x + 50) // 100)
    num = f"+{whole}" if whole > 0 else f"{whole}"
    word = form_word(f, g)
    for t in (f"Form now {num}, {word}", f"Form {num}, {word}"):
        if word and len(t) <= max_chars:
            return t
    return f"Form now {num}"
def streak_form(log):
    try:
        text = open(f"{out}/{log}.log", errors="replace").read()
    except OSError:
        return None
    m = re.findall(r'Streak form: "([^"]*)"', text)
    return m[-1] if m else None
so_far = 0
for n, app in enumerate(("Training", "Run", "Race", "Trail", "Workout-free", "Workout-strength", "Workout-circuit",
                         "Ride")):
    so_far += loads[n] if n < len(loads) else 0
    got = streak_form(app)
    if long_run:
        check(f"{app}'s form", got is not None and got.startswith("Form "), f"{got}")
        continue
    f, g = 4000, 4000
    for d in [40] * 13 + [so_far]:
        f, g = form_step(f, d, 42), form_step(g, d, 7)
    want = form_text(f, g, 18 if app.startswith(("Workout", "Ride")) else 21)
    check(f"{app}'s form", got == want, f"{got!r} (reference {want!r}, loads so far {so_far})")

first, second = summaries("Streak-1"), summaries("Streak-2")
check("Streak, first open", first is not None and first[:3] == (6, 8, 8), f"files, entries, new = {first[:3] if first else None}")
check("Streak, second open", second is not None and second[2] == 0 and second[3] == 8,
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
want = sorted([("HybridXTraining", "Run"), ("HybridXRace", "Hybrid"), ("HybridXRide", "Ride"), ("HybridXRun", "Run"),
               ("HybridXTrail", "Run"), ("HybridXWorkout", "Workout"), ("HybridXWorkout", "Strength"),
               ("HybridXWorkout", "Hybrid")])
c1, c2 = counted("streak-1.json"), counted("streak-2.json")
check("Streak counts each", c1 == want, ", ".join(f"{a} {k}" for a, k in c1) if c1 else "no state")
check("Streak, still once each", c2 == want, f"{len(c2) if c2 else 0} sessions after the second open")

# Training load in Streak: its state's "L" = [1, first day, first open day, fitness x100,
# fatigue x100, seeded, [14 daily loads]] (StateCodec). Streak first opened today (Streak-0), so
# the window ends today and today holds every session's load. The reference: Banister's model
# from nothing, 13 empty days then today's load, each day's step rounded half away from zero, as
# LoadHistory's integer one (an independent copy, here).
import time
def step(avg, load, days):
    diff = load * 100 - avg
    half = days // 2
    delta = (diff + half) // days if diff >= 0 else -((-diff + half) // days)
    return max(0, avg + delta)
try:
    st = json.load(open(f"{out}/streak-2.json"))
except (OSError, ValueError):
    st = {}
L = st.get("L")
today = int(time.time() // 86400)   # the simulators' local day (the container runs in UTC)
total = sum(loads)
if not L or len(L) != 7 or len(L[6]) != 14:
    check("Streak keeps the loads", False, f"no \"L\" in its state ({L})")
else:
    started, first, open_start, fit_x100, fat_x100, seeded, daily = L
    day_i = today - open_start
    got_today = daily[day_i] if 0 <= day_i < 14 else None
    check("Streak keeps the loads", started == 1 and first == today and open_start == today - 13
          and got_today == total and sum(daily) == total,
          f"today's load {got_today} == the summaries' {total} ({'+'.join(map(str, loads))}); "
          f"first day {first - today:+d}, window from {open_start - today:+d}")
    # What the screens show for today: LoadHistory::figures, from the state ...
    f, g = fit_x100, fat_x100
    for d in daily:
        f, g = step(f, d, 42), step(g, d, 7)
    # ... and the reference, from the summaries alone.
    rf, rg = 0, 0
    for d in [0] * 13 + [total]:
        rf, rg = step(rf, d, 42), step(rg, d, 7)
    check("Streak's fitness, fatigue, form", (f, g, f - g) == (rf, rg, rf - rg) and (total == 0 or rf > 0),
          f"fitness {f / 100:.2f}, fatigue {g / 100:.2f}, form {(f - g) / 100:+.2f} (reference "
          f"{rf / 100:.2f}, {rg / 100:.2f}, {(rf - rg) / 100:+.2f}); day 1 of 14, so the word is Learning")

# Trail's minutes for Run's VO2max: information only (the simulator's heart rate is not steady running).
vo2 = sorted(os.path.basename(p) for p in glob.glob(f"{out}/SharedData/HybridX/trail_vo2_*.json"))
rows.append(("info", "Trail -> Run VO2 windows", ", ".join(vo2) if vo2 else "none left (expected in the simulator)"))

# X Training's minutes for Run's VO2max (X Training TP4): its file names its run by the .fit's
# start, and X Run, opened once more after it, counts that run once, behind its own mark.
tfiles = sorted(glob.glob(f"{out}/SharedData/HybridX/training_vo2_*.json"))
tfits = sorted(glob.glob(f"{out}/Training/Output/Activity/*/*.fit") + glob.glob(f"{out}/Training/Output/Activity/*.fit"))
# Quick mode opens X Run after X Training's save, so its first open reads them; LONG=1 records
# at once, so the second does. Either way: once across both.
run2 = ""
for log in ("Run", "Run-2"):
    try:
        run2 += open(f"{out}/{log}.log", errors="replace").read()
    except OSError:
        pass
try:
    rh = json.load(open(f"{out}/Run/Output/vo2.json"))
except (OSError, ValueError):
    rh = {}
if not tfiles:
    tlog = open(f"{out}/Training.log", errors="replace").read() if os.path.exists(f"{out}/Training.log") else ""
    check("X Training -> Run VO2 windows", "nothing to leave for X Run" in tlog and rh.get("training", 0) == 0,
          "none left (the simulator's minutes not steady enough); X Run's mark stays 0")
else:
    tdoc = json.load(open(tfiles[0]))
    with fitdecode.FitReader(tfits[0]) as r:
        tsess = next(f for f in r if isinstance(f, fitdecode.FitDataMessage) and f.name == "session")
    tstart = int(tsess.get_value("start_time").timestamp())
    check("X Training's VO2 windows", len(tfiles) == 1 and tdoc.get("v") == 1 and tdoc.get("id") == tstart,
          f"{os.path.basename(tfiles[0])}: run {tdoc.get('id')} (the .fit's start {tstart}), "
          f"{len(tdoc.get('w', []))} windows, max HR {tdoc.get('max')}")
    seen = re.findall(rf"X Training run {tstart}: (\d+) windows", run2)
    check("X Run reads them once", len(seen) == 1 and rh.get("v") == 5 and rh.get("training") == tstart,
          f"logged {len(seen)} time(s); vo2.json v{rh.get('v')}, training mark {rh.get('training')}")

w = max(len(r[1]) for r in rows)
for status, name, detail in rows:
    print(f"{status:5} {name:<{w}}  {detail}")
print("Streak's own copy after the first open:", f"{out}/streak-1.json")
sys.exit(0 if ok else 1)
EOF
