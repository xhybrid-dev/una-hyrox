#!/bin/bash
#
# Build a pretend watch for the HybridX Streak simulator: several apps'
# Activity folders holding real FIT files (written by the SDK's FitWriter via
# Tests/Host's make_fit), dated around today, plus the awkward cases.
#
# The simulator's file system is rooted at "../../../../../Output/" from its
# working directory, and ".." from there is plain host-directory traversal
# (una-sdk Simulator/Kernel.cpp), so the tree is:
#
#   <root>/a/b/c/d/e/       run the simulator from here
#   <root>/Output/          HybridX Streak's own folder (state.json lands here)
#   <root>/Running/Activity/YYYYMM/activity_*.fit   ... and the other apps
#   <root>/SharedData/HybridX/streak.json           the public copy
#
# Usage:  sim_fixtures.sh <root>      (needs build-tests/make_fit built)
# Prints what a first open should find.
set -eu

ROOT=${1:?usage: sim_fixtures.sh <root>}
APP=$(cd "$(dirname "$0")/../.." && pwd)
MAKE_FIT="$APP/build-tests/make_fit"
[ -x "$MAKE_FIT" ] || { echo "Build $MAKE_FIT first (cmake --build hybridx-streak/build-tests)" >&2; exit 1; }

rm -rf "$ROOT"
mkdir -p "$ROOT/a/b/c/d/e" "$ROOT/Output" "$ROOT/SharedData" "$ROOT/Settings"

# Monday of this week (weeks start on Monday by default, S1).
DOW=$(date +%u)                      # 1 = Monday
MON=$(date -d "-$((DOW - 1)) days" +%Y-%m-%d)
day() { date -d "$MON $1 days" +%Y%m%d; }
month() { date -d "$MON $1 days" +%Y%m; }

# fit <app> <sport> <sub> <day offset from Monday> <HHMMSS> <minutes>
fit() {
    local app=$1 sport=$2 sub=$3 off=$4 hms=$5 min=$6
    local dir="$ROOT/$app/Activity/$(month "$off")"
    mkdir -p "$dir"
    "$MAKE_FIT" "$dir/activity_$(day "$off")T$hms.fit" "$sport" "$sub" "$(day "$off")T$hms" "$min"
    echo "$dir/activity_$(day "$off")T$hms.fit"
}

TODAY_OFF=$((DOW - 1))
echo "Monday $MON, today is day +$TODAY_OFF"

# This week (FitProfile sports: Running 1, Cycling 2, Walking 11, Hiking 17, Generic 0).
fit Running     1 0 0 071000 35   >/dev/null   # Run, 35 min
fit HybridXRace 1 0 0 180000 71   >/dev/null   # Race writes Running/Generic: a Hybrid session
if [ "$TODAY_OFF" -ge 1 ]; then
    fit Cycling 2 0 1 063000 64   >/dev/null   # Ride, 64 min
fi
fit Hiking      11 0 "$TODAY_OFF" 120000 6 >/dev/null   # a 6-minute walk: under the 10-minute minimum

# Being recorded right now: named in Hiking's .recording marker.
REC=$(fit Hiking 17 0 "$TODAY_OFF" 130000 90)
printf 'Activity/%s/%s\n4096\n' "$(month "$TODAY_OFF")" "$(basename "$REC")" > "$ROOT/Hiking/Activity/.recording"

# A broken file: half-written, never finalised.
mkdir -p "$ROOT/Treadmill/Activity/$(month 0)"
printf 'not a finished FIT file' > "$ROOT/Treadmill/Activity/$(month 0)/activity_$(day 0)T200000.fit"

# Last week: found by later opens' catch-up, never by a first open (S15).
fit Workout 0 0 -3 063000 45 >/dev/null

cat <<EOF
A first open should count, this week:
  Run 35 min (Running), Hybrid 71 min (HybridXRace)$( [ "$TODAY_OFF" -ge 1 ] && echo ", Ride 64 min (Cycling)")
and list, not counted:
  Walk 6 min (Hiking) -- under 10
and skip: the Hiking file being recorded, the broken Treadmill file, last week's Workout.
Run the simulator from: $ROOT/a/b/c/d/e
EOF
