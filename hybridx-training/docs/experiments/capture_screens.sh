#!/bin/bash
#
# X Training's screens in the PC simulator (TP3), by key presses, into
# docs/screens/tp3-*.png (or the folder given).
#
# Each scenario is a pretend watch: app_config.json (the phone's settings) and
# a plan.json made so that the host's today falls where the scenario needs it
# (before the plan starts, a rest day, a session day, the plan over, ...). The
# simulator reads the host's clock and zone. The service keeps a plan.json
# whose settings match the phone's, as on the watch.
#
# As sim_plan.sh: Xvfb for a display, xdotool for the buttons (keys 1..4 are
# L1, L2, R1, R2), ImageMagick's import for frames. Build the simulator first
# (its CMakeLists.txt). About 2 minutes.
#
# Usage:  UNA_SDK=/path/to/una-sdk ./capture_screens.sh [out-dir] [scenario ...]
#   scenarios: before rest session done finished noplan problem week notes settings newplan
#              phoneask phonekeep reps trial vo2 offer pause ask changes (default: all)
# vo2 (TP4) writes into docs/screens/tp4-*.png: X Run's VO2max on the Plan screen. offer, pause, ask
# and changes (TP5) write tp5-*.png: the plan's memory (a missed long run offered, a paused plan, a
# trial's 5K waiting for an answer, moving, skipping and pausing).
set -u
REPO=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:-$REPO/docs/screens}
shift || true
SCENARIOS=${*:-before rest session done finished noplan problem week notes settings newplan phoneask phonekeep reps trial vo2 offer pause ask changes}
SOFT="$REPO/Software"
BIN="$SOFT/Apps/LVGL-GUI/simulator/build/bin"
DISP=:96
LOG=${SIM_LOG:-/tmp/x-training-capture.log}
[ -x "$BIN/HybridXTrainingSimulator" ] || { echo "Build the simulator first ($BIN)" >&2; exit 1; }
mkdir -p "$OUT"

TODAY=$(date +%Y%m%d)
DOW=$(( $(date +%u) - 1 ))                       # 0 Monday .. 6 Sunday
MONDAY=$(date -d "$(date +%F) -$DOW days" +%Y%m%d)
monday_weeks() { date -d "$(date +%F) -$DOW days $1 weeks" +%Y%m%d; }   # this Monday +/- n weeks

Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
SIMPID=
cleanup() { [ -n "$SIMPID" ] && kill -9 "$SIMPID" 2>/dev/null; kill $XPID 2>/dev/null; }
trap cleanup EXIT
sleep 2

# The phone's settings: plan, 5K, days mask (bit 0 Monday), long run day 1..7, half (optional).
config() {
    local plan=$1 fivek=$2 days=$3 long=$4 half=${5:-}
    b() { [ $(( (days >> $1) & 1 )) = 1 ] && echo true || echo false; }
    cat > "$SOFT/Output/app_config.json" <<EOF
{"schema":1,"values":{"plan":$plan,"fiveK":"$fivek","fiveKRecent":true,"halfTime":"$half","raceDate":"","level":2,
"runMon":$(b 0),"runTue":$(b 1),"runWed":$(b 2),"runThu":$(b 3),"runFri":$(b 4),"runSat":$(b 5),
"runSun":$(b 6),"longRunDay":$long,"easyByHr":false}}
EOF
}
# plan.json for the 10K plan at level 2, 5K 22:00, from @start, with an optional log entry.
planfile() {
    local days=$1 long=$2 start=$3 log=${4:-}
    local extra=${5:-}
    printf '{"v":1,"s":[1,2,%d,%d,0,0,0,1320,%s,1,0,0],"c":[0,%s,0,10],"log":[%s],"st":%s%s}' \
        "$days" "$(( long - 1 ))" "$start" "$start" "$log" "${SETTLED:-$TODAY}" "$extra" > "$SOFT/Output/plan.json"
}

