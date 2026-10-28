#!/usr/bin/env python3
"""Decode a workout FIT file with fitdecode (an independent reader) and check it
says what the app means it to say.

    fit_check.py <file.fit> <scenario>      scenario: full | autolap | nohr | crash | sim

Checks: valid header and CRC; file_id; the one developer field
(resting_calories, as the SDK's Workout example); timer events in pairs; one
record a second (a gap only where paused); laps that chain and sum to the
session, with the right lap_trigger (manual 0, time 1); calories per lap and
for the session, and for `full` the session's calories worked out again here
from the records (each second in the zone of the heart rate before it, on the
watch's default thresholds, 80 kg, the example's MET table); session sport
generic and profile name Workout; and for `crash`, that the records survive
with no laps. `sim` is a file from the PC simulator (random heart rate, its own
weight): structure is checked, exact numbers are not.
Exit status 1 if any check fails.
"""
import sys
import fitdecode

path, scenario = sys.argv[1], sys.argv[2]
failures = []


def check(cond, what):
    print(("  ok   " if cond else "  FAIL ") + what)
    if not cond:
        failures.append(what)


def fields(frame):
    return {f.name: f.value for f in frame.fields}


frames = []
with fitdecode.FitReader(path, check_crc=fitdecode.CrcCheck.RAISE) as r:
    for fr in r:
        if isinstance(fr, fitdecode.FitDataMessage):
            frames.append((fr.name, fields(fr), fr))

by = {}
for name, f, fr in frames:
    by.setdefault(name, []).append(f)

print(f"{path}: {len(frames)} data messages: " + ", ".join(f"{k} x{len(v)}" for k, v in by.items()))

# --- identity
fid = by["file_id"][0]
check(fid.get("type") == "activity", "file_id.type is activity")
check(fid.get("manufacturer") in (351, "una"), f"file_id.manufacturer is UNA ({fid.get('manufacturer')})")
check(fid.get("product_name") == "UNA Watch", "file_id.product_name is UNA Watch")

# --- developer data: only the example's lap resting calories
dev = {f.get("field_name"): f for f in by.get("field_description", [])}
check(set(dev) == {"resting_calories"}, f"one developer field, resting_calories ({sorted(dev)})")
check(dev.get("resting_calories", {}).get("units") == "kcal", "resting_calories is in kcal")

# --- timer events
kinds = [e.get("event_type") for e in by.get("event", [])]
check(kinds and kinds[0] == "start", "first event is a timer start")
if scenario != "crash":
    check(kinds[-1] == "stop", "last event is a timer stop")
    check(all(kinds[i] != kinds[i + 1] for i in range(len(kinds) - 1)), "timer events alternate start/stop")
    want_events = {"full": 4, "nohr": 4, "autolap": 2}.get(scenario)
    if want_events is not None:
        check(len(kinds) == want_events, f"{len(kinds)} timer events, want {want_events}")

# --- records
recs = by.get("record", [])
check(len(recs) > 0, f"{len(recs)} records")
ts = [r["timestamp"] for r in recs]
gaps = [(b - a).total_seconds() for a, b in zip(ts, ts[1:])]
big = [g for g in gaps if g != 1.0]
if scenario in ("full", "nohr"):
    check(len(recs) == 415, "415 records (435 s less the 20 s pause)")
    check(len(big) == 1 and big[0] == 21.0, f"one gap, at the pause ({big})")
elif scenario == "autolap":
    check(len(recs) == 150 and not big, "150 records, no gaps")
check(all(g > 0 for g in gaps), "record timestamps strictly increase")
hrs = [r.get("heart_rate") for r in recs]
if scenario == "nohr":
    check(all(h is None for h in hrs), "no heart rate on any record (invalid, not zero)")
elif scenario == "sim":
    check(all(h is None or 20 < h <= 255 for h in hrs), "heart rate absent or plausible on every record")
else:
    check(all(h is not None for h in hrs), f"heart rate on every record ({min(hrs)}..{max(hrs)})")

if scenario == "crash":
    check(len(recs) >= 400, f"{len(recs)} records survive the crash (>= 400 of 415)")
    check("lap" not in by and "session" not in by, "no laps or session after a crash (written at stop)")
    check(not failures, "recovered file decodes with a valid CRC")
    sys.exit(1 if failures else 0)

