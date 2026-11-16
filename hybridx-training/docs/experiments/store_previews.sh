#!/bin/bash
#
# The store previews for X Training: 6 simulator screens, 480x480, into Resources/previews/
# (the folder pack-store-zip.sh puts in the zip). They are copied from the app's screen
# captures in docs/screens/ (capture_screens.sh), chosen by eye: a capture's key presses can
# drift with the machine's load, so a file name is only approximately what it shows, and a
# capture is looked at before it is committed. The previews:
#
#  1-menu     (tp3-session-main.png)   the main menu, Today first with its session
#  2-today    (tp3-session-today.png)  today's session: the target, the distance, the purpose
#  3-week     (tp3-week-week.png)      this week, seven rows from Monday
#  4-plan     (tp4-vo2-plan10k.png)    the plan's weeks, and X Run's VO2max
#  5-paces    (tp3-week-paces1.png)    the paces from the 5K
#  6-rep      (tp3-reps-card.png)      a short rep's result on the next step's card
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
pick tp3-session-main.png 1-menu
pick tp3-session-today.png 2-today
pick tp3-week-week.png 3-week
pick tp4-vo2-plan10k.png 4-plan
pick tp3-week-paces1.png 5-paces
pick tp3-reps-card.png 6-rep
exit $rc
