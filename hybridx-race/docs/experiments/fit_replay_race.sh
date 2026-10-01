#!/bin/bash
# Build and run fit_replay_race.cpp, then decode the result.
# Usage:  UNA_SDK=/path/to/una-sdk ./fit_replay_race.sh [out.fit]
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
LIBS=$(cd "$HERE/../../Software/Libs" && pwd)
OUT=${1:-$HERE/fit-candidates/replay.fit}
: "${UNA_SDK:?UNA_SDK is not set}"
mkdir -p "$(dirname "$OUT")"
BIN=$(mktemp)
g++ -std=c++17 -O1 -w -o "$BIN" "$HERE/fit_replay_race.cpp" \
    "$LIBS/Sources/ActivityWriter.cpp" "$LIBS/Sources/RaceModel.cpp" \
    "$UNA_SDK/Libs/Source/Fit/FitWriter.cpp" "$UNA_SDK/Libs/Source/Fit/FitCrc.cpp" \
    "$UNA_SDK/Libs/Source/Fit/RecordingMarker.cpp" \
    "$UNA_SDK/Libs/Source/JSON/JsonStreamWriter.cpp" \
    "$UNA_SDK/Tests/Host/support/KernelTestDoubles.cpp" \
    "$UNA_SDK/Libs/Source/UnaLogger/Logger.cpp" \
    -I"$LIBS/Header" -I"$UNA_SDK/Libs/Header" -I"$UNA_SDK/Tests/Host"
"$BIN" "$OUT"
rm -f "$BIN"
python3 "$HERE/fit_decode_report.py" "$OUT"
