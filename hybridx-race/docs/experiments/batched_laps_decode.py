import fitdecode, sys

stations = ["SkiErg","Sled Push","Sled Pull","Burpee Broad Jumps",
            "Row","Farmers Carry","Sandbag Lunges","Wall Balls"]
expected = []
for r in range(1, 9):
    expected.append(("RUN", 0, r, 0, 300 + r))
    expected.append((stations[r-1], 2, r, r, 200 + r*3))

order, laps, session, records = [], [], None, 0
with fitdecode.FitReader(sys.argv[1]) as fr:
    for f in fr:
        if not isinstance(f, fitdecode.FitDataMessage):
            continue
        order.append(f.name)
        if f.name == "record":
            records += 1
        elif f.name == "lap":
            laps.append({fld.name: fld.value for fld in f.fields})
        elif f.name == "session":
            session = {fld.name: fld.value for fld in f.fields}

print(f"decoded cleanly: {records} records, {len(laps)} laps")

# Ordering: every lap must come after every record, and before the session.
last_record = max(i for i, n in enumerate(order) if n == "record")
first_lap = min(i for i, n in enumerate(order) if n == "lap")
session_at = order.index("session")
print(f"last record #{last_record} < first lap #{first_lap} < session #{session_at}: "
      f"{last_record < first_lap < session_at}")

ok = True
for i, (lap, (label, styp, rnd, sid, dur)) in enumerate(zip(laps, expected)):
    checks = {
        "message_index": lap.get("message_index") == i,
        "elapsed": abs((lap.get("total_elapsed_time") or 0) - dur) < 0.001,
        "timer": abs((lap.get("total_timer_time") or 0) - dur) < 0.001,
        "segment_type": lap.get("segment_type") == styp,
        "round": lap.get("round") == rnd,
        "station_id": lap.get("station_id") == sid,
    }
    if not all(checks.values()):
        ok = False
        print(f"  LAP {i} ({label}) MISMATCH: "
              f"{ {k: v for k, v in checks.items() if not v} } -> {lap}")

print(f"all 16 laps match expected duration/index/dev-fields: {ok}")
print(f"session num_laps={session.get('num_laps')} sport={session.get('sport')} "
      f"sub_sport={session.get('sub_sport')} "
      f"race_format={session.get('race_format')} completed={session.get('completed')}")
