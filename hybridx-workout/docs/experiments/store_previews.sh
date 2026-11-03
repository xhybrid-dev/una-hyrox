#!/bin/bash
#
# The store previews for X Workout: 6 simulator screens, 480x480, into Resources/previews/
# (the folder pack-store-zip.sh puts in the zip). They are copied from the app's walkthrough
# screens in docs/screens/, chosen by eye: a walkthrough's key presses can drift with the
# machine's load, so its file names are only approximately what they show, and a capture is
# looked at before it is committed. The previews:
#
#  1-menu  (w3-??-main-after-settings.png)
#  2-strength-set  (w3-??-strength-set1.png)
#  3-rest-countdown  (w3-??-strength-rest-last-seconds.png)
#  4-tabata  (w3-??-tabata-work.png)
#  5-circuit-station  (w3-??-circuit-station.png)
#  6-saved  (w3-??-saved.png)
#
# --recapture first re-runs the walkthrough, capture_plans.sh (a strength workout and a circuit, and a look at EMOM, AMRAP and Tabata), into docs/screens. It needs what that
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
    "$HERE/capture_plans.sh" || rc=1
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
pick "w3-??-main-after-settings.png" 1-menu
pick "w3-??-strength-set1.png" 2-strength-set
pick "w3-??-strength-rest-last-seconds.png" 3-rest-countdown
pick "w3-??-tabata-work.png" 4-tabata
pick "w3-??-circuit-station.png" 5-circuit-station
pick "w3-??-saved.png" 6-saved
exit $rc
