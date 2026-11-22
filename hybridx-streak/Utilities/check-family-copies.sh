#!/bin/bash
# Fails if Streak's copies of the family's readers differ from their originals.
# Streak's Your numbers screen (NOTES "S7") shows what X Training, X Run and
# X Weigh-in leave in the shared folder, read with those apps' own code:
#   - X Training's training_today.json reader (its glance's) and its reader of
#     X Run's vo2max.json;
#   - X Weigh-in's log reader and trend, so the weight is the one X Weigh-in shows.
# The originals are in hybridx-training and x-weighin: change them there, then
#   ./hybridx-streak/Utilities/check-family-copies.sh --sync
# copies them over. Run by CI; no arguments checks.
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
XT="$ROOT/hybridx-training/Software/Libs/Core"
WI="$ROOT/x-weighin/Software/Libs/Core"
FAM="$ROOT/hybridx-streak/Software/Libs/Family"

# "<original> <copy, under Libs/Family>"
PAIRS=(
    "$XT/Header/Date.hpp         Header/Date.hpp"
    "$XT/Header/PlanTypes.hpp    Header/PlanTypes.hpp"
    "$XT/Header/PlanText.hpp     Header/PlanText.hpp"
    "$XT/Header/TodayFile.hpp    Header/TodayFile.hpp"
    "$XT/Sources/TodayFile.cpp   Sources/TodayFile.cpp"
    "$XT/Header/RunVo2Max.hpp    Header/RunVo2Max.hpp"
    "$XT/Sources/RunVo2Max.cpp   Sources/RunVo2Max.cpp"
    "$WI/Header/Calendar.hpp     Header/Calendar.hpp"
    "$WI/Header/WeighLog.hpp     Header/WeighLog.hpp"
    "$WI/Sources/WeighLog.cpp    Sources/WeighLog.cpp"
    "$WI/Header/WeighProfile.hpp Header/WeighProfile.hpp"
    "$WI/Sources/WeighProfile.cpp Sources/WeighProfile.cpp"
    "$WI/Header/WeightUnit.hpp   Header/WeightUnit.hpp"
    "$WI/Header/WeighText.hpp    Header/WeighText.hpp"
    "$WI/Header/Trend.hpp        Header/Trend.hpp"
    "$WI/Sources/Trend.cpp       Sources/Trend.cpp"
    "$WI/Header/LogStore.hpp     Header/LogStore.hpp"
    "$WI/Sources/LogStore.cpp    Sources/LogStore.cpp"
)

bad=0
for pair in "${PAIRS[@]}"; do
    read -r from to <<<"$pair"
    if [ "${1:-}" = "--sync" ]; then
        mkdir -p "$(dirname "$FAM/$to")"
        cp "$from" "$FAM/$to" && echo "copied ${from#"$ROOT"/}"
    elif ! cmp -s "$from" "$FAM/$to"; then
        echo "DIFFERS: hybridx-streak/Software/Libs/Family/$to  vs  ${from#"$ROOT"/}" >&2
        bad=1
    fi
done
if [ $bad -ne 0 ]; then
    echo "Streak's copies of the family's readers have drifted. Fix the original, then run:" >&2
    echo "  hybridx-streak/Utilities/check-family-copies.sh --sync" >&2
    exit 1
fi
[ "${1:-}" = "--sync" ] || echo "Streak's copies of the family's readers match their originals."
