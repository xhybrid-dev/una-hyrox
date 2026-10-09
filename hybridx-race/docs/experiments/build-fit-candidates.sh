#!/bin/bash
#
# Build the candidate FIT files for Jon to upload to Garmin Connect and Strava.
#
# These come out of the app's OWN ActivityWriter and RaceModel, driven over the
# SDK's kernel test doubles, so the file is what the watch writes rather than a
# stand-in that might differ (which is exactly how the first round went wrong:
# see NOTES.md 5.9).
#
# Round 5: what decides the title and type Strava shows? Built after Jon saw
# the built-in apps' activities arrive as "generic - UNA Watch" and
# "running - UNA Watch" (NOTES.md 5.28). One variable changes per file; each ends
# at the moment it is written and is spaced a day apart, so they land as
# separate activities (the flaw that spoiled rounds 1-3, NOTES.md 5.12).
#
# session.sport_profile_name (field 110, string) is NOT in SDK/Fit/FitProfile.hpp.
# Its number, like fitness_equipment (sport 4) and hiit (62) if they are ever
# wanted, is the public FIT profile's, read from the tables fitdecode generates
# from Garmin's FIT SDK, and the report below decodes it back by name.
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

#  file                              sport sub runs-only days-ago run-m workout   profile        product
CANDIDATES=(
    "N1-running-as-the-app-is.fit         1    0 0 0 1000 -         -              -"
    "N2-training.fit                     10    0 0 1 1000 -         -              -"
    "N3-running-profile-name.fit          1    0 0 2 1000 -         HybridX-Race  -"
    "N4-hybridx-everywhere.fit            1    0 0 3 1000 HybridX   HybridX        HybridX"
)

echo
echo "== writing =="
rm -f "$OUT"/N*.fit   # earlier rounds' files (H, J, K) are evidence; leave them
for spec in "${CANDIDATES[@]}"; do
    # shellcheck disable=SC2086
    set -- $spec
    (cd "$OUT" && "$BIN" "$1" "$2" "$3" "$4" "$5" "$6" "$7" "$8" "$9")
done

echo
echo "== what is in each file's naming fields =="
python3 -I "$HERE/fit_naming_report.py" "$OUT"/N*.fit

echo
echo "== decoding =="
python3 "$HERE/fit_decode_report.py" "$OUT"/N*.fit

rm -f "$BIN"
echo
echo "Upload all four to Strava by hand, in this order. A day apart, they cannot"
echo "collide. For each, note the TITLE and the ACTIVITY TYPE Strava shows."
echo "  N1  what the app writes today"
echo "  N2  sport = training: does the type change to Workout?"
echo "  N3  sport_profile_name set: does the title or type pick it up?"
echo "  N4  HybridX in the workout name, profile name and product name"
