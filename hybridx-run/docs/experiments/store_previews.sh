#!/bin/bash
#
# The store previews for X Run: 6 simulator screens, 480x480, into Resources/previews/
# (the folder pack-store-zip.sh puts in the zip). They are copied from the app's walkthrough
# screens in docs/screens/, chosen by eye: a walkthrough's key presses can drift with the
# machine's load, so its file names are only approximately what they show, and a capture is
# looked at before it is committed. The previews:
#
#  1-running  (run/03-after.png)
#  2-lap  (run/03b-lap.png)
#  3-vo2max  (vo2-02-overview.png)
#  4-recent-runs  (vo2-03-runs-1.png)
#  5-summary  (run/06-overview.png)
#  6-best-splits  (run/11-splits.png)
#
# --recapture first re-runs the walkthrough, sim_run.sh (a 240-second run with a seeded history, into docs/screens/run/) and capture_vo2.sh (the VO2max screens), into docs/screens. It needs what that
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
    printf '{"schema":1,"values":{"birthYear":1986,"birthMonth":5,"restingHr":52}}' > "$SCREENS/.run_cfg.json"
    rm -rf "$SCREENS/run"
    SEED=1 "$HERE/sim_run.sh" 240 "$SCREENS/run" "$SCREENS/.run_cfg.json" > /dev/null || rc=1
    rm -f "$SCREENS/.run_cfg.json"
    "$HERE/capture_vo2.sh" || rc=1
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
pick "run/03-after.png" 1-running
pick "run/03b-lap.png" 2-lap
pick "vo2-02-overview.png" 3-vo2max
pick "vo2-03-runs-1.png" 4-recent-runs
pick "run/06-overview.png" 5-summary
pick "run/11-splits.png" 6-best-splits
exit $rc
