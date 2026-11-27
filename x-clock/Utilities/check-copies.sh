#!/bin/bash
# Fails if the Clock Probe's copies of other apps' reader code differ from the originals.
# The probe reads the family's files with the family's own readers, byte for byte:
#   - X Training's training_today.json reader (TodayFile, Date, PlanTypes);
#   - HybridX Run's vo2max.json reader (GlanceView, Vo2Config, Vo2WindowConfig);
#   - Streak's in-memory directory tree for host tests (TreeFileSystem).
# The originals are the ones to change; then
#   ./x-clock/Utilities/check-copies.sh --sync
# copies them over. Run by CI; no arguments checks. (The activity link's copy is
# checked by hybridx-link/Utilities/check-link-copies.sh.)
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
XC="$ROOT/x-clock"
LIBS="$XC/Tools/Probe/Software/Libs"
TR="$ROOT/hybridx-training/Software/Libs/Core"
RUN="$ROOT/hybridx-run/Software/Libs"
STK="$ROOT/hybridx-streak/Tests/Host/support"

# "original  copy" (absolute paths)
PAIRS=(
    "$TR/Header/Date.hpp               $LIBS/Training/Header/Date.hpp"
    "$TR/Header/PlanTypes.hpp          $LIBS/Training/Header/PlanTypes.hpp"
    "$TR/Header/TodayFile.hpp          $LIBS/Training/Header/TodayFile.hpp"
    "$TR/Sources/TodayFile.cpp         $LIBS/Training/Sources/TodayFile.cpp"
    "$RUN/Glance/Header/GlanceView.hpp $LIBS/Vo2/Header/GlanceView.hpp"
    "$RUN/Glance/Sources/GlanceView.cpp $LIBS/Vo2/Sources/GlanceView.cpp"
    "$RUN/Core/Header/Vo2Config.hpp    $LIBS/Vo2/Header/Vo2Config.hpp"
    "$RUN/Core/Header/Vo2WindowConfig.hpp $LIBS/Vo2/Header/Vo2WindowConfig.hpp"
    "$STK/TreeFileSystem.hpp           $XC/Tests/Host/support/TreeFileSystem.hpp"
    "$STK/TreeFileSystem.cpp           $XC/Tests/Host/support/TreeFileSystem.cpp"
)

bad=0
for pair in "${PAIRS[@]}"; do
    read -r from to <<<"$pair"
    if [ "${1:-}" = "--sync" ]; then
        mkdir -p "$(dirname "$to")" && cp "$from" "$to" && echo "copied ${from#"$ROOT"/}"
    elif ! cmp -s "$from" "$to"; then
        echo "DIFFERS: ${to#"$ROOT"/}  vs  ${from#"$ROOT"/}" >&2
        bad=1
    fi
done
if [ $bad -ne 0 ]; then
    echo "The Clock Probe's copies have drifted from the originals. Fix the original, then run:" >&2
    echo "  x-clock/Utilities/check-copies.sh --sync" >&2
    exit 1
fi
[ "${1:-}" = "--sync" ] || echo "The Clock Probe's copies match their originals (${#PAIRS[@]} files)."
