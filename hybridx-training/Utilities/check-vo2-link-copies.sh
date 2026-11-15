#!/bin/bash
# Fails if X Training's copies of HybridX Run's window builder, file codec and
# SafeFile differ from Run's. X Training leaves its runs' steady minutes for
# Run's VO2max in the same records Run builds itself (X Training NOTES, "TP4"),
# so the code that builds and writes them must be Run's, byte for byte. Run's are
# the originals: change them there, then
#   ./hybridx-training/Utilities/check-vo2-link-copies.sh --sync
# copies them over (and hybridx-trail/Utilities/check-vo2-link-copies.sh --sync
# for Trail's). Run by CI; no arguments checks.
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
RUN="$ROOT/hybridx-run/Software/Libs"
XT="$ROOT/hybridx-training/Software/Libs"

PAIRS=(
    "Core/Header/Vo2WindowConfig.hpp RunCore/Header/Vo2WindowConfig.hpp"
    "Core/Header/Vo2Window.hpp       RunCore/Header/Vo2Window.hpp"
    "Core/Header/Vo2WindowFile.hpp   RunCore/Header/Vo2WindowFile.hpp"
    "Core/Sources/Vo2Window.cpp      RunCore/Sources/Vo2Window.cpp"
    "Core/Sources/Vo2WindowFile.cpp  RunCore/Sources/Vo2WindowFile.cpp"
    "App/Header/SafeFile.hpp         App/Header/SafeFile.hpp"
    "App/Sources/SafeFile.cpp        App/Sources/SafeFile.cpp"
)

bad=0
for pair in "${PAIRS[@]}"; do
    read -r from to <<<"$pair"
    if [ "${1:-}" = "--sync" ]; then
        cp "$RUN/$from" "$XT/$to" && echo "copied $from"
    elif ! cmp -s "$RUN/$from" "$XT/$to"; then
        echo "DIFFERS: hybridx-training/Software/Libs/$to  vs  hybridx-run/Software/Libs/$from" >&2
        bad=1
    fi
done
if [ $bad -ne 0 ]; then
    echo "X Training's copies have drifted from Run's. Fix Run's original, then run:" >&2
    echo "  hybridx-training/Utilities/check-vo2-link-copies.sh --sync" >&2
    exit 1
fi
[ "${1:-}" = "--sync" ] || echo "X Training's copies of Run's window code match Run's."
