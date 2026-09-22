#!/bin/bash
#
# Build the candidate FIT files for Jon to upload to Garmin Connect and Strava.
#
# These come out of the app's OWN ActivityWriter and RaceModel, driven over the
# SDK's kernel test doubles, so the file is what the watch writes rather than a
# stand-in that might differ (which is exactly how the first round went wrong:
# see NOTES.md 5.9).
#
# Round 3. Distance is settled -- Jon confirmed on 22 September 2026 that every
# stated metre works, 10 480 m for a Full race -- and every step is now named, so
# the one open question is what a HYROX race should call itself.
#
# The sub_sport values below are the public FIT data dictionary's, read from the
# profile tables fitdecode generates from Garmin's FIT SDK. Two of them are NOT
# in SDK/Fit/FitProfile.hpp, whose SubSport enum stops at 6 (NOTES.md 5.11).
#
# Usage:  UNA_SDK=/path/to/una-sdk ./build-fit-candidates.sh
# Needs:  g++, python3, pip install fitdecode
#
set -euo pipefail

HERE=$(cd "$(dirname "$0")" && pwd)
LIBS=$(cd "$HERE/../../Software/Libs" && pwd)
OUT="$HERE/fit-candidates"

if [ -z "${UNA_SDK:-}" ]; then
    echo "UNA_SDK is not set." >&2
    exit 1
fi

mkdir -p "$OUT"
BIN="$OUT/fitsample"

echo "== building =="
# -w: the SDK's own JsonStreamWriter.cpp has printf-format warnings we neither
# own nor may fix; our sources are built warning-clean by the real build.
g++ -std=c++17 -O1 -w -o "$BIN" "$HERE/fit_race_sample.cpp" \
    "$LIBS/Sources/ActivityWriter.cpp" \
    "$LIBS/Sources/RaceModel.cpp" \
    "$UNA_SDK/Libs/Source/Fit/FitWriter.cpp" \
    "$UNA_SDK/Libs/Source/Fit/FitCrc.cpp" \
    "$UNA_SDK/Libs/Source/Fit/RecordingMarker.cpp" \
    "$UNA_SDK/Libs/Source/JSON/JsonStreamWriter.cpp" \
    "$UNA_SDK/Tests/Host/support/KernelTestDoubles.cpp" \
    "$UNA_SDK/Libs/Source/UnaLogger/Logger.cpp" \
    -I"$LIBS/Header" -I"$UNA_SDK/Libs/Header" -I"$UNA_SDK/Tests/Host"

#          file                    sport sub runs-only
CANDIDATES=(
    "E-running-named.fit         1  0 0"   # what Jon liked, now with lap names
    "F-cardio-named.fit         10 26 0"   # training / cardio_training
    "G-hiit-named.fit           10 70 0"   # training / hiit
)

echo
echo "== writing =="
rm -f "$OUT"/*.fit
for spec in "${CANDIDATES[@]}"; do
    # shellcheck disable=SC2086
    set -- $spec
    (cd "$OUT" && "$BIN" "$1" "$2" "$3" "$4")
done

echo
echo "== decoding =="
python3 "$HERE/fit_decode_report.py" "$OUT"/*.fit

rm -f "$BIN"
echo
echo "Upload each to Garmin Connect and Strava. What to compare:"
echo "  all three  do the laps come through named (SKIERG, RUN 2/8, ...)?"
echo "  E vs F/G   does Running, Cardio or HIIT describe the race best, and"
echo "             does the choice cost anything -- pace, zones, training load?"
