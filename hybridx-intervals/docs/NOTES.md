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

- [x] Did OS-level Bluetooth pairing/bonding with the watch succeed, and how
      was it triggered (a prompt from the OS, a code on the watch, something
      else)? **Partly answered on 29 September 2026, from a phone, not a PC**
      (hybridx-trail/docs/NOTES.md, "Phone delivery over BLE"). The bond the UNA
      app makes is the phone's, and a second Android app (nRF Connect) used it
      with no prompt, while the UNA app stayed connected. FTS reported version
      5 and answered LISTDIR `/Apps`. A PC pairing on its own is still untested.
- [x] Watch's name: **"UNA WATCH 042648"** (Jon's watch). Note that the watch
      **didn't show in a scan** while the UNA app was connected, so
      `send_plan.py`'s scan may need the UNA app closed, or the phone's
      Bluetooth off.
- [ ] MKDIR on an already-existing directory: what status byte came back?
      Not run for Intervals. Route Sender runs MKDIR only when a folder is
      missing (Trail NOTES); the status byte is still unrecorded.
- [x] Did the classic WRITE flow complete (`WRITE_PACING` reaching
      `freeSpace == 0`)? **Answered by Trail, not by this probe:** Route Sender
      wrote 685 bytes (version 5, windowed) and DIGEST matched. The classic
      version 4 flow that `send_plan.py` uses was not exercised.
- [ ] Did the probe app on the watch report **GO**, and did its preview match
      the JSON `send_plan.py` sent? Never run. Retired (see "Gate P0: closed").
- [ ] Anything `send_plan.py`'s console output flagged as unexpected.

## Gate P0 — what decides next steps

**GO** (the write lands and the watch app reads it back): Option A (a real
phone app over BLE FTS) is viable. Scope P1: the workout data model and the
real-time target/pace-zone engine.

**Not GO**: fall back to Option B, `AppConfig`'s compact single-preset path. No
workaround attempted — same rule as Streak's Gate 0.

## Gate P0: closed by Trail's evidence (30 September 2026)

The Intervals probe (`Tools/Probe`, `send_plan.py`) was never run. Its
question was whether a second app can write a file into a watch app's own
folder over BLE File Transfer and have the watch app read it. Trail answered
that on the same watch and the same phone:

- **Write:** Route Sender 0.1.1 (a standalone Android app, not the UNA app)
  wrote into `/Apps/HybridXTrail/Routes/` over FTS version 5, with the UNA app
  still connected, no scan and no pairing prompt. DIGEST (size and CRC-32)
  matched. (`hybridx-trail/docs/NOTES.md`, "Route Sender run 2".)
- **Read-back:** a watch app reads its own folder as ordinary files, which
  Trail already does with `Routes/` over USB.

**Gate P0 is GO for Option A** (a real phone sender over BLE FTS). The
`AppConfig` fallback (Option B) stays available as an on-watch preset.

What this does **not** cover, and stays open:
- The probe's own PC path (Python, `bleak`, OS-level pairing). Not needed if the
  sender is an Android app that uses the phone's existing bond.
- **An Intervals file reaching an Intervals watch app.** Trail's read-back on
  the watch after a BLE write is itself still ticked open in its notes
  ("Trail shows and loads a route sent this way"). Same for whether an app
  sees a file that arrives while it is running.
- iPhone: no path except UNA adding "send file to app" (`hybridx-trail/docs/UNA_GPX_REQUEST.md`).

The probe's code and `docs/PROBE.md` stay in the repo as reference and are
still built by CI; nothing depends on them.

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


## P2: the workout file and its parser (30 September 2026)

Agreed with Jon: JSON, one workout per file, in the app's `Workouts/` folder.
Schema in `docs/WORKOUT_FILE.md`. Parser `Software/Libs/Core/Sources/
WorkoutParser.cpp` (`parseWorkout`), pure C++ and SDK-free like the rest of
Core: no heap, integers only, every read bounds-checked, first error sticks.

### What was built and checked

- **`parseWorkout(buf, len, Workout&) -> ParseResult`** reads the whole file,
  converts the wire's human units (seconds, metres, sec/km) into `Step`'s ms
  and cm, then runs `validate()`. A parsed workout therefore always meets
  `WorkoutEngine::start`'s precondition. `ParseResult` carries the error, the
  `ValidationError` if `Invalid`, and the byte offset.
