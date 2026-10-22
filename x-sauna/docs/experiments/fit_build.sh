#!/bin/bash
# Build Tests/Host/tools/make_fit.cpp over the SDK's kernel test doubles.
# Usage:  UNA_SDK=/path/to/una-sdk ./fit_build.sh <out-binary>
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
LIBS=$(cd "$HERE/../../Software/Libs" && pwd)
TESTS=$(cd "$HERE/../../Tests/Host" && pwd)
: "${UNA_SDK:?UNA_SDK is not set}"
g++ -std=c++17 -O1 -w -o "$1" "$TESTS/tools/make_fit.cpp" \
    "$LIBS/App/Sources/ActivityWriter.cpp" \
    "$UNA_SDK/Libs/Source/Fit/FitWriter.cpp" "$UNA_SDK/Libs/Source/Fit/FitCrc.cpp" \
    "$UNA_SDK/Libs/Source/Fit/RecordingMarker.cpp" \
    "$UNA_SDK/Libs/Source/JSON/JsonStreamWriter.cpp" \
    "$UNA_SDK/Tests/Host/support/KernelTestDoubles.cpp" \
    "$UNA_SDK/Libs/Source/UnaLogger/Logger.cpp" \
    -I"$LIBS/App/Header" -I"$LIBS/Core/Header" -I"$UNA_SDK/Libs/Header" -I"$UNA_SDK/Tests/Host"
