# HybridX Intervals — Notes

The running log for the structured-workout idea: SDK findings, decisions, and
open questions. Same conventions as `hybridx-race/docs/NOTES.md` and
`hybridx-streak/docs/NOTES.md`: evidence first, file:line citations into
`una-sdk/`, nothing claimed that wasn't read.

---

## Why this project exists, and why it's separate from Race

Jon wants managed, paced, timed structured workouts (Garmin-Connect-style:
warm-up, work/rest steps with pace or HR targets, repeats, cool-down) for
running and cycling — sport-agnostic, not Hyrox-specific, so it's its own
project rather than a Race feature.

## Research pass — what the platform can and can't do (25 September 2026)

### The SDK's own interval feature has no per-step target

RunLVGL's `Settings::Intervals` (`Docs/Examples/Running-Architecture.md:282-329`)
is a fixed warm-up → N×(run, rest) → cool-down pattern, bounded only by time or
distance, configured entirely on-watch through `MenuIntervalsScreen` and seven
sub-screens. There is no concept of a per-step pace/HR/power target:
`WktStepTarget` (`SDK/Fit/FitProfile.hpp:68`) only implements `Open = 2`. Nothing
like it exists for cycling. This rules out reusing or extending the built-in
feature for a real Garmin-Connect-style builder.

### Real-time targets: pace or heart rate only, no power or cadence

- Pace: GPS-derived, smoothed on-watch (RunLVGL `Service.hpp:48-51`).
- Heart rate: internal PPG or an external strap.
- **No power, no cadence.** `Docs/ExternalSensors.md:14-30`: v1 external BLE
  sensors are HR-only (`Kind::HRM = 1<<0`); the cadence/power bits are
  *reserved*, not implemented. Confirmed no power-meter support exists on this
  platform — a structured-cycling workout with power targets is not currently
  buildable.

### There is no existing mechanism to push a structured workout onto the watch

Two real options, both confirmed from the SDK docs rather than assumed:

1. **`AppConfig`** (`Docs/app-config-fields.md`) — phone-editable via UNA's own
   companion app's generic settings UI, but scalar fields only (bool/int/float/
   string ≤128 bytes), ≤32 fields total. Enough for one compact preset, not a
   multi-step builder.