- **Strict on purpose:** unknown names, non-integers, negatives, out-of-range
  numbers, `low > high`, a second `steps` key and an over-long name are each an
  error; nothing is defaulted or truncated. Unknown keys are skipped (bounded
  to nesting depth 8) so a newer sender can add fields. `v` is checked when
  read, so a newer file is reported as `BadVersion` even if it has things this
  reader cannot parse, provided `v` comes first.
- **Tests:** `Tests/Host/WorkoutParserTest.cpp`, 23 cases, plus two fixture
  files (`Tests/Host/fixtures/`). Whole suite **68 tests**, all green, both
  plain and with `-fsanitize=address,undefined`. Every prefix of the worked
  example is parsed from an exactly-sized heap buffer (so any over-read is an
  ASan error), and so is every one-byte corruption from a set of 14 bytes.
  The worked example also runs through `WorkoutEngine` to completion.
- **Compile check:** `g++ -std=c++17 -Wall -Wextra -Wpedantic -Wconversion
  -Wshadow -fno-exceptions -fno-rtti` on the new source: no warnings. The
  container has no ARM compiler, so this is the host compiler, not the
  cross-compile check P1 did. CI's watch build is the arbiter.

### Finding: a comment in `WorkoutTypes.hpp` had pace backwards

The `Target` comment said `low` is the slow bound and `high` the fast one. The
code and its tests do the opposite: `TargetEvaluator::classify` treats `low` as
the **fast** bound (`paceSecPerKm < low` is Over, `> high` is Under), i.e. a
numeric band with `low <= high`. The comment was wrong, not the code; it is
fixed, and the file format follows the code ("4:00 to 4:10 per km" is `low: 240,
high: 250`). The parser refuses `low > high` for every kind, so a sender that
gets it backwards is told, rather than the watch silently reading an empty band.

### Proposed defaults for the P1 open questions (**awaiting Jon**)

Nothing below is decided; these are what P2 assumed so the schema could be
written. Each is cheap to change.

1. **Targets stay app-side; FIT records every step as `Open`.** Consequence: a
   Strava or Garmin Connect upload shows no targets. A `developer_data` field
   (`FitProfile.hpp:174-183`) could carry them later, unproven.
2. **One repeat block at a time.** Nesting is refused by `validate()`. A
   "pyramid" or "sets of sets" would need it.
3. **An `open` step ends on a manual press.** Still an assumption about what
   the platform's other apps do; not confirmed from the SDK.
4. **Pace is a band, `low` and `high` on the wire.** The phone builder can still
   offer "target and margin" and store the band.
5. **Limits stay at 20 steps and 31-character names** for v1, and the file at
   4,096 bytes. No spec behind them: revisit after real use.

### Not done in P2 (by agreement)

The watch app, the phone sender, and the UNA request. Next, if these defaults
are fine: P3, the watch app started from Trail's LVGL scaffold (workout list,
preview, run screens), then P4, sending workouts (Route Sender or a fork), and
the UNA request widened to cover workouts as well as GPX.

## P3a: the app starts from RunLVGL (2 October 2026)

