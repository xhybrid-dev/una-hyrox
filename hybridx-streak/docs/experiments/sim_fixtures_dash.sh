#!/bin/bash
#
# A pretend watch for the family dashboard (NOTES "S7"): the whole X family's shared
# files, as a real watch holds them, for the simulator's By sport and Your numbers
# screens. Every file is written by hand from its app's documented format, so this
# also checks the formats are enough to write one:
#   act_*.json            the activity link (hybridx-link/docs/ACTIVITY_LINK.md): this
#                         week and last week, from five apps, with distances;
#   training_today.json   X Training's TodayFile.hpp: a plan, with a session today;
#   vo2max.json           X Run's Vo2History toSharedJson (X Training RunVo2Max.hpp);
#   weighins.csv          X Weigh-in's log (WeighLog.hpp), three weeks of weigh-ins,
#   weighin_units.txt     and its unit (0 kg, 1 stones and pounds, 2 lb).
#
#   <root>/a/b/c/d/e/                 run the simulator from here
#   <root>/Output/                    HybridX Streak's own folder
#   <root>/SharedData/HybridX/        the family's files
#
# Times are this machine's local time. TODAY_STATE (session, done, rest) picks what
# X Training says of today; UNITS (0, 1, 2) X Weigh-in's unit; NO_FAMILY=1 leaves
# out everything but the activity link; BANKED=1 leaves this week exactly 3 sessions
# (the default target, no bonus), for Home's banked line.
# Usage:  sim_fixtures_dash.sh <root>
set -eu

ROOT=${1:?usage: sim_fixtures_dash.sh <root>}
TODAY_STATE=${TODAY_STATE:-session}
UNITS=${UNITS:-0}
rm -rf "$ROOT"
mkdir -p "$ROOT/a/b/c/d/e" "$ROOT/Output" "$ROOT/Settings" "$ROOT/SharedData/HybridX"
SHARED="$ROOT/SharedData/HybridX"

DOW=$(date +%u)                      # 1 = Monday
MON=$(date -d "-$((DOW - 1)) days" +%Y-%m-%d)
TODAY_OFF=$((DOW - 1))

Z=$(date +%z)
SIGN=${Z:0:1}; ZH=${Z:1:2}; ZM=${Z:3:2}
TZMIN=$((10#$ZH * 60 + 10#$ZM)); [ "$SIGN" = "-" ] && TZMIN=$((-TZMIN))
TZSTORED=$((TZMIN + 1440))

# entry <day offset from this Monday, may be negative> <HH:MM> <seconds> <metres> <avg> <max> <sport> <load>
entry() {
    local utc
    utc=$(date -d "$MON $1 days $2" +%s)
    printf '[%s,%s,%s,%s,%s,%s,%s,%s]' "$utc" "$TZSTORED" "$3" "$4" "$5" "$6" "$7" "$8"
}
file() {
    local slot=$1 app=$2; shift 2
    local body; body=$(IFS=,; echo "$*")
    printf '{"v":1,"app":"%s","a":[%s]}' "$app" "$body" > "$SHARED/act_$slot.json"
}
# A day this week, no later than today (so nothing is in the future).
d() { local x=$1; [ "$x" -gt "$TODAY_OFF" ] && x=$TODAY_OFF; echo "$x"; }

# Sport codes: 1 run, 2 ride, 3 walk, 4 strength, 5 workout, 6 hybrid, 7 row, 9 other.
# Last week: a run, a ride, a strength session. This week: two runs (one X Training's),
# a ride, a strength session and a race-format session.
file run HybridXRun "$(entry -6 07:10 2400 6800 148 168 1 38)" "$(entry 0 07:10 2100 6000 148 168 1 35)"
file ride HybridXRide "$(entry -4 17:30 3600 27500 132 158 2 52)" "$(entry "$(d 1)" 06:30 3840 24000 130 155 2 49)"
if [ "${BANKED:-0}" = 1 ]; then
    file workout HybridXWorkout "$(entry -2 12:00 2700 0 120 150 4 22)"
else
    file training HybridXTraining "$(entry "$(d 2)" 06:00 1860 5200 150 170 1 41)"
    file workout HybridXWorkout "$(entry -2 12:00 2700 0 120 150 4 22)" "$(entry "$(d 3)" 12:10 2520 0 122 152 4 24)"
fi
file race HybridXRace "$(entry "$TODAY_OFF" 18:00 4260 9000 160 182 6 95)"

if [ "${NO_FAMILY:-0}" != 1 ]; then
    ymd() { date -d "$MON $1 days" +%Y%m%d; }
    # X Training: a 10K plan that began four weeks ago; the file covers this Monday + 20 days.
    # Rows: [date, kind, status, run week, "what", "pace"]; kinds 0 rest, 1 easy, 3 long,
    # 5 tempo, 7 intervals; status 0 to come, 1 done.
    case "$TODAY_STATE" in
        done) TST=1 ;;
        *)    TST=0 ;;
    esac
    TK=5; TW="30 min"; TP="4:18/km"
    if [ "$TODAY_STATE" = rest ]; then TK=0; TW=""; TP=""; fi
    rows=""
    for off in $(seq 0 20); do
        if [ "$off" -eq "$TODAY_OFF" ]; then
            r="[$(ymd "$off"),$TK,$TST,5,\"$TW\",\"$TP\"]"
        else
            case $(( (off + 7 - TODAY_OFF) % 7 )) in
                1) r="[$(ymd "$off"),1,0,5,\"40 min\",\"5:40/km\"]" ;;
                3) r="[$(ymd "$off"),7,0,5,\"5 x 3 min\",\"3:58/km\"]" ;;
                5) r="[$(ymd "$off"),3,0,5,\"75 min\",\"5:50/km\"]" ;;
                *) r="[$(ymd "$off"),0,0,5,\"\",\"\"]" ;;
            esac
            [ "$off" -lt "$TODAY_OFF" ] && r="${r/,0,5,/,1,5,}"   # earlier days done
        fi
        rows="$rows${rows:+,}$r"
    done
    printf '{"v":1,"plan":1,"wks":10,"ps":0,"s":%s,"e":%s,"f":%s,"t":%s,"d":[%s]}' \
        "$(ymd -28)" "$(ymd 41)" "$(ymd 0)" "$(ymd 20)" "$rows" > "$SHARED/training_today.json"

    # X Run's VO2max.
    printf '{"v":2,"vo2maxX10":523,"runs":5,"utc":%s,"beforeX10":519,"trend":[505,512,519,523]}' \
        "$(date -d "$MON 0 days 07:45" +%s)" > "$SHARED/vo2max.json"

    # X Weigh-in: three weeks, drifting down from 80.6 kg.
    {
        echo "date,time,kg"
        for back in $(seq 20 -1 1); do
            kg=$(( 8060 - (20 - back) * 6 + (back % 3) * 15 ))
            printf '%s,07:%02d,%d.%02d\n' "$(date -d "-$back days" +%Y-%m-%d)" $(( back % 50 )) $((kg / 100)) $((kg % 100))
        done
    } > "$SHARED/weighins.csv"
    printf '%s\n' "$UNITS" > "$SHARED/weighin_units.txt"
fi

cat <<EOF
This week (from $MON): runs, a ride, a strength session and a race-format session, with
distances; last week: a run, a ride and a strength session. X Training: $TODAY_STATE today;
X Run: VO2max 52.3 from 5 runs; X Weigh-in: three weeks of weigh-ins (unit $UNITS).
Run the simulator from: $ROOT/a/b/c/d/e
EOF
