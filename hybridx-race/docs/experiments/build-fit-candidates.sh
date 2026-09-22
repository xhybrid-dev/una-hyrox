#!/bin/bash
#
# Build the candidate FIT files for Jon to upload to Garmin Connect and Strava.
#
# These come out of the app's OWN ActivityWriter and RaceModel, driven over the
# SDK's kernel test doubles, so the file is what the watch writes rather than a
# stand-in that might differ (which is exactly how the first round went wrong:
# see NOTES.md 5.9).
#
# Two questions are still open, so four files, one per combination:
#
#   sport     training (10) or running (1)
#   distance  every metre the format states (10 480 m), or the runs only (8 km)
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

#          file                             sport sub runs-only
CANDIDATES=(
    "A-training-all-distances.fit  10 0 0"
    "B-running-all-distances.fit    1 0 0"
    "C-training-runs-only.fit      10 0 1"
    "D-running-runs-only.fit        1 0 1"
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
echo "  A vs B  does sport=training or sport=running label the race better?"
echo "  A vs C  are the stations' metres worth having, given what they do to pace?"