start_sim() {
    DISPLAY=$DISP stdbuf -oL -eL "$BIN/HybridXTrainingSimulator" > "$LOG" 2>&1 & SIMPID=$!
    for _ in $(seq 60); do
        grep -q "Today: status" "$LOG" && grep -q "Mock.Buzzer::play" "$LOG" && break
        sleep 1
    done
    sleep 2
    WID=$(DISPLAY=$DISP xdotool search --name 'X Training' | head -1)
    [ -n "$WID" ] || { echo "FATAL: no simulator window" >&2; exit 1; }
}
stop_sim() { kill -9 "$SIMPID" 2>/dev/null; wait "$SIMPID" 2>/dev/null; SIMPID=; }
key()  { DISPLAY=$DISP xdotool key "$1"; sleep "${2:-1}"; }
hold() { DISPLAY=$DISP xdotool keydown "$1"; sleep "$2"; DISPLAY=$DISP xdotool keyup "$1"; sleep 1; }
# Save a run: R1 the action menu, L2 to Save, R1 held (the confirm's hold).
save_run() { key 3 1; for _ in $(seq "$1"); do key 2; done; snap "$2-menu"; hold 3 3; }
snap() { DISPLAY=$DISP timeout 10 import -window "$WID" "$OUT/${SNAPPFX:-tp3}-$1.png" 2>/dev/null; echo "  ${SNAPPFX:-tp3}-$1"; }
fresh() { rm -rf "$SOFT/Output" "$SOFT/SharedData"; mkdir -p "$SOFT/Output"; }

# Four run days with today among them: today, +2, +3 and +5 days on; the long run +5.
DAYS_TODAY=0
for k in 0 2 3 5; do DAYS_TODAY=$(( DAYS_TODAY | (1 << ((DOW + k) % 7)) )); done
LONG_TODAY=$(( (DOW + 5) % 7 + 1 ))
# Today the long run: the time trial in a cut-back week.
DAYS_LONG=0
for k in 0 2 3 5; do DAYS_LONG=$(( DAYS_LONG | (1 << ((DOW + k) % 7)) )); done
LONG_LONG=$(( DOW + 1 ))
# The same days moved one on: today is a rest day.
DAYS_REST=0
for k in 1 3 4 6; do DAYS_REST=$(( DAYS_REST | (1 << ((DOW + k) % 7)) )); done
LONG_REST=$(( (DOW + 6) % 7 + 1 ))

