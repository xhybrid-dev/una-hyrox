#!/bin/bash
#
# Screenshots of S9, form and fitness up front (NOTES "S9"), in the REAL HybridX Streak app, cut
# to the watch's round face, into docs/screens/real/ (s9-*.png).
#
# Pretend watches (sim_fixtures_dash.sh, plus make_state's 16-week block):
#   A  the week found, then one new run on a second open: its toast, then "Form now ...";
#      L1 to the Form page, R1 to the graphs, R2 back, L2 back to the mountain, R1 to the menu
#      (Training load first);
#   B  "Opens on: Form" set on the phone (app_config.json): the first open has news, so it stays
#      on the mountain; the second has none, so it opens on the Form page;
#   C  Settings on the watch: "Opens on" switched by R1, and app_config.json written;
#   D  a first week: the Form page says "Learning".
# Each run's log is checked.
#
# Build first: the real simulator (Software/Apps/LVGL-GUI/simulator/build) and build-tests'
# make_state. Needs Xvfb, xdotool and ImageMagick. About three minutes.
# Usage:  ./capture_form.sh [output-dir]
set -u

APP=$(cd "$(dirname "$0")/../.." && pwd)
BIN="$APP/Software/Apps/LVGL-GUI/simulator/build/bin/HybridXStreakSimulator"
OUT=${1:-$APP/docs/screens/real}
DISP=:95
WORK=$(mktemp -d)
mkdir -p "$OUT"
FAIL=0

convert -size 520x520 xc:'#1b1f22' -fill black -draw "circle 259.5,259.5 259.5,20" -transparent black \
    -fill none -stroke '#3a4044' -strokewidth 5 -draw "circle 259.5,259.5 259.5,17" "$WORK/bezel.png"

Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
sleep 2

run() {   # run <tree> [log name]
    cd "$1/a/b/c/d/e"
    DISPLAY=$DISP "$BIN" > "$1/${2:-sim}.log" 2>&1 & SIMPID=$!
    sleep 0.8
    WID=$(DISPLAY=$DISP xdotool search --name 'HybridX Streak' | head -1)
}
stop() { DISPLAY=$DISP xdotool key Escape; sleep 1; kill $SIMPID 2>/dev/null; }
key() { DISPLAY=$DISP xdotool key "$1"; sleep "${2:-1.1}"; }
snap() {
    DISPLAY=$DISP import -window "$WID" "$WORK/raw.png" 2>/dev/null
    convert "$WORK/raw.png" -background black -gravity center -extent 520x520 "$WORK/bezel.png" -composite "$OUT/$1.png"
}
expect() {   # expect <log> <pattern> <what>
    if grep -q "$2" "$1"; then echo "PASS  $3"; else echo "FAIL  $3 (no \"$2\" in $1)"; FAIL=1; fi
}
config() {   # config <tree> <opensOnLoad true|false>: the phone's settings, the defaults otherwise
    printf '{"schema":1,"values":{"weeklyTarget":3,"weekStart":1,"counts":0,"minMinutes":10,"onePerDay":false,"opensOnLoad":%s}}' \
        "$2" > "$1/Output/app_config.json"
}
new_run() {   # new_run <tree>: one more run in X Run's summary, an hour ago
    python3 - "$1/SharedData/HybridX/act_run.json" <<'PY'
import json, sys, time
p = sys.argv[1]
f = json.load(open(p))
now = int(time.time()) - 3600
tz = time.localtime(now).tm_gmtoff // 60 + 1440
f["a"].append([now, tz, 2700, 8200, 152, 174, 1, 70])
f["a"].sort(key=lambda e: e[0])
open(p, "w").write(json.dumps(f, separators=(",", ":")))
PY
}

# -- A: a new run, its form, and the two pages ---------------------------------------------------
A="$WORK/a"
bash "$APP/docs/experiments/sim_fixtures_dash.sh" "$A" >/dev/null
"$APP/build-tests/make_state" "$A/Output/state.json" 16 0 block >/dev/null
run "$A" first; sleep 19; stop            # the week's news, played and saved
new_run "$A"
run "$A"
sleep 1.6;  snap s9-01-toast-session
sleep 2.4;  snap s9-02-toast-form
sleep 3.5;  snap s9-03-home-mountain
key 1 1.5;  snap s9-04-form-page
key 3 1.5;  snap s9-05-graph-from-form
key 4 1.5;  snap s9-06-back-to-form
key 2;      snap s9-07-back-to-mountain
key 3;      snap s9-08-menu-load-first
key 4; stop
expect "$A/sim.log" "Summaries: 5 files, 9 entries, 1 new" "A: the second open finds the one new run"
expect "$A/sim.log" "Series: 84 days" "A: the Form page asks for the graphs' days"

# -- B: "Opens on: Form" from the phone ------------------------------------------------------------
B="$WORK/b"
bash "$APP/docs/experiments/sim_fixtures_dash.sh" "$B" >/dev/null
"$APP/build-tests/make_state" "$B/Output/state.json" 16 0 block >/dev/null
config "$B" true
run "$B" first
sleep 3;    snap s9-10-opens-on-form-news-first   # this week's news plays on the mountain first
sleep 16; stop
run "$B"
sleep 2.5;  snap s9-11-opens-on-form              # nothing new: straight to the Form page
stop
expect "$B/sim.log" "Summaries: 5 files, 8 entries, 0 new" "B: the second open has no news"

# -- C: Settings on the watch ----------------------------------------------------------------------
C="$WORK/c"
bash "$APP/docs/experiments/sim_fixtures_dash.sh" "$C" >/dev/null
config "$C" false
run "$C"
sleep 17
key 3; for i in 1 2 3 4 5 6; do key 2 0.6; done   # the menu's last row: Settings
key 3; for i in 1 2 3 4 5; do key 2 0.6; done     # Settings' last row: Opens on
sleep 0.5;  snap s9-20-settings-opens-on-mountain
key 3 1.5;  snap s9-21-settings-opens-on-form
key 4; key 4; stop
if grep -q '"opensOnLoad":true' "$C/Output/app_config.json"; then echo "PASS  C: the watch's setting is saved for the phone"; else
    echo "FAIL  C: app_config.json: $(cat "$C/Output/app_config.json")"; FAIL=1; fi

# -- D: a first week -----------------------------------------------------------------------------
D="$WORK/d"
bash "$APP/docs/experiments/sim_fixtures_dash.sh" "$D" >/dev/null
run "$D"
sleep 17
key 1 1.5;  snap s9-30-form-page-learning
key 2; stop

kill $XPID 2>/dev/null
echo "Screens in $OUT (s9-*.png); logs in $WORK"
exit $FAIL
