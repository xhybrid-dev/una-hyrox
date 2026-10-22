#!/usr/bin/env python3
"""Decode a sauna session FIT file with fitdecode (an independent reader) and
check it says what the app means it to say.

    fit_check.py <file.fit> <scenario>      scenario: full | sauna | nohr | crash | sim

Checks: valid header and CRC; file_id; developer data and field descriptions;
timer events in pairs; one record a second (gaps only where paused); the
watch temperature developer field on every record; laps that chain and sum to
the session; phase, round and recovery developer fields; session sport and
profile name; and for `crash`, that the records survive with no laps.
`sim` is a file from the PC simulator (random heart rate, made-up temperature,
sauna + plunge + rest + a second sauna, one pause): structure is checked, exact
numbers are not.
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

# --- developer data
dev = {}
for f in by.get("field_description", []):
    dev[f.get("field_name")] = f
expected = {"watch_temp": 136, "phase": 2, "round": 2, "hr_recovery_bpm": 131,
            "mode": 2, "rest_enabled": 2, "completed": 2}
for n, base in expected.items():
    ok = n in dev and dev[n].get("fit_base_type_id") in (base, {136: "float32", 131: "sint16"}.get(base, "uint8"))
    check(ok, f"developer field {n} described (base type {dev.get(n, {}).get('fit_base_type_id')})")
check(len(by.get("developer_data_id", [])) == 1, "one developer_data_id")

# --- timer events
events = [f for f in by.get("event", [])]
kinds = [e.get("event_type") for e in events]
check(kinds and kinds[0] == "start", "first event is a timer start")
if scenario != "crash":
    check(kinds[-1] == "stop", "last event is a timer stop")
    paired = all(kinds[i] != kinds[i + 1] for i in range(len(kinds) - 1))
    check(paired, "timer events alternate start/stop")
    want_events = {"full": 4, "nohr": 4, "sauna": 2, "sim": 4}[scenario]
    check(len(kinds) == want_events, f"{len(kinds)} timer events, want {want_events}")

# --- records
recs = by.get("record", [])
check(len(recs) > 0, f"{len(recs)} records")
ts = [r["timestamp"] for r in recs]
gaps = [(b - a).total_seconds() for a, b in zip(ts, ts[1:])]
big = [g for g in gaps if g != 1.0]
if scenario in ("full", "nohr"):
    check(len(recs) == 415, "415 records (435 s minus the 20 s pause)")
    check(len(big) == 1 and big[0] == 21.0, f"one gap, at the pause ({big})")
elif scenario == "sauna":
    check(len(recs) == 390 and not big, "390 records, no gaps")
elif scenario == "sim":
    check(len(big) == 1 and big[0] > 2, f"one gap, at the pause ({big})")
check(all(g > 0 for g in gaps), "record timestamps strictly increase")
temps = [r.get("watch_temp") for r in recs]
check(all(t is not None for t in temps), "every record carries watch_temp")
check(all(12.0 <= t <= 85.0 for t in temps if t is not None),
      f"watch_temp (a decimal, not hundredths) within the invented SimTemp range ({min(temps):.2f}..{max(temps):.2f})")
hrs = [r.get("heart_rate") for r in recs]
if scenario == "nohr":
    check(all(h is None for h in hrs), "no heart rate on any record (invalid, not zero)")
elif scenario == "sim":
    check(all(h is None or 20 < h <= 255 for h in hrs), "heart rate absent or plausible on every record")
else:
    check(all(h is not None and 60 <= h <= 150 for h in hrs), f"heart rate on every record ({min(hrs)}..{max(hrs)})")

if scenario == "crash":
    check(len(recs) >= 400, f"{len(recs)} records survive the crash (>= 400 of 435)")
    check("lap" not in by and "session" not in by, "no laps or session after a crash (written at stop)")
    check(not failures, "recovered file decodes with a valid CRC")
    sys.exit(1 if failures else 0)

# --- laps
laps = by.get("lap", [])
sess = by["session"][0]
want_laps = {"sauna": 3}.get(scenario, 4)
check(len(laps) == want_laps, f"{want_laps} laps (got {len(laps)})")
check([l.get("message_index") for l in laps] == list(range(len(laps))), "lap message_index 0..n-1")
check(all(l.get("lap_trigger") == "manual" for l in laps), "every lap trigger is manual")
cursor = laps[0]["start_time"]
chain = True
for l in laps:
    chain &= l["start_time"] == cursor
    cursor = l["timestamp"]
check(chain, "each lap starts where the last ended")
check(sum(l["total_elapsed_time"] for l in laps) == sess["total_elapsed_time"], "laps' elapsed sum to the session's")
check(sum(l["total_timer_time"] for l in laps) == sess["total_timer_time"], "laps' timer time sums to the session's")
phases = [l.get("phase") for l in laps]
want = {"sauna": [0, 2, 0]}.get(scenario, [0, 1, 2, 0])
check(phases == want, f"lap phases {phases} == {want}")
rounds = [l.get("round") for l in laps]
want_r = {"sauna": [1, 1, 2]}.get(scenario, [1, 1, 1, 2])
check(rounds == want_r, f"lap rounds {rounds} == {want_r}")
rec = [l.get("hr_recovery_bpm") for l in laps]
print("  info lap hr_recovery_bpm:", rec)
check(rec[-1] is None, "the last lap has no recovery (it never ended)")
if scenario == "sim":
    pass
elif scenario == "nohr":
    check(all(r is None for r in rec), "no recovery without a heart rate")
    check(all(l.get("avg_heart_rate") is None for l in laps), "no lap heart rate without HR")
else:
    check(any(r is not None and r > 0 for r in rec[:-1]), "a recovery value was recorded")

# --- session
check(sess.get("sport") == "training" or sess.get("sport") == 10, f"session sport training ({sess.get('sport')})")
check(sess.get("sub_sport") in ("generic", 0), "sub_sport generic")
check(sess.get("num_laps") == want_laps, "session num_laps")
check(sess.get("sport_profile_name") == "Sauna", f"sport_profile_name is Sauna ({sess.get('sport_profile_name')})")
check(sess.get("mode") == (1 if scenario == "sauna" else 0), f"mode developer field ({sess.get('mode')})")
check(sess.get("rest_enabled") == 1, "rest_enabled developer field")
check(sess.get("completed") == 1, "completed developer field")
act = by["activity"][0]
check(act.get("num_sessions") == 1, "activity num_sessions")

print("FAILED" if failures else "ALL CHECKS PASSED")
sys.exit(1 if failures else 0)
