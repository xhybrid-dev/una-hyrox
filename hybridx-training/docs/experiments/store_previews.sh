#!/bin/bash
#
# The store previews for X Training: 9 simulator screens, 480x480, into Resources/previews/
# (the folder pack-store-zip.sh puts in the zip). They are copied from the app's screen
# captures in docs/screens/ (capture_screens.sh; the tm-* ones from `TREADMILL=1 sim_plan.sh`,
# 17 November 2026), chosen by eye: a capture's key presses can drift with the machine's load, so
# a file name is only approximately what it shows, and a capture is looked at before it is
# committed. The previews, in the order the store shows them:
#
#  1-today         (tp3-session-today.png)   today's session: the target, the distance, the purpose
#  2-gauge         (tm-gauge.png)            the pace gauge, with the time left in the step
#  3-rep           (tp3-reps-card.png)       a short rep's result on the next step's card
#  4-week          (tp3-week-week.png)       this week, seven rows from Monday
#  5-paces         (tp3-week-paces1.png)     the paces from the 5K
#  6-changes       (tp5-changes-menu.png)    Changes: move, skip, pause or end the plan (1.1.0)
#  7-missed        (tp5-offer-today.png)     a missed long run, offered back (1.1.0)
#  8-treadmill     (tm-today.png)            today on a treadmill: the target as a belt speed (1.1.0)
#  9-treadmill-rep (tm-rep.png)              a 400 m rep at 15.1 km/h on a treadmill (1.1.0)
#
# --recapture first re-runs capture_screens.sh (about 8 minutes; Xvfb, xdotool, ImageMagick,
# fitdecode and the simulator built) into docs/screens. Look at the previews again before
# committing them. The release: hybridx-link/docs/STORE_RELEASE.md.
#
# Usage:  UNA_SDK=/path/to/una-sdk ./store_previews.sh [--recapture]
set -u
HERE=$(cd "$(dirname "$0")" && pwd)
REPO=$(cd "$HERE/../.." && pwd)
SCREENS="$REPO/docs/screens"
OUT="$REPO/Resources/previews"
rc=0
if [ "${1:-}" = "--recapture" ]; then
    echo "Re-running capture_screens.sh into docs/screens (look at its screens before keeping them)..."
    "$HERE/capture_screens.sh" "$SCREENS" || rc=1
fi
# pick <file under docs/screens> <preview name>: kept at 480x480 (the watch's 240 at 2x).
pick() {
    local f="$SCREENS/$1"
    if [ ! -f "$f" ]; then echo "  FAIL: docs/screens has no $1" >&2; rc=1; return; fi
    python3 - "$f" "$OUT/$2.png" <<'PY'
import sys
from PIL import Image
im = Image.open(sys.argv[1]).convert("RGB")
if im.size != (480, 480):
    im = im.resize((480, 480), Image.LANCZOS)
im.save(sys.argv[2])
PY
    echo "  $2.png  <- $1"
}
mkdir -p "$OUT"
rm -f "$OUT"/*.png
pick tp3-session-today.png 1-today
pick tm-gauge.png 2-gauge
pick tp3-reps-card.png 3-rep
pick tp3-week-week.png 4-week
pick tp3-week-paces1.png 5-paces
pick tp5-changes-menu.png 6-changes
pick tp5-offer-today.png 7-missed
pick tm-today.png 8-treadmill
pick tm-rep.png 9-treadmill-rep
exit $rc
