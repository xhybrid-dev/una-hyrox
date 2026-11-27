#!/bin/bash
#
# Runs the Clock Probe's simulator on a pretend watch and checks what the probe learns.
#
#   sim_probe.sh [scenario] [out-dir]
#
# scenario (what make_family builds, in the real format): full (default), noplan, none, bad, empty.
# It starts the simulator under Xvfb, lets the service scan and log, takes a screenshot, presses the
# wrist key (5) three times, takes another, optionally waits through a minute row (MINUTE=1, about
# 75 s), then reads the probe's own Logs/ and prints what it found. The simulator has no wrist
# sensor on a real wrist, so the raise is the SDK's key; no daily sensors, so steps read nothing.
#
# Needs Xvfb, xdotool and ImageMagick. Build first:
#   cmake -S x-clock/Tests/Host -B x-clock/build-tests && cmake --build x-clock/build-tests
#   (cd x-clock/Tools/Probe/Software/Apps/LVGL-GUI/simulator && cmake -S . -B build -G Ninja && cmake --build build)
set -eu

REPO=$(cd "$(dirname "$0")/../.." && pwd)
SCEN=${1:-full}
OUT=${2:-$REPO/docs/screens}
BIN="$REPO/Tools/Probe/Software/Apps/LVGL-GUI/simulator/build/bin"
MAKE="$REPO/build-tests/make_family"
WATCH=${WATCH:-/tmp/clockprobe-watch-$SCEN}
DISP=:96
LOG=${SIM_LOG:-/tmp/clockprobe-sim-$SCEN.log}

[ -x "$BIN/ClockProbeSimulator" ] || { echo "Build the simulator first ($BIN)" >&2; exit 1; }
[ -x "$MAKE" ] || { echo "Build the host tests first ($MAKE)" >&2; exit 1; }
mkdir -p "$OUT"

# The pretend watch: the simulator's file system root is <watch>/Output, run from five folders down,
# so ../SharedData is <watch>/SharedData, as on the real watch (Docs/shared-data.md).
rm -rf "$WATCH"
mkdir -p "$WATCH/a/b/c/d/e" "$WATCH/Output"
"$MAKE" "$WATCH/SharedData" "$SCEN"
echo "${MODE_TXT:-on}" > "$WATCH/Output/mode.txt"   # the wrist sensor on, so the key is heard

Xvfb $DISP -screen 0 800x800x24 >/dev/null 2>&1 & XPID=$!
SIMPID=""
cleanup() { [ -n "$SIMPID" ] && kill -9 $SIMPID 2>/dev/null || true; kill $XPID 2>/dev/null || true; }
trap cleanup EXIT
sleep 2

: > "$LOG"
( cd "$WATCH/a/b/c/d/e" && exec env DISPLAY=$DISP stdbuf -oL -eL "$BIN/ClockProbeSimulator" >> "$LOG" 2>&1 ) & SIMPID=$!
sleep 7
WID=$(DISPLAY=$DISP xdotool search --name 'Clock Probe' | tail -1)
[ -n "$WID" ] || { echo "FATAL: no simulator window" >&2; tail -20 "$LOG" >&2; exit 1; }
snap() { DISPLAY=$DISP timeout 10 import -window "$WID" "$OUT/$1.png" 2>/dev/null; echo "  screenshot $1.png"; }

snap "f0-$SCEN-1-start"
for _ in 1 2 3; do DISPLAY=$DISP xdotool key 5; sleep 1; done
sleep 2
snap "f0-$SCEN-2-after-raises"
if [ "${MINUTE:-0}" = "1" ]; then
    echo "  waiting through a minute row..."
    sleep 75
    snap "f0-$SCEN-3-after-a-minute"
fi
kill -0 $SIMPID 2>/dev/null || { echo "FAIL: the simulator exited" >&2; tail -20 "$LOG" >&2; exit 1; }

# Stop it as the watch does for USB mode (Escape quits the simulator and the service gets its stop
# command), so the buffered rows reach the file; then read the probe's own log.
DISPLAY=$DISP xdotool key Escape
sleep 3
LOGS="$WATCH/Output/Logs"
echo
echo "== Logs the probe wrote ($LOGS)"
ls -1 "$LOGS" 2>/dev/null | sed 's/^/  /' || echo "  (none)"
echo
echo "== events.csv"
cat "$LOGS/events.csv" 2>/dev/null | cut -c1-170 | sed 's/^/  /' || true
echo
for f in "$LOGS"/scan_*.txt; do
    [ -f "$f" ] || continue
    echo "== $(basename "$f")"
    cut -c1-170 "$f" | sed 's/^/  /'
done
if [ "${MINUTE:-0}" = "1" ]; then
    echo
    echo "== min_*.csv"
    cat "$LOGS"/min_*.csv 2>/dev/null | cut -c1-200 | sed 's/^/  /' || true
fi

# A few plain checks.
echo
echo "== checks"
fail=0
check() { if eval "$2"; then echo "  PASS  $1"; else echo "  FAIL  $1"; fail=1; fi; }
check "the probe wrote its own folder" "[ -s \"$LOGS/state.txt\" ]"
check "a scan was logged" "grep -q ',scan,' \"$LOGS/events.csv\" 2>/dev/null"
case "$SCEN" in
    full|noplan)
        check "all twelve known files found" "grep -q 'scan,start: 11 of 12 found' \"$LOGS/events.csv\" || grep -q 'scan,start: 12 of 12 found' \"$LOGS/events.csv\""
        check "the full dump has the family's text" "grep -q 'file act_run: exists' \"$LOGS\"/scan_*.txt"
        check "weighins.csv is listed but never read" "grep -q 'file weighins: exists.*size only' \"$LOGS\"/scan_*.txt && ! grep -q 'date,time,kg' \"$LOGS\"/scan_*.txt"
        check "three wrist raises were logged" "[ \$(grep -c ',wrist,' \"$LOGS/events.csv\") -ge 3 ]"
        ;;
    empty)
        check "no files found, no crash" "grep -q 'scan,start: 0 of 12 found; 0 bad; list no/no' \"$LOGS/events.csv\""
        ;;
    none)
        # Only the SDK's stride.json (one of the twelve): the shared folder lists, HybridX is missing.
        check "one file found, HybridX missing, no crash" "grep -q 'scan,start: 1 of 12 found; 0 bad; list ok/no' \"$LOGS/events.csv\""
        ;;
    bad)
        check "bad files are reported, not mistaken for missing" "grep -q 'bad' \"$LOGS/events.csv\" && grep -q 'bad format' \"$LOGS\"/scan_*.txt"
        ;;
esac
echo
[ $fail -eq 0 ] && echo "ALL CHECKS PASSED" || { echo "SOME CHECKS FAILED (service log: $LOG)"; exit 1; }
