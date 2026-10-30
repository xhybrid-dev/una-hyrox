#!/bin/bash
# Fails if an app's copy of the HybridX activity link differs from the original
# in hybridx-link/. The original is the one to change; then
#   ./hybridx-link/Utilities/check-link-copies.sh --sync
# copies it into every app below. Run by CI; no arguments checks.
#
# An app joins by adding its Software/Libs/Link folder to APPS (and running
# --sync once), in the phase that wires it in.
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
FROM="$ROOT/hybridx-link/Software/Libs/Link"

APPS=(
    hybridx-run
    hybridx-streak
    hybridx-race
    hybridx-trail
    hybridx-workout
    hybridx-ride
)

FILES=(
    Header/ActivityLink.hpp
    Sources/ActivityLink.cpp
    Header/StreakNow.hpp
    Sources/StreakNow.cpp
)

bad=0
for app in ${APPS[@]+"${APPS[@]}"}; do
    TO="$ROOT/$app/Software/Libs/Link"
    for f in "${FILES[@]}"; do
        if [ "${1:-}" = "--sync" ]; then
            mkdir -p "$(dirname "$TO/$f")" && cp "$FROM/$f" "$TO/$f" && echo "copied $f into $app"
        elif ! cmp -s "$FROM/$f" "$TO/$f"; then
            echo "DIFFERS: $app/Software/Libs/Link/$f  vs  hybridx-link/Software/Libs/Link/$f" >&2
            bad=1
        fi
    done
done
if [ $bad -ne 0 ]; then
    echo "An app's copy of the activity link has drifted from hybridx-link's. Fix the original, then run:" >&2
    echo "  hybridx-link/Utilities/check-link-copies.sh --sync" >&2
    exit 1
fi
[ "${1:-}" = "--sync" ] || echo "The apps' copies of the activity link match hybridx-link's (${#APPS[@]} apps)."
