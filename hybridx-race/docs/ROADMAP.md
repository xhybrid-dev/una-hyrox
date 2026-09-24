# Roadmap

This is the working list of everything past v0.1.0 (Gate 6): what the brief
already scoped in §16, and what's come up since — including the sensor
capability survey from 23–24 September 2026. It doesn't re-decide anything
`NOTES.md` §0.12 already tracks as an open decision (D1–D9); it exists so
"what's next" lives in one place instead of scattered across chat history.

Status labels used below:

- **Scoped (brief §16)** — Jon already named this in the brief; restated here
  with the technical detail the brief deliberately left out ("not to be built
  now").
- **Done** — shipped, verified, just not yet reflected in `CHANGELOG.md`'s
  `[Unreleased]` framing at the time it was written.
- **Proposed** — surfaced in conversation, not yet scoped by Jon, not started.
  Nothing here is built. Default behaviour is unchanged until Jon says go.
- **Blocked** — scoped, but waiting on an input only Jon can supply.

Nothing on this page authorises work. Each item still needs its own plan-mode
pass and Gate when it's actually picked up, per `CLAUDE.md`.

---

## 1. Data fidelity — how a segment's distance is known

### 1.1 Today: distance is assumed, not measured

Every lap's `distanceM` is a lookup — the station's fixed metres from
`Race::kStations[]`, or the configured run length — never a sensor reading.
`Service.hpp` documents this as a deliberate scope cut when adapting from the
SDK's Workout example ("removed: pressure and altitude, distance and speed").
Correct for a nominal 1 km run; silently wrong if the run is short, long, or
drifting.

### 1.2 Proposed: a run-distance source setting

Let the athlete choose how each run's distance/pace is derived, instead of
always assuming the configured length:

| Mode | Source | Fits |
|---|---|---|
| **Manual** (today's behaviour, stays default) | `Race::kStations[]` / configured run length, unchanged | Racing HYROX itself, or any sim where the distance is by definition fixed |
| **Watch estimate** | `SDK::Sensor::Type::RUNNING_CADENCE` fed through the SDK's own `SDK::Calibration::StrideMath` / `TreadmillSpeedEstimator` (the same pair the SDK's **Treadmill** example app uses for GPS-free indoor distance) | Training indoors — a treadmill, a track, a gym floor — where the real distance covered may not match the nominal length |
| **GPS** | `SDK::Sensor::Type::GPS_LOCATION` / `GPS_SPEED` / `GPS_DISTANCE` (used by the SDK's Running/Cycling/Hiking examples) | Training outdoors. This is brief §16's v1.3 item ("outdoor sims with GPS and automatic run completion at the set distance") — folded in here as a third mode of the same setting rather than a separate special case |

**Why a selector rather than always-on sensing:** HYROX itself is indoor and
has fixed, known distances — D9 in the brief already leans towards *not*
writing measured distance over the nominal one for the race proper. A
training run has no such guarantee. Letting the athlete pick per context (not
per app version) means racing and training both get the number that's
actually true for that session, and nobody who never touches the setting
sees any change from today.

**Scope, if Jon confirms this:** a new AppConfig field (a third state
alongside the existing `roxzoneSplits`/`runDistanceM`/etc. pattern —
`AppConfigFields.cpp`/`.hpp` and the manifest's `configFields` kept in sync,
same as every other setting); a settings-screen row; `RaceModel`/`Service`
accepting a live distance for Run segments instead of always resolving the
static table value; for the watch-estimate mode, the SDK's own plausibility
gate (implied step length clamped 0.15–2.50 m) already guards against bad
cadence readings, but this still needs testing against an actual HYROX run
cadence, not just the Treadmill example's assumptions; for GPS mode, an
indoor-signal-loss story (what a run screen shows/does with no fix) that the
brief's v1.3 line doesn't spell out. Not a config tweak — a real feature,
sized like F14/F15, and it touches `RaceModel`'s "pure, parameter-driven"
core (`ARCHITECTURE.md`), so it wants its own plan-mode pass rather than
riding in on something else.

**Status: Proposed.** Surfaced 24 September 2026. Needs Jon's go-ahead before
any of the three sub-items — the Manual mode is what ships today either way.

---

## 2. Ambient temperature

### 2.1 What's available

`SDK::Sensor::Type::AMBIENT_TEMPERATURE` — `SensorDataParserTemperature.hpp`,
class `Temperature`, one float field, sentinel `-273.15` on invalid data
(unit is device-specific per the header comment, not stated as always °C).
Not currently connected anywhere in `Service.cpp`.

### 2.2 The FIT side, sourced the way `wkt_step_name` was

`SDK/Fit/FitProfile.hpp` does not declare a temperature field on `Record` —
checked directly, zero matches. The public FIT profile does: `Record.temperature`
is field **13**, base type **sint8** (whole degrees C, no scale/offset),
confirmed from `fitdecode`'s bundled Garmin profile tables the same way
`wkt_step_name` (field 0) and the `sub_sport` values were sourced in 5.11 —
not from memory, checkable by writing it and reading it back. Same
`FitWriter`-is-profile-agnostic mechanism applies: nothing to invent, just to
verify by round-trip decode before it ships, and to log to UNA alongside the
other `FitProfile.hpp` gaps (5.11) since this is a third field the SDK's own
profile omits.

### 2.3 Scope, if Jon confirms this

Smaller than the distance-source item: connect `AMBIENT_TEMPERATURE`
alongside the existing sensor set in `Service.cpp`, add a `temperature`
field to `RecordData` (1 Hz, same as heart rate), write FIT field 13 on
`Record`. No settings-screen row needed — this is a background record field,
not a user choice. Verify the SDK header's unit claim before trusting the
value (device-specific per the comment; confirm what this watch actually
reports before writing it as °C) and confirm the sensor is genuinely ambient
and not wrist/skin temperature, which would be a different (and less
useful) number to log.

**Status: Proposed.** Surfaced 24 September 2026. Low complexity relative to
§1 — worth sequencing before it if Jon wants a quick win first.

---

## 3. Already scoped in the brief (§16), restated here for one place to look

Unchanged from the brief; not re-decided. Included for completeness now that
this file exists.

- **v1.1:** custom sims (choose stations, run distance, number of rounds);
  Relay (two runs and two stations per athlete).
- **v1.2:** target pacing from the HybridX dataset (F14, blocked on D6 — Jon's
  pacing share table); splits face (F15).
- **v1.3:** outdoor sims with GPS and automatic run completion at the set
  distance — see §1.2 above, now folded into the run-distance-source setting
  as its GPS mode rather than a separate feature.
- **v2:** wall ball and burpee rep counting from the IMU (research project;
  needs recorded training data first).
- **HybridX integration:** read completed activities via Strava, map laps to
  segments using the developer fields or lap order, compare splits against
  the race-results dataset; deliver simple presets to the watch through
  AppConfig (values file capped at 8 KB).
- **Glance:** last race time and best splits.

## 4. Already done, not yet a separate roadmap item

- **F16 — FIT workout/workout_step names.** Brief §16 doesn't list this as
  done because it wasn't, when §16 was written. It is now: `NOTES.md` §5.11–
  5.12, confirmed working in Garmin Connect by Jon (23 September 2026, D2
  closed alongside it). `CHANGELOG.md`'s `### Notes` still lists F16 among
  "deliberately not in this release" — that line is now stale and is being
  corrected alongside this file.