# --- laps
laps = by.get("lap", [])
sess = by["session"][0]
want_laps = {"full": 4, "nohr": 4, "autolap": 3}.get(scenario)
if want_laps is not None:
    check(len(laps) == want_laps, f"{want_laps} laps (got {len(laps)})")
check([l.get("message_index") for l in laps] == list(range(len(laps))), "lap message_index 0..n-1")
triggers = [l.get("lap_trigger") for l in laps]
want_t = {"autolap": ["time", "time", "manual"]}.get(scenario, ["manual"] * len(laps) if scenario != "sim" else None)
if want_t is not None:
    check(triggers == want_t, f"lap triggers {triggers} == {want_t}")
if scenario == "autolap":
    check([l["total_timer_time"] for l in laps] == [60, 60, 30], f"auto laps of 60, 60, 30 s ({[l['total_timer_time'] for l in laps]})")
cursor = laps[0]["start_time"]
chain = True
for l in laps:
    chain &= l["start_time"] == cursor
    cursor = l["timestamp"]
check(chain, "each lap starts where the last ended")
check(sum(l["total_elapsed_time"] for l in laps) == sess["total_elapsed_time"], "laps' elapsed sum to the session's")
check(sum(l["total_timer_time"] for l in laps) == sess["total_timer_time"], "laps' timer time sums to the session's")

# --- calories
lap_kcal = [l.get("total_calories") for l in laps]
lap_rest = [l.get("resting_calories") for l in laps]
s_kcal = sess.get("total_calories")
s_meta = sess.get("unknown_196", sess.get("metabolic_calories"))   # FitProfile.hpp's 196; not in fitdecode's profile
print(f"  info lap kcal {lap_kcal}, lap resting {lap_rest}; session {s_kcal} kcal, metabolic {s_meta}")
check(all(k is not None for k in lap_kcal), "every lap has total_calories")
check(all(k is not None for k in lap_rest), "every lap has resting_calories")
check(s_kcal is not None and s_meta is not None, "the session has total and metabolic calories")
if s_kcal is not None and all(k is not None for k in lap_kcal):
    check(abs(sum(lap_kcal) - s_kcal) <= len(laps), f"the laps' calories ({sum(lap_kcal)}) add up to the session's within rounding")
if s_kcal is not None and s_meta is not None:
    check(s_meta <= s_kcal, "the resting part is no more than the total")
if scenario == "full":
    # Again from the records: the first second has no zone yet, each later
    # second is in the zone of the record before it (a pause's gap counts its
    # one active second), at 80 kg.
    th = [95, 114, 133, 152, 171, 190]
    met10 = [10, 25, 45, 70, 100, 125]

    def zone(hr):
        z = 0
        for i, t in enumerate(th):
            if hr and hr > t:
                z = i + 1
        return min(z, 5)
    met_ms = met10[0] * 1000
    for prev in hrs[:-1]:
        met_ms += met10[zone(prev)] * 1000
    x10 = (met_ms * 80000 + 1800000000) // 3600000000
    want = x10 // 10 + (1 if x10 % 10 >= 5 else 0)
    check(s_kcal == want, f"session calories {s_kcal} == {want} worked out from the records")
    rest_x10 = (len(recs) * 1000 * 10 * 80000 + 1800000000) // 3600000000
    want_rest = rest_x10 // 10 + (1 if rest_x10 % 10 >= 5 else 0)
    check(s_meta == want_rest, f"metabolic calories {s_meta} == {want_rest} (1 MET for {len(recs)} s)")
if scenario == "nohr":
    check(all(l.get("avg_heart_rate") is None for l in laps), "no lap heart rate without HR")
    check(s_kcal == s_meta, "with no heart rate every second is resting")

# --- session
check(sess.get("sport") in ("generic", 0), f"session sport generic ({sess.get('sport')}), as the SDK's Workout example")
check(sess.get("sub_sport") in ("generic", 0), "sub_sport generic")
check(sess.get("num_laps") == len(laps), "session num_laps")
check(sess.get("sport_profile_name") == "Workout", f"sport_profile_name is Workout ({sess.get('sport_profile_name')})")
act = by["activity"][0]
check(act.get("num_sessions") == 1, "activity num_sessions")

print("FAILED" if failures else "ALL CHECKS PASSED")
sys.exit(1 if failures else 0)
