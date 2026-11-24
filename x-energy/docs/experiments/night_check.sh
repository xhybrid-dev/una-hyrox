#!/bin/bash
# A night's start and end in the simulator without waiting for one: the probe's local time
# is moved with TZ. Run A starts at 21:58:20 and crosses 22:00 (the night opens); run B starts
# at 07:58:20 the next morning (a restart mid-night: the night carries on, flagged) and crosses
# 08:00 (the night closes into nights.csv and the Nights page). About 6 minutes.
# Usage: night_check.sh [out-dir]   (SECS=170 each run)
set -u
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"   # x-energy/
SIM="$ROOT/Tools/Probe/Software/Apps/LVGL-GUI/simulator/build/bin"
OUT=${1:-/tmp/energy-night}
mkdir -p "$OUT"
rm -rf "$ROOT/Tools/Probe/Software/Output"   # the simulator's watch: starts empty
offset() {  # $1 = target local "HH:MM:SS", $2 = days after run A's local date (0 or 1)
  python3 - "$1" "$2" "${DAYA:-}" <<'PY'
import sys, time, datetime
h, m, s = map(int, sys.argv[1].split(":")); plus = int(sys.argv[2]); daya = sys.argv[3]
now = time.time()
utc = datetime.datetime.fromtimestamp(now, datetime.timezone.utc).replace(tzinfo=None)
if plus == 0:
    target = utc.replace(hour=h, minute=m, second=s, microsecond=0)
    if target < utc:
        target += datetime.timedelta(days=1)
    print(int((target - utc).total_seconds()), target.date().isoformat())
else:
    d = datetime.date.fromisoformat(daya) + datetime.timedelta(days=1)
    target = datetime.datetime(d.year, d.month, d.day, h, m, s)
    print(int((target - utc).total_seconds()), d.isoformat())
PY
}
tz() { local o=$1; printf "XXX-%d:%02d:%02d" $((o/3600)) $(((o%3600)/60)) $((o%60)); }
run() {  # $1 = TZ, $2 = seconds, $3 = name
  Xvfb :94 -screen 0 800x600x24 >/dev/null 2>&1 & local X=$!
  sleep 1
  (cd "$SIM" && DISPLAY=:94 TZ="$1" ./EnergyProbeSimulator > "$OUT/$3.log" 2>&1) & local S=$!
  sleep 4
  local W=$(DISPLAY=:94 xdotool search --name "Energy Probe" | head -1)
  sleep "$2"
  DISPLAY=:94 timeout 5 xdotool key --window "$W" 2; sleep 1
  DISPLAY=:94 timeout 10 import -window "$W" "$OUT/$3-nights.png"
  DISPLAY=:94 timeout 5 xdotool key --window "$W" 1; sleep 1
  DISPLAY=:94 timeout 10 import -window "$W" "$OUT/$3-now.png"
  DISPLAY=:94 timeout 5 xdotool key --window "$W" 4; sleep 3
  kill $S 2>/dev/null; kill $X 2>/dev/null; sleep 1
}
read OA DAYA < <(offset 21:58:20 0)
export DAYA
echo "run A: TZ $(tz $OA), local date $DAYA"
run "$(tz $OA)" ${SECS:-170} A
read OB DAYB < <(offset 07:58:20 1)
echo "run B: TZ $(tz $OB), local date $DAYB"
run "$(tz $OB)" ${SECS:-170} B
