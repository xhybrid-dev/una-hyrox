#!/bin/bash
#
# A pretend watch for the HybridX Streak simulator as a REAL watch is: no Activity
# folders to read, only the summaries the HybridX apps leave in the shared folder
# (hybridx-link/docs/ACTIVITY_LINK.md). The files are written by hand here, from the
# format in that document, so this also checks that the document is enough to write
# one: no host tool, no library code.
#
#   <root>/a/b/c/d/e/                          run the simulator from here
#   <root>/Output/                             HybridX Streak's own folder
#   <root>/SharedData/HybridX/act_*.json       the summaries
#
# Times are this machine's local time; the zone offset is read from `date`.
# Usage:  sim_fixtures_link.sh <root>
set -eu

ROOT=${1:?usage: sim_fixtures_link.sh <root>}
rm -rf "$ROOT"
mkdir -p "$ROOT/a/b/c/d/e" "$ROOT/Output" "$ROOT/Settings" "$ROOT/SharedData/HybridX"
SHARED="$ROOT/SharedData/HybridX"

DOW=$(date +%u)                      # 1 = Monday
MON=$(date -d "-$((DOW - 1)) days" +%Y-%m-%d)
TODAY_OFF=$((DOW - 1))

# The zone offset in minutes, plus 1440 (the file holds no negative number).
Z=$(date +%z)                        # +0100
SIGN=${Z:0:1}; ZH=${Z:1:2}; ZM=${Z:3:2}
TZMIN=$((10#$ZH * 60 + 10#$ZM)); [ "$SIGN" = "-" ] && TZMIN=$((-TZMIN))
TZSTORED=$((TZMIN + 1440))

# entry <day offset from Monday> <HH:MM> <seconds> <metres> <avg hr> <max hr> <sport>
entry() {
    local utc
    utc=$(date -d "$MON $1 days $2" +%s)
    printf '[%s,%s,%s,%s,%s,%s,%s]' "$utc" "$TZSTORED" "$3" "$4" "$5" "$6" "$7"
}
# file <slot> <app> <entries...>
file() {
    local slot=$1 app=$2; shift 2
    local body; body=$(IFS=,; echo "$*")
    printf '{"v":1,"app":"%s","a":[%s]}' "$app" "$body" > "$SHARED/act_$slot.json"
}

# Sport codes: 1 run, 2 ride, 3 walk, 4 strength, 5 workout, 6 hybrid, 7 row, 9 other.
file run HybridXRun "$(entry 0 07:10 2100 6000 148 168 1)" "$(entry "$TODAY_OFF" 06:00 1800 4800 150 170 1)"
file race HybridXRace "$(entry 0 18:00 4260 9000 160 182 6)"
if [ "$TODAY_OFF" -ge 1 ]; then
    # An app that does not exist yet: a bike app, in the slot reserved for it.
    file ride HybridXRide "$(entry 1 06:30 3840 24000 130 155 2)"
fi
file workout HybridXWorkout "$(entry 0 12:00 300 0 0 0 5)"            # 5 minutes: under the 10-minute minimum
printf 'not a summary' > "$SHARED/act_trail.json"                     # a broken file is skipped

cat <<EOF
A first open should count, this week (a run on Monday, another today if it is not Monday):
  Run 35 min, Run 30 min (HybridXRun), Hybrid 71 min (HybridXRace)$( [ "$TODAY_OFF" -ge 1 ] && echo ", Ride 64 min (HybridXRide)")
and list, not counted: Workout 5 min (HybridXWorkout), under 10.
It should skip the broken act_trail.json. The log line is "Summaries: 4 files, ...".
Run the simulator from: $ROOT/a/b/c/d/e
EOF
