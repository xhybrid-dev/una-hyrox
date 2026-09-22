#!/bin/bash
#
# D2: which FIT sport / sub_sport should a HYROX race carry?
#
# Strava and Garmin Connect decide what to call an activity, what icon to give
# it and which of their own charts to show from these two enums, and nothing in
# the file tells them what a HYROX race is. The only way to answer it is to
# upload the same race several ways and look, which is what this produces.
#
# Every file below is the SAME simulated race -- a Full race, Roxzone off, 16
# segments, 1 Hz heart rate, the three segment developer fields -- differing
# only in the session's sport and sub_sport.
#
# The values come from SDK/Fit/FitProfile.hpp and nowhere else. That header
# declares Sport { Generic 0, Running 1, Cycling 2, Training 10, Walking 11,
# Hiking 17 } and SubSport { Generic 0, Treadmill 1, Street 2, Trail 3, Track 4,
# IndoorCycling 6 }. The public FIT profile has values that might suit a race
# better -- fitness_equipment, hiit, cardio_training -- but their numbers are
# not in this SDK, and CLAUDE.md forbids inventing FIT profile numbers. If Jon
# wants those tested he needs to confirm the numbers first.
#
# Usage:  UNA_SDK=/path/to/una-sdk ./build-fit-candidates.sh
# Needs:  g++, python3, pip install fitdecode
#
set -euo pipefail

HERE=$(cd "$(dirname "$0")" && pwd)
OUT="$HERE/fit-candidates"

if [ -z "${UNA_SDK:-}" ]; then
    echo "UNA_SDK is not set." >&2
    exit 1
fi

mkdir -p "$OUT"
BIN="$OUT/lapbatch"

echo "== building =="
g++ -std=c++17 -O1 -o "$BIN" "$HERE/batched_laps.cpp" \
    "$UNA_SDK/Libs/Source/Fit/FitWriter.cpp" \
    "$UNA_SDK/Libs/Source/Fit/FitCrc.cpp" \
    "$UNA_SDK/Tests/Host/support/KernelTestDoubles.cpp" \
    "$UNA_SDK/Libs/Source/UnaLogger/Logger.cpp" \
    -I"$UNA_SDK/Libs/Header" -I"$UNA_SDK/Tests/Host"

# name                        sport  sub_sport
# Sport::Training=10, Sport::Running=1, SubSport::Generic=0, SubSport::Track=4
CANDIDATES=(
    "race-training-generic.fit 10 0"
    "race-running-generic.fit   1 0"
    "race-running-track.fit     1 4"
)

echo
echo "== writing =="
for spec in "${CANDIDATES[@]}"; do
    # shellcheck disable=SC2086
    set -- $spec
    (cd "$OUT" && "$BIN" "$1" "$2" "$3")
done

echo
echo "== decoding =="
for spec in "${CANDIDATES[@]}"; do
    # shellcheck disable=SC2086
    set -- $spec
    echo "-- $1"
    python3 "$HERE/batched_laps_decode.py" "$OUT/$1"
    echo
done

rm -f "$BIN"
echo "Candidates in $OUT -- upload each to Strava and Garmin Connect and see"
echo "which one they label most usefully. Whichever wins becomes the default in"
echo "ActivityWriter::TrackData (sport, subSport), which already takes them as"
echo "parameters."