2. **BLE File Transfer Service** (`Docs/BLE-File-Transfer-Service.md`) — a
   documented wire protocol over GATT service `0xFEBB`, explicitly described as
   "what a companion app or integration implements against" — **not gated to
   UNA's own phone app**. `SDK::Kernel::fs` is sandbox-rooted per app
   (`Docs/app-config-fields.md:27`, `"/"` is the app's own directory), and the
   BLE side reaches that same sandbox at the absolute path `/Apps/<AppDir>/`
   (`Docs/app-config-fields.md:26`).

Nothing in `MessageTypes.hpp` notifies an app when a file arrives over FTS — a
watch app has to scan its own folder when opened. That's actually the natural
moment here: you'd open the workout app right before a session anyway.

### Decision: probe BLE FTS before building a phone app

Asked to choose between (A) a real phone companion app over BLE FTS or (B) the
`AppConfig` compact-preset fallback, Jon chose to **probe BLE FTS first** —
prove the transport works on his hardware before committing to an iOS+Android
build. This mirrors HybridX Streak's Gate 0 (`hybridx-streak/docs/PROBE.md`): a
throwaway, minimal check of one unproven platform behaviour, run on real
hardware, before any product work depends on it.

Two things are genuinely unknown and can't be checked without a physical watch:

1. Whether standard OS-level BLE pairing/bonding with the watch works.
   `Docs/BLE-Services-Overview.md:7-8` says a "bonded, encrypted connection" is
   required but not how pairing is triggered — standard BLE Security Manager
   pairing is assumed, unverified.
2. Whether a `WRITE`/`WRITE_DATA`/`WRITE_PACING` sequence, sent by a minimal
   client against the documented protocol, actually lands the bytes in the
   app's own sandbox, and whether the app can read them back.

## Build note: `APP_USE_ICONS Off` does not work for a Utility-type app

While scaffolding the probe app (`APP_TYPE "Utility"`), `set(APP_USE_ICONS
"Off")` (the pattern HybridX Streak's *Glance*-type probe uses to skip icon
artwork) got past `app_merging.py`'s `Image.open()` call but then failed with:

```
app_merging.py: error: -normal_icon is required for non-Glance apps
```

Reading `una-sdk/cmake/una-app.cmake:400-430`: `APP_USE_ICONS Off` only omits
the `-normal_icon`/`-small_icon` flags from the cmake wrapper's call to
`app_merging.py` — it doesn't tell `app_merging.py` itself to make those flags
optional. `app_merging.py`'s own argparse requires `-normal_icon` unconditionally
for every `-type` other than `Glance`. So **`APP_USE_ICONS Off` only works for
Glance-type apps** (why Streak's Glance probe could use it); a Utility-type app
needs real icon files regardless of whether the app is throwaway.

Fixed by adding placeholder icon artwork (`Resources/make_icons.py`, a plain
teal tile with a white "P", same generation pattern as Race's and Streak's real
icons) and pointing `RESOURCES_PATH` at it in
`Tools/Probe/Software/Apps/Probe-CMake/CMakeLists.txt`.

## Findings for Jon (fill in after running `docs/PROBE.md`)

- [ ] Did OS-level Bluetooth pairing/bonding with the watch succeed, and how
      was it triggered (a prompt from the OS, a code on the watch, something
      else)?
- [ ] Watch's advertised BLE name, for `send_plan.py --name`.
- [ ] MKDIR on an already-existing directory: what status byte came back?
- [ ] Did the classic WRITE flow complete (`WRITE_PACING` reaching
      `freeSpace == 0`)?
- [ ] Did the probe app on the watch report **GO**, and did its preview match
      the JSON `send_plan.py` sent?
- [ ] Anything `send_plan.py`'s console output flagged as unexpected.

## Gate P0 — what decides next steps

**GO** (the write lands and the watch app reads it back): Option A (a real
phone app over BLE FTS) is viable. Scope P1: the workout data model and the
real-time target/pace-zone engine.

**Not GO**: fall back to Option B, `AppConfig`'s compact single-preset path. No
workaround attempted — same rule as Streak's Gate 0.

## P1: the workout data model and pace/HR engine (26 September 2026)

Built ahead of Gate P0's result, since it's useful whichever way that lands
(a real phone app over BLE FTS, or the `AppConfig` fallback) — a pure C++
workout model and real-time evaluation engine, no GUI, no phone app, no
transport decision. `Software/Libs/Core/`, alongside `Tools/Probe/` (P0's
disposable BLE check, which this doesn't touch).

### P1.1 What's real and what's app-side-only

Every FIT-related enum was checked directly against
`una-sdk/Libs/Header/SDK/Fit/FitProfile.hpp` before use, not taken on trust:

- `DurationKind` mirrors `WktStepDuration` (`FitProfile.hpp:65-67`) —
  `Time=0, Distance=1, Open=5, RepeatUntilStepsComplete=6` — with the same
  numeric values, so a later lowering to FIT is a plain cast.
- `StepIntensity` mirrors `Intensity` (`FitProfile.hpp:56`) the same way.
- **`TargetKind` (Pace/HeartRateZone/HeartRateBpm) is app-side only.**
  `WktStepTarget` (`FitProfile.hpp:68`) defines only `Open=2` — there is no
  FIT-encodable way to record a real pace/HR number today. `ActivityWriter::
  addWorkout` (`Examples/Apps/Running/Software/Libs/Header/ActivityWriter.hpp:98-103`,
  `.cpp:303-338`) already writes `Workout`/`WorkoutStep` FIT messages for an
  arbitrary step array — that encode path is not a gap — but it always
  writes `TargetType = Open`, because that's all the on-watch interval
  feature currently produces. P1 keeps that unchanged: the FIT file
  continues to record `Open` for every step; the real target lives only in
  `Target`/`TargetKind`, evaluated live by `TargetEvaluator`. **Needs Jon's
  sign-off before P2** — see Open questions below.
- No `Power`/`Cadence` target kind: `Docs/ExternalSensors.md:14-30` — HR
  only, cadence/power bits reserved and unimplemented.
- `HrZones::zoneOf` generalises the Workout example's `Service::getHrZone`
  (`Examples/Apps/Workout/Software/Libs/Sources/Service.cpp:904-916`) to the
  SDK's real ceiling (`RequestSystemSettings::skMaxHearRateTh = 7`,
  `Libs/Header/SDK/Messages/CommandMessages.hpp:201`) rather than that
  example's own hardcoded 5-zone cap — Core takes thresholds as plain
  arguments; sending `RequestSystemSettings` for real is a later,
  watch-integration concern.
- Repeat-step encoding (`Step::durationValue`/`repeatCount` for
  `RepeatUntilStepsComplete`) copies `ActivityWriter::WorkoutStepData`'s
  convention exactly (first-step index / iteration count as comments in
  `ActivityWriter.hpp:98-103` state verbatim), so a later lowering to FIT is
  a field copy, not a redesign.
- Pace smoothing reuses `SDK::Metric::SpeedSmoother` directly — not
  reimplemented; `TargetEvaluator::Sample` is meant to be filled from its
  `getPace()`.

### P1.2 What was built

`Software/Libs/Core/{Header,Sources}/`: `WorkoutTypes.hpp` (`Workout`,
`Step`, `Target`, the mirrored enums), `WorkoutValidation` (rejects an empty
workout, a bad repeat index, `repeatCount == 0`, and nested repeat ranges —
P1 supports one active range at a time), `HrZones`, `TargetEvaluator`
(`classify()` — a pure function to `Under`/`InZone`/`Over`/`NoSample`/
`NoTarget` — plus `CueDebouncer`, which only reports a state change after 3
consecutive agreeing ticks), `WorkoutEvents` (modeled directly on
`hybridx-streak`'s `StreakEvents.hpp`: a fixed 8-slot array, a
`ZoneChanged`-yields-first drop policy), and `WorkoutEngine` (the
step-sequencing cursor — no internal clock, `tick(nowMs, distanceCm,
Events&)` takes both as arguments, unsigned subtraction for elapsed time
across the wrapping ms clock, same rule as Race/Streak).

45 host tests (`Tests/Host/`), all green — `Time`/`Distance` step completion
exactly at the boundary, `Open` steps never auto-advancing, a repeat block
visiting its steps in order exactly M times before falling through, a
clock-wraparound case, and the debouncer's run-length/reset behaviour. Every
Core source also compiles clean with the ARM cross-compiler
(`arm-none-eabi-g++ -Wall -Wextra -Wpedantic`, no warnings) — a
compile-check only, confirming no accidental SDK/heap dependency, since
nothing calls Core yet.

One GCC internal-compiler-error was hit and worked around during test
writing: repeatedly reassigning an aggregate-initialised `Events{}` to the
same local (`events = Events {};`) crashed this container's `g++` inside
`gimplify.cc`. Fixed by giving each tick its own freshly-declared `Events`
local instead of reusing one — not a bug in `Events` itself, just an ICE
triggered by that specific reassignment pattern with this compiler.

### Open questions for Jon

1. **The target-representation question (P1.1 above).** Is "the real
   target lives only in the app's own workout format, FIT keeps recording
   `Open`" acceptable long-term, or should the recorded activity somehow
   carry the intended target (e.g. via a `developer_data` field —
   `FieldDescription`/`DeveloperDataId` exist in `FitProfile.hpp:174-183`,
   real but unused by any app in this SDK so far)? Also confirm P1 must
   **not** write a `TargetType` value beyond the defined `Open=2` member —
   this plan assumes no.
2. **Repeat-block nesting.** P1 supports one active range at a time
   (`WorkoutValidation` rejects nesting). Is that enough, or is nesting (a
   set of sets) a real requirement?
3. **"Open" step termination.** The enum exists; nothing found says how an
   Open-duration step ends on-watch. P1 assumes the standard "ends on a
   manual lap/advance" convention — an assumption, not a confirmed platform
   fact.
4. **Pace target shape.** `Target.low`/`.high` models a band (fast/slow
   bounds in sec/km). Garmin Connect sometimes expresses a pace target as
   "value ± margin" instead — either fits the struct, but which does the
   eventual builder UI expose?
5. **Sizing constants.** `Workout::kMaxSteps` (20) and `kNameChars` (32) are
   placeholder bounds with no spec behind them. Is there an expected upper
   bound for how large a workout needs to be?