cd "$BIN" || exit 1
for sc in $SCENARIOS; do
    echo "--- $sc"
    SNAPPFX=
    fresh
    case $sc in
    before)   # Jon's first install: the plan starts next Monday
        config 1 22:00 "$DAYS_TODAY" "$LONG_TODAY"
        planfile "$DAYS_TODAY" "$LONG_TODAY" "$(monday_weeks 1)"
        start_sim
        snap before-main; key 3; snap before-today ;;
    rest)
        config 1 22:00 "$DAYS_REST" "$LONG_REST"
        planfile "$DAYS_REST" "$LONG_REST" "$(monday_weeks -1)"
        start_sim
        snap rest-main; key 3; snap rest-today ;;
    session)  # week 5 of the 10K plan, today a quality day
        config 1 22:00 "$DAYS_TODAY" "$LONG_TODAY"
        planfile "$DAYS_TODAY" "$LONG_TODAY" "$(monday_weeks -4)"
        start_sim
        snap session-main; key 3; snap session-today
        key 2; snap session-step1; key 2; snap session-step2; key 2; snap session-step3
        key 1; key 1; key 1; key 3 2; snap session-countdown ;;
    done)
        config 1 22:00 "$DAYS_TODAY" "$LONG_TODAY"
        planfile "$DAYS_TODAY" "$LONG_TODAY" "$(monday_weeks -4)" "[$TODAY,7,1,0]"
        start_sim
        snap done-main; key 3; snap done-today ;;
    finished) # a 10-week plan that ended last week
        config 1 22:00 "$DAYS_TODAY" "$LONG_TODAY"
        planfile "$DAYS_TODAY" "$LONG_TODAY" "$(monday_weeks -11)"
        start_sim
        snap finished-main; key 3; snap finished-today ;;
    noplan)
        config 0 22:00 "$DAYS_TODAY" "$LONG_TODAY"
        start_sim
        snap noplan-main; key 3; snap noplan-today ;;
    week)     # week 5, two days logged before today: This week, a day, Plan, Paces
        config 1 22:00 "$DAYS_TODAY" "$LONG_TODAY"
        D2=$(date -d "$(date +%F) -2 days" +%Y%m%d)    # two days ago: done
        D4=$(date -d "$(date +%F) -4 days" +%Y%m%d)    # four days ago: part done
        planfile "$DAYS_TODAY" "$LONG_TODAY" "$(monday_weeks -4)" "[$D4,1,2,0],[$D2,1,1,0]"
        start_sim
        key 2; key 2; key 2; snap week-menu       # L2 x3: This week
        key 3 2; snap week-week                   # R1: This week
        key 2; snap week-week-next                # L2: the next row
        key 3 2; snap week-day                    # R1: that day
        key 2; snap week-day-step1                # L2: its first step
        key 4; key 4                              # R2: back to the week, then the menu
        key 2; key 3 2; snap week-plan            # L2: Plan; R1
        key 4; key 2; key 2; key 3 2; snap week-paces1   # R2; L2 x2 (Changes, Paces); R1
        key 2; snap week-paces2; key 2; snap week-paces3
        key 2; snap week-paces4 ;;                # no note for this runner: stays on the third
    notes)    # a 32:00 5K (threshold meets 10K pace) and a half well behind it
        config 1 32:00 "$DAYS_TODAY" "$LONG_TODAY" 2:40:00
        planfile "$DAYS_TODAY" "$LONG_TODAY" "$(monday_weeks -4)"
        start_sim
        key 2; key 2; key 2; key 2; key 2; key 2; key 3 2; snap notes-paces1
        key 2; snap notes-paces2; key 2; snap notes-paces3; key 2; snap notes-paces4 ;;
    settings) # the plan's three settings, race alerts turned through its values
        config 1 22:00 "$DAYS_TODAY" "$LONG_TODAY"
        planfile "$DAYS_TODAY" "$LONG_TODAY" "$(monday_weeks -4)"
        start_sim
        key 1; snap settings-menu; key 3 2
        key 2; snap settings-reps; key 3; snap settings-reps-on
        key 2; snap settings-easyhr
        key 2; snap settings-race; key 3; snap settings-race-first2k; key 3; snap settings-race-both
        echo "  settings.json: $(tr -d ' \n' < "$SOFT/Output/settings.json" | grep -oE '"(reps_by_distance|race_alerts)":[a-z0-9]+' | tr '\n' ' ')"
        echo "  plan.json reps by distance (6th): $(grep -oE '"s":\[[0-9,]+' "$SOFT/Output/plan.json")" ;;
    newplan)  # a plan under way; New plan on the watch: the half, a race, started
        config 1 22:00 "$DAYS_TODAY" "$LONG_TODAY"
        planfile "$DAYS_TODAY" "$LONG_TODAY" "$(monday_weeks -4)"
        start_sim
        key 2; key 2; key 2; key 2; key 3 2; snap newplan-plan    # L2 x4: Plan; R1
        key 3 2; snap newplan-menu                                # R1: New plan
        key 3; snap newplan-half                                  # R1 on Plan: the half
        key 2; key 3 2; snap newplan-fivek                        # Your 5K: the picker
        key 1; key 3; key 2; key 2; snap newplan-fivek-secs; key 3 2   # 21, then :02; saved
        key 2; key 2; key 3; snap newplan-level                   # Level: 3
        key 2; key 3 2; snap newplan-days; key 4 2                # Run days, back
        key 2; key 2; key 3 2; snap newplan-race                  # Race: the list
        key 2; key 2; key 2; key 2; key 2; key 2; key 2; key 2; key 2; key 2; key 2; key 2; key 2; key 2; key 2; key 2; key 2; key 2; key 2; key 2
        snap newplan-race-pick; key 3 2; snap newplan-race-set
        key 2; key 3 3; snap newplan-confirm                      # Next: the summary
        key 3 3; snap newplan-today                               # R1: started; Today
        echo "  app_config.json: $(tr -d ' \n' < "$SOFT/Output/app_config.json" | grep -oE '"(plan|fiveK|level|raceDate)":"?[^,"]*"?' | tr '\n' ' ')"
        echo "  plan.json: $(grep -oE '"(s|c)":\[[0-9,]+\]' "$SOFT/Output/plan.json" | tr '\n' ' ')" ;;
    plans)    # New plan: the three plans on offer (R1 on the Plan row cycles them)
        config 1 22:00 "$DAYS_TODAY" "$LONG_TODAY"
        planfile "$DAYS_TODAY" "$LONG_TODAY" "$(monday_weeks -4)"
        start_sim
        key 2; key 2; key 2; key 2; key 3 2; key 3 2; snap plans-10k   # Plan; New plan: 10K
        key 3; snap plans-half; key 3; snap plans-vo2 ;;               # R1: the half; R1: the VO2max block
    phoneask) # the phone asks for the half while the 10K is under way: start it
        config 2 22:00 "$DAYS_TODAY" "$LONG_TODAY"
        planfile "$DAYS_TODAY" "$LONG_TODAY" "$(monday_weeks -4)"
        start_sim
        snap phoneask-main; key 3 2; snap phoneask-today
        key 3 2; snap phoneask-ask; key 3 3; snap phoneask-started
        echo "  plan.json: $(grep -oE '"(s|c)":\[[0-9,]+\]' "$SOFT/Output/plan.json" | tr '\n' ' ')" ;;
    phonekeep) # the same ask, answered "keep mine": the phone's values go back to the 10K
        config 2 22:00 "$DAYS_TODAY" "$LONG_TODAY"
        planfile "$DAYS_TODAY" "$LONG_TODAY" "$(monday_weeks -4)"
        start_sim
        key 3 2; key 3 2; key 2; snap phonekeep-ask; key 3 3; snap phonekeep-kept
        echo "  app_config.json plan: $(tr -d ' \n' < "$SOFT/Output/app_config.json" | grep -oE '"plan":[0-9]')"
        echo "  plan.json: $(grep -oE '"(s|c)":\[[0-9,]+\]' "$SOFT/Output/plan.json" | tr '\n' ' ')" ;;
    reps)     # week 3: 6 x 400 m; a rep ended by hand, judged on the next step's card
        config 1 22:00 "$DAYS_TODAY" "$LONG_TODAY"
        planfile "$DAYS_TODAY" "$LONG_TODAY" "$(monday_weeks -2)"
        start_sim
        key 3 2; snap reps-today; key 3 8          # Today; R1: the countdown, then the warm-up's card
        snap reps-warmup; sleep 3; key 4 4         # the card gone, R2: the first rep (its card, 3 s)
        sleep 14; snap reps-rep1; key 4 1          # 400 m, ended by hand after about 15 s
        snap reps-card                             # the jog's card: "Last rep", "aim"
        grep -E "Rep judged" "$LOG" | tail -1 | sed 's/^ *//'
        sleep 3; snap reps-jog
        save_run 3 reps; sleep 1; snap reps-saved; sleep 4; snap reps-saved-after ;;
    trial)    # week 4: the 5K time trial; the warm-up ends only on a held R2
        config 1 22:00 "$DAYS_LONG" "$LONG_LONG"
        planfile "$DAYS_LONG" "$LONG_LONG" "$(monday_weeks -3)"
        start_sim
        key 3 2; snap trial-today; key 3 8         # Today; R1: the countdown, then the warm-up's card
        snap trial-warmup; sleep 3
        key 4 2; snap trial-tap                    # a tap: still the warm-up
        grep -cE "Planned session started" "$LOG" | sed 's/^/  sessions started: /'
        DISPLAY=$DISP xdotool keydown 4; sleep 0.5; snap trial-holding; sleep 1; DISPLAY=$DISP xdotool keyup 4; sleep 0.5
        snap trial-card; sleep 4; snap trial-trial
        save_run 3 trial; sleep 1; snap trial-saved ;;
    problem)  # a plan, but no 5K
        config 1 "" "$DAYS_TODAY" "$LONG_TODAY"
        start_sim
        snap problem-main; key 3; snap problem-today ;;
    vo2)      # TP4: X Run's VO2max (its vo2max.json) on the Plan screen
        vo2file() {   # X Run's public file, as its Vo2History::toSharedJson writes it
            mkdir -p "$SOFT/SharedData/HybridX"
            printf '{"v":2,"vo2maxX10":%d,"runs":6,"utc":%d,"beforeX10":%d,"trend":[440,446,%d]}' \
                "$1" "$(date +%s)" "$(( $1 - 4 ))" "$1" > "$SOFT/SharedData/HybridX/vo2max.json"
        }
        # The 10K plan, week 5, no figure kept yet: the service keeps 45.2 as the plan's own.
        config 1 22:00 "$DAYS_TODAY" "$LONG_TODAY"
        planfile "$DAYS_TODAY" "$LONG_TODAY" "$(monday_weeks -4)"
        vo2file 452
        start_sim
        key 2; key 2; key 2; key 2; key 3 2; SNAPPFX=tp4 snap vo2-plan10k
        grep -o '"vo2s":[0-9]*' "$SOFT/Output/plan.json" | sed 's/^/  plan.json /'
        stop_sim
        # The VO2max block, week 4 of 8, with 44.6 kept from its start: the second measure.
        fresh
        config 3 22:00 "$DAYS_TODAY" "$LONG_TODAY"
        printf '{"v":1,"s":[3,2,%d,%d,0,0,0,1320,%s,1,0,0],"c":[0,%s,0,8],"log":[],"vo2s":446}' \
            "$DAYS_TODAY" "$(( LONG_TODAY - 1 ))" "$(monday_weeks -3)" "$(monday_weeks -3)" > "$SOFT/Output/plan.json"
        vo2file 452
        start_sim
        key 2; key 2; key 2; key 2; key 3 2; SNAPPFX=tp4 snap vo2-planblock ;;
    offer)    SNAPPFX=tp5; # TP5: yesterday's long run was missed; today rests: it is offered, and accepted
        # Run days: yesterday (the long run), then +1, +2 and +4 days; the plan began four weeks ago and
        # every session before yesterday is logged done.
        Y=$(( (DOW + 6) % 7 )); D_OFFER=0
        for k in 6 1 2 4; do D_OFFER=$(( D_OFFER | (1 << ((DOW + k) % 7)) )); done
        YD=$(date -d "$(date +%F) -1 day" +%F)
        START=$(date -d "$YD -$(( $(date -d "$YD" +%u) - 1 )) days -4 weeks" +%Y%m%d); LOGS=""
        for n in $(seq 0 40); do
            d=$(date -d "$START +$n days" +%Y%m%d); yd=$(date -d "$YD" +%Y%m%d)
            [ "$d" -ge "$yd" ] && break
            wd=$(( $(date -d "$START +$n days" +%u) - 1 ))
            [ $(( (D_OFFER >> wd) & 1 )) = 1 ] && LOGS="${LOGS:+$LOGS,}[$d,1,1,0]"
        done
        config 1 22:00 "$D_OFFER" "$(( Y + 1 ))"
        SETTLED=$(date -d "$(date +%F) -1 day" +%Y%m%d) planfile "$D_OFFER" "$(( Y + 1 ))" "$START" "$LOGS"
        start_sim
        snap offer-main; key 3 2; snap offer-today
        key 3 2; snap offer-ask; key 3 2; snap offer-yes
        echo "  plan.json log tail: $(grep -oE '\[[0-9]{8},[0-9],[0-9],[0-9]+\]' "$SOFT/Output/plan.json" | tail -2 | tr '\n' ' ')"
        echo "  plan.json ov: $(grep -oE '"ov":\[\[[0-9,]+\]\]' "$SOFT/Output/plan.json")" ;;
    pause)    SNAPPFX=tp5; # TP5: a plan paused three days ago: Today says so; R1 resumes it
        config 1 22:00 "$DAYS_TODAY" "$LONG_TODAY"
        planfile "$DAYS_TODAY" "$LONG_TODAY" "$(monday_weeks -4)" "" ",\"ps\":$(date -d "$(date +%F) -3 days" +%Y%m%d)"
        start_sim
        snap pause-main; key 3 2; snap pause-today
        key 3 2; snap pause-resumed
        echo "  plan.json: $(grep -oE '"(ps|st|adj)":[^]]*' "$SOFT/Output/plan.json" | tr '\n' ' ')" ;;
    ask)      SNAPPFX=tp5; # TP5: a time trial's 5K (22:50, slower than the 22:00) waits for "update or keep"
        config 1 22:00 "$DAYS_TODAY" "$LONG_TODAY"
        planfile "$DAYS_TODAY" "$LONG_TODAY" "$(monday_weeks -4)" "" ",\"ak\":1370"
        start_sim
        snap ask-main; key 3 2; snap ask-today
        key 3 2; snap ask-choice; key 3 2; snap ask-updated
        echo "  app_config.json fiveK: $(tr -d ' \n' < "$SOFT/Output/app_config.json" | grep -oE '"fiveK":"[^"]*"')"
        echo "  plan.json: $(grep -oE '"(s|ak)":[^]]*' "$SOFT/Output/plan.json" | tr '\n' ' ')" ;;
    changes)  SNAPPFX=tp5; # TP5: the Changes menu: today's session to tomorrow, skipped, the plan paused, ended
        # Run days with tomorrow among them: today, +1, +3 and +5 days on; the long run +5.
        D_MOVE=0
        for k in 0 1 3 5; do D_MOVE=$(( D_MOVE | (1 << ((DOW + k) % 7)) )); done
        config 1 22:00 "$D_MOVE" "$(( (DOW + 5) % 7 + 1 ))"
        planfile "$D_MOVE" "$(( (DOW + 5) % 7 + 1 ))" "$(monday_weeks -4)"
        start_sim
        key 2; key 2; key 2; key 2; key 2; snap changes-main     # L2 x5: Changes
        key 3 2; snap changes-menu; key 2; snap changes-move; key 2; snap changes-skip
        key 2; snap changes-pause; key 2; snap changes-end; key 3 1; snap changes-end-ask
        key 1; key 1; key 1; key 1; key 1                          # back to the first
        key 3 2; snap changes-tomorrow                             # tomorrow's session today: Today
        echo "  plan.json ov: $(grep -oE '"ov":\[\[[0-9,]+\],?\[?[0-9,]*\]*\]' "$SOFT/Output/plan.json")"
        key 4 2; key 3 2; key 2; key 2; key 3 2; snap changes-skipped   # Main; Changes; Skip today
        echo "  plan.json log: $(grep -oE '\[[0-9]{8},[0-9]+,4,[0-9]+\]' "$SOFT/Output/plan.json" | tr '\n' ' ')" ;;
    *) echo "unknown scenario $sc" >&2 ;;
    esac
    grep -E "Today:" "$LOG" | tail -1 | sed 's/^ *//'
    stop_sim
done
fresh
echo "--- screens in $OUT"