Agreed with Jon (P3 plan): the app **replaces** RunLVGL's basic built-in
intervals with the workout engine, takes workouts from files (USB now; the
phone or UNA's app later) and, after the P3 gate, a better on-watch builder.
Running only: bike workouts are listed but not runnable.

- **Verbatim copy first.** `Software/` is the SDK's RunLVGL (`a7a995a1`),
  committed unchanged on its own (as Trail's T2.1), so every Intervals change
  is a readable diff against UNA's original: the GUI in `Apps/LVGL-GUI`, the
  CMake project in `Apps/HybridXIntervals-CMake`, the service in `Libs/App`.
  `Libs/Core` (P1, P2) sits beside it; `Libs/libs.cmake` compiles both.
- **Identity.** `HybridXIntervals`, type `Activity`, development APP_ID
  `DBDC6FEA92394563` (first 16 hex of md5("HybridXIntervals")). The name on
  the watch is **"HX Intervals"**: `app_merging.py:221` keeps 15 bytes of the
  name, and "HybridX Intervals" is 17. Version from `intervals-v*` tags
  (`Software/cmake/intervals-version.cmake`, as Trail's). The title on the
  start screen reads "INTERVALS".
- **Icon** (`Resources/make_icons.py`): a workout profile in blocks, low grey
  warm-up, three tall lime work blocks with short grey rests, a low grey
  cool-down. The P0 probe's placeholder icons moved to
  `Tools/Probe/Resources`, with the probe's `RESOURCES_PATH`.
- **CI** builds the app next to the probe and adds its `.uapp` to
  `watch-apps`.

### P3a verified

- Watch target, local compile check (Ubuntu's `arm-none-eabi-gcc` with
  Race's syscall stubs, so **not installable**; the watch copy comes from
  CI): `HybridXIntervals_0.0.0-dev.uapp`, 419,996 bytes. Only warnings are two
  in SDK files. Core (P1, P2) is now compiled by the ARM compiler for real,
  with no warnings: the cross-compile check P2 could not do.
- Simulator builds and runs (dummy video driver, 5 s, clean shutdown).
- Behaviour is still RunLVGL's; P3b replaces its intervals.

## P3b: workouts from files drive the run (2 October 2026)

### P3b.1 What changed

RunLVGL's fixed warm-up / run / rest / cool-down state machine (and the
`Settings::Intervals` it read) no longer drives anything in the service. A
workout from `Workouts/` does, through three new pure pieces of Core, all
host-tested:

- **`WorkoutStore`** (modelled on Trail's `Navigator`): lists `Workouts/`
  (creating it), parses every `.json`, keeps a summary of each (name, sport,
  totals with repeats expanded, the parser's error and byte offset if it
  can't be read), sorts by name, and remembers the choice in `workout.sel`.
  Files are at most 4 KB, so there's no index cache: every scan parses again.
  16 workouts at most (the rest are flagged, not listed); names over 47 bytes
  are skipped, not truncated. Bike workouts are listed but not runnable.
- **`WorkoutRunner`**: the engine, the target check and the cues in one
  place. The service gives it the activity's **active** time and distance
  once a second, so a pause needs no special case. It reports what the
  service must do: save a lap (every step is its own lap), announce a new
  step, buzz a cue, finish.
- **`WorkoutSummary`**, and two engine additions: `repeatPosition()` ("2 of
  6", right on the first pass too, which `iterationsRemaining()` was not) and
  `nextStepIndex()` (for a "next" line, without moving the engine).

The service (`Libs/App/Sources/Service.cpp`):
- scans and restores the choice when the GUI starts; messages 0x20-0x25
  (`Commands.hpp`) carry the list, the chosen workout and the cues, as
  pointers into static storage like Trail's routes;
- intervals mode runs the chosen workout; with none chosen it is a plain run;
- writes the workout to the FIT file as a 1:1 copy of the file's steps, so a
  step's index is its `workout_step` message index, and links each lap to it
  (targets are not written: FIT here has only "open", P1.1);
- feeds RunLVGL's existing interval screens through `Track::IntervalsData`
  (phase from the step's intensity, timer from its duration), plus new
  fields for the P3c screens: step, next step, repeat position, target band,
  zone.
- About 8 KB of workout state lives in static storage, not on the 10 KB
  service stack.

### P3b.2 Cues (proposed defaults, **awaiting Jon**)

- No cue in the first **15 s** of a step: pace and heart rate still describe
  the last step's effort.
- Then a cue when the state has been Under or Over for **3 seconds running**
  (the P1 debouncer).
- **Too slow / too low:** three short buzzes and beeps ("pick it up").
  **Too fast / too high:** one long beep and one buzz ("ease off").
- A **reminder every 60 s** while still out of the band. Nothing for coming
  back in: the screen shows it.
- Every new step: the alert screen, and a buzz if it changed by itself (not
  after a press), as RunLVGL did.

### P3b.3 Found in the simulator: LVGL's pool on RunLVGL's own screens

Going from the start screen to RunLVGL's intervals menu crashed the
simulator (a null from `lv_malloc` inside `lv_style_init`, building the menu's
sensor row): RunLVGL builds the next screen before freeing the last, so both
must fit in LVGL's 40 KB pool, and these two do not. This is RunLVGL's own
code, unchanged by us. Ported Trail's fix (its T3.2): load the new screen's
empty root, free the old screen, then build. Pool peak after the fix: 78 %.

### P3b.4 Test workouts and the simulator script

- `Tools/TestWorkouts/`: example files (6 x 400 m, 5 x 1 km, a heart-rate
  tempo, a bike workout, a broken file, and `sim-short.json` sized for the
  simulator's fast runner), with a README for copying them to the watch.
  A host test checks each parses as the README says.
- `docs/experiments/sim_run.sh [workout] [seconds] [shots]`: puts the test
  workouts on a pretend watch, chooses one (by writing `workout.sel`, until
  the P3c list exists), starts it through RunLVGL's intervals menu, ends the
  open cool-down with R2, saves, and prints the service's log lines.

### P3b.5 Verified

- **Host tests: 97**, all green, plain and under ASan/UBSan (WorkoutStore 11,
  WorkoutRunner 9, WorkoutSummary 4, the engine's new queries 4, the test
  workouts 1, as well as P1-P2's).
- **Watch target** compile check: 428,380 B `.uapp`, no warnings in our code
  (Ubuntu toolchain with stubs, not installable; CI builds the real one).
- **Simulator, end to end** (`sim_run.sh sim-short.json 230`): the store found
  6 workouts, logged the broken one ("error 2 at byte 21"), restored "Sim
  short", and ran it: a 20 s warm-up, 3 x (200 m, 20 s rest), and an open
  cool-down ended with R2. One "over target" cue per 200 m (the simulated
  runner is far faster than 4:00-4:20/km), about 17 s into the step, after
  the 15 s settling time and the 3 s debounce. Laps closed at 200.3, 203.9
  and 202.7 m (1 s ticks at 5-6 m/s). LVGL pool peak 82 %.
- **The FIT file, decoded** (`fitdecode`): `workout` "Sim short" with 5
  `workout_step`s exactly as the file (time 20 s warm-up; distance 200 m
  active; time 20 s rest; repeat from step 1, 3 times; open cool-down), 9
  laps linked to steps 0, 1, 2, 1, 2, 1, 2, 4 and the last one (after the
  workout ended) unlinked, and the session closed (9 laps, 1,213 m).

### P3b.6 Known, for P3c

- RunLVGL's GUI fills the first second of an intervals run from its old
  settings (`Model::trackStart`), so a timed warm-up reads "Open" until the
  service's first update. P3c fills it from the chosen workout instead.
- The intervals screens still show only phase and timer; the target band,
  zone colour, repeat position and next step are in `Track::IntervalsData`
  for P3c to show.
- `Settings::Intervals` and RunLVGL's seven setup screens are still compiled
  and reachable, but no longer change what runs. P3c removes them.

## P3c: the screens (2 October 2026)

Screenshots of every step: `docs/screens/` (`docs/experiments/capture_screens.sh`,
which drives the simulator through the list, a preview, and a whole run of
`sim-short.json`).

### P3c.1 What there is

- **Start screen:** RunLVGL's wheel; the Intervals item's hint is the workout
  chosen (amber) or "No workout". It opens the list, after a rescan (files may
  have been copied in over USB since).
- **Workout list** (modelled on Trail's route list): "No workout" (a plain run),
  then the workouts by name with their totals ("2.4 km, 29 min"; minutes
  rounded up; "1 open" counts steps that end on a press), then "Add workouts"
  ("copy files by USB", or "16 shown: remove some"). A file that can't be read
  is listed by its file name, in red, with the reason ("not a workout file",
  "newer format: update app", "a repeat is wrong"...); a bike workout in grey,
  "bike: not yet". Neither opens.
- **Preview:** the name, the totals, then every step on its own line ("Run
  400 m", "Rest 1:30", "Cool-down, open") with its target under it ("@ 3:50-4:10
  /km"), a repeat block under an amber "6 x". L1/L2 scroll ("1 more"); R1
  starts; R2 goes back and puts back the workout chosen before. Opening it
  chooses the workout, as Trail's route preview does.
- **Countdown:** the workout's name, totals and first step, where RunLVGL showed
  reps, run and rest. R2 goes back to the preview.
- **Run face** (RunLVGL's intervals face): the title is the step and its pass,
  "RUN 2/6"; the big timer counts down a timed step, shows the distance left
  of a distance step, or counts up an open one; under it, the measure the
  target is set in (pace, or heart rate) **coloured against the band: blue
  under, lime in, red over**, white while settling (15 s) or with no target;
  at the bottom, the band ("4:00-4:20", "Zone 4"). Without a target it is
  RunLVGL's: pace when running, heart rate when resting.
- **Cue banner:** on each cue, for 4 s over any face: "SPEED UP" / "SLOW DOWN"
  for a pace target, "PUSH ON" / "EASE OFF" for heart rate, blue or red.
- **Step alert** (RunLVGL's): its pass ("1/3") and its target ("@ 4:00-4:20
  /km") where RunLVGL had the runner.
- RunLVGL's **seven set-up screens are removed** (the intervals menu, repeats,
  run/rest metric and the time and distance pickers). `PickerLogic.hpp` and
  the picker descriptors in `AppMenu.hpp` stay for the on-watch builder (P3d);
  `Settings::Intervals` stays in the settings file, unused.
- **Text** is pure Core, host-tested: `WorkoutText` (step lines, targets,
  totals, problems; integer formatting, British English) and `TextFold`
  (copied from Trail: accents to ASCII for the watch fonts). The GUI also
  compiles Core now, for them.

### P3c.2 Proposed, **for Jon**

- Colours: under = sky blue, in = lime, over = red. (Garmin-like; the
  display has 64 colours.)
- Banner words: "SPEED UP" / "SLOW DOWN" (pace), "PUSH ON" / "EASE OFF" (heart rate).
- The countdown's R2 goes back to the preview; the list's R2 to the start
  screen.

### P3c.3 Verified

- **Host tests: 113**, green, plain and under ASan/UBSan (WorkoutText 8,
  TextFold 8, as well as P1-P3b's).
- **Watch target** compile check: 422,156 B `.uapp` (smaller than P3b's:
  the set-up screens are gone), no warnings in our code. Not installable
  (Ubuntu toolchain with stubs); CI builds the real one.
- **Simulator walkthrough** (`capture_screens.sh`, 31 frames): every list entry
  kind, a preview scrolled, a preview left (the choice put back), then "Sim
  short" run through: warm-up counting down from its first second, each rep's
  alert with its target, the pace white while settling then red with "SLOW
  DOWN", RunLVGL's other faces, the rest's alert and face (heart rate), the
  open cool-down ended with R2, "Workout completed", saving, the summary.
  Service log: 9 laps, one cue per rep, track stopped. LVGL pool peak 82 %.
- Found and fixed in the captures: totals lines too wide for the round screen
  at the top (the preview and the countdown now fit them, stepping down to
  14 pt); two key counts in the script.

## Gate P3: what Jon can test on the watch

The `.uapp` comes from CI: **Actions → Watch builds →** the newest green run on
the branch **→ Artifacts → watch-apps → `HybridXIntervals_*.uapp`**.

1. **Install:** on the watch drive, make `Apps/HybridXIntervals/`, copy the
   `.uapp` in, eject safely, power-cycle. "HX Intervals" in the app list.
2. **Add workouts:** open the app once (it makes `Workouts/`), connect USB again,
   and copy the files from `Tools/TestWorkouts/` (not `sim-short.json`) into
   `Apps/HybridXIntervals/Workouts/`. Eject, open the app.
   - Expect: Intervals → the list shows them by name, the broken one in red,
     Zone 2 spin greyed "bike: not yet".
3. **A real session:** choose "6 x 400 m" (or "5 x 1 km"), check the preview, and
   run it. Expect: a countdown, the warm-up counting down, an alert and buzz at
   each new step, the pace coloured against 3:50-4:10/km, a banner and buzz
   when off pace for a few seconds (three short = speed up, one long = slow
   down), a reminder each minute if it stays off.
4. **Afterwards:** the activity in the UNA app, then Strava or Garmin Connect.
   Expect: one lap per step, and the workout's steps (as "open" targets).

Decisions for Jon at this gate: the cue rules (P3b.2), the colours and banner
words (P3c.2), the P1 defaults still open (P2 section), and the shape of the
on-watch builder (P3d, below).

### P3d (next): the on-watch builder, proposed

Not built: the design is Jon's call. A suggestion to react to:
- **Quick builder:** a short wizard writing a new `Workouts/*.json` through a
  `Core/WorkoutWriter` (round-trip tested against the parser): reps (1-20),
  work (time or distance, RunLVGL's pickers), rest (time), target (none, pace
  band, or heart-rate zone), warm-up and cool-down (open or timed).
- **Duplicate and tweak:** from a workout's preview, copy it and change its reps
  or its target band (the common "same session, a bit faster" week to week).
- Open questions: how a pace band is entered with four buttons (a centre pace
  and a fixed width, say ±5 s/km, is quickest); whether quick builder covers
  enough, or free-form step editing is wanted.
