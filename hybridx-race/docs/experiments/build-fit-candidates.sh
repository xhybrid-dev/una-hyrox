#!/bin/bash
#
# Build the candidate FIT files for Jon to upload to Garmin Connect and Strava.
#
# These come out of the app's OWN ActivityWriter and RaceModel, driven over the
# SDK's kernel test doubles, so the file is what the watch writes rather than a
# stand-in that might differ (which is exactly how the first round went wrong:
# see NOTES.md 5.9).
#
# Round 4. Sport and distance are settled: running/generic, every stated metre,
# 10 480 m for a Full race. What these two files are for is the ONE question the
# earlier rounds could not answer, because every candidate carried the same
# hard-coded start time and Garmin Connect therefore saw one activity being
# re-uploaded rather than several to compare (NOTES.md 5.12).
#
# Each file now ends at the moment it is written and is spaced a day apart, so
# they land in Garmin as separate activities.
#
# The cardio sub_sport below is the public FIT data dictionary's, read from the
# profile tables fitdecode generates from Garmin's FIT SDK; it is NOT in
# SDK/Fit/FitProfile.hpp, whose SubSport enum stops at 6 (NOTES.md 5.11).
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

#          file                    sport sub runs-only days-ago
CANDIDATES=(
    "H-running-fresh.fit         1  0 0 0"   # the decided combination
    "J-cardio-fresh.fit         10 26 0 1"   # the control, a day earlier
)

echo
echo "== writing =="
rm -f "$OUT"/*.fit
for spec in "${CANDIDATES[@]}"; do
    # shellcheck disable=SC2086
    set -- $spec
    (cd "$OUT" && "$BIN" "$1" "$2" "$3" "$4" "$5")
done

echo
echo "== decoding =="
python3 "$HERE/fit_decode_report.py" "$OUT"/*.fit

rm -f "$BIN"
echo
echo "Upload both to Garmin Connect. They are a day apart, so they cannot"
echo "collide with each other or with anything uploaded before. What to check:"
echo "  H   filed as Running, and are the laps named (SKIERG, RUN 2/8, ...)?"
echo "  J   filed as anything OTHER than Running -- if it is, the sport field"
echo "      does reach Garmin and the earlier rounds were only ever colliding."
