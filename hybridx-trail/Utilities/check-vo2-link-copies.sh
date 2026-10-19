#!/bin/bash
# Fails if HybridX Trail's copies of Run's window builder, file codec and
# SafeFile differ from Run's. Run's are the originals: change them there, then
#   ./hybridx-trail/Utilities/check-vo2-link-copies.sh --sync
# copies them over. Run by CI; no arguments checks.
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
RUN="$ROOT/hybridx-run/Software/Libs"
TRAIL="$ROOT/hybridx-trail/Software/Libs/Vo2Link"

PAIRS=(
    "Core/Header/Vo2WindowConfig.hpp Header/Vo2WindowConfig.hpp"
    "Core/Header/Vo2Window.hpp       Header/Vo2Window.hpp"
    "Core/Header/Vo2WindowFile.hpp   Header/Vo2WindowFile.hpp"
    "Core/Sources/Vo2Window.cpp      Sources/Vo2Window.cpp"
    "Core/Sources/Vo2WindowFile.cpp  Sources/Vo2WindowFile.cpp"
    "App/Header/SafeFile.hpp         Header/SafeFile.hpp"
    "App/Sources/SafeFile.cpp        Sources/SafeFile.cpp"
)

bad=0
for pair in "${PAIRS[@]}"; do
    read -r from to <<<"$pair"
    if [ "${1:-}" = "--sync" ]; then
        cp "$RUN/$from" "$TRAIL/$to" && echo "copied $from"
    elif ! cmp -s "$RUN/$from" "$TRAIL/$to"; then
        echo "DIFFERS: hybridx-trail/Software/Libs/Vo2Link/$to  vs  hybridx-run/Software/Libs/$from" >&2
        bad=1
    fi
done
if [ $bad -ne 0 ]; then
    echo "Trail's copies have drifted from Run's. Fix Run's original, then run:" >&2
    echo "  hybridx-trail/Utilities/check-vo2-link-copies.sh --sync" >&2
    exit 1
fi
[ "${1:-}" = "--sync" ] || echo "Trail's Vo2Link copies match Run's."
