#!/bin/bash
# Build the FIT sample tool, write a ride in each scenario through the app's
# own ActivityWriter, and decode each with fitdecode (pip install fitdecode).
# Usage:  UNA_SDK=/path/to/una-sdk ./fit_check.sh [work-dir]
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
WORK=${1:-$(mktemp -d)}
mkdir -p "$WORK"
"$HERE/fit_build.sh" "$WORK/make_fit"
rc=0
for sc in full outdoor autolap nohr crash; do
    echo "== $sc"
    (cd "$WORK" && ./make_fit "$sc.fit" "$sc" | tail -2)
    python3 -I "$HERE/fit_check.py" "$WORK/$sc.fit" "$sc" || rc=1
done
exit $rc
