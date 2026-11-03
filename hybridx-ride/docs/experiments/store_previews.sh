#!/bin/bash
#
# The store previews for X Ride: 6 simulator screens, 480x480, into Resources/previews/
# (the folder pack-store-zip.sh puts in the zip). They are copied from the app's walkthrough
# screens in docs/screens/, chosen by eye: a walkthrough's key presses can drift with the
# machine's load, so its file names are only approximately what they show, and a capture is
# looked at before it is committed. The previews:
#
#  1-menu  (r1-??-main-autopause-on.png)
#  2-riding  (r1-??-live-main-outdoor.png)
#  3-auto-paused  (r1-??-live-auto-paused.png)
#  4-intervals  (r1-??-live-plan-countdown.png)
#  5-summary  (r1-??-summary-ride.png)
#  6-saved  (r1-??-saved-streak-line.png)
#
# --recapture first re-runs the walkthrough, capture_screens.sh (an outdoor ride with an auto-pause, and an indoor Intervals session), into docs/screens. It needs what that
# needs (Xvfb, xdotool, ImageMagick, fitdecode; the simulator built). Run it on an idle machine,
# then look at the previews again before committing them. The release:
# hybridx-link/docs/STORE_RELEASE.md.
#
# Usage:  UNA_SDK=/path/to/una-sdk ./store_previews.sh [--recapture]

set -u
HERE=$(cd "$(dirname "$0")" && pwd)
REPO=$(cd "$HERE/../.." && pwd)
SCREENS="$REPO/docs/screens"
OUT="$REPO/Resources/previews"
rc=0
if [ "${1:-}" = "--recapture" ]; then
    echo "Re-running the walkthrough into docs/screens (look at its screens before keeping them)..."
    "$HERE/capture_screens.sh" || rc=1
fi
# pick <file pattern under docs/screens> <preview name>: kept at 480x480 (the watch's 240 at 2x).
pick() {
    local f
    f=$(ls "$SCREENS"/$1 2>/dev/null | head -n 1)
    if [ -z "$f" ]; then echo "  FAIL: docs/screens has no $1" >&2; rc=1; return; fi
    python3 - "$f" "$OUT/$2.png" <<'PY'
import sys
from PIL import Image
im = Image.open(sys.argv[1]).convert("RGB")
if im.size != (480, 480):
    im = im.resize((480, 480), Image.LANCZOS)
im.save(sys.argv[2])
PY
    echo "  $2.png  <- ${f#$SCREENS/}"
}
mkdir -p "$OUT"
rm -f "$OUT"/*.png
pick "r1-??-main-autopause-on.png" 1-menu
pick "r1-??-live-main-outdoor.png" 2-riding
pick "r1-??-live-auto-paused.png" 3-auto-paused
pick "r1-??-live-plan-countdown.png" 4-intervals
pick "r1-??-summary-ride.png" 5-summary
pick "r1-??-saved-streak-line.png" 6-saved
exit $rc
