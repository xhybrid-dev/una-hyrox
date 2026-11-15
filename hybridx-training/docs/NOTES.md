# HybridX Training: notes

The running log: decisions, what was read in the SDK, what was checked and how, and what is
still open. Same conventions as the other apps' notes: evidence first, file citations into
`una-sdk/`, nothing claimed that was not read or run.

---

## TP0: decisions and desk checks (14 November 2026)

### Decisions (Jon, 14 November 2026)

| Decision | |
|---|---|
| Build it | Yes, knowing the overlap with UNA's Running (brief §2) |
| A new app, or a mode of X Run | **A new app** |
| The name | **X Training** on the watch |
| Reviews | An independent review of the plan and a running coach's review of the plans: done, `REVIEW.md` |

### Names and identities

| Thing | Value |
|---|---|
| Shown name (`APP_USER_NAME`) | `X Training` (10 bytes; the limit is 15) |
| Project name in the docs | HybridX Training |
| Repository folder | `hybridx-training/` (was `hybridx-plan/` in the first brief, commit `0e1b710`) |
| App folder on the watch, and the link's `app` | `HybridXTraining`: 15 characters, the link's limit exactly (`ActivityLink.hpp:55`) |
| Development APP_ID | `6E050D707D4D2D32` (first 16 hex of md5 of `HybridXTraining`); the portal issues the real one at release |
| Glance | `HXTrainingGlance`, development APP_ID `C4B5BAD6DC827E16` (md5 of that name) |
| Tags | `training-vX.Y.Z` |
| Shared files | `act_training.json` (link slot `training`), `training_today.json` (the glance's), `training_vo2_0..2.json` (the Run link) |
| Phases | TP0-TP7 |

### What was read in the SDK for the build (pin `sdk-v1.6.2`)

- **A valid Program** (`Libs/Header/SDK/Fit/FitWorkoutReader.hpp`, "What it checks"): at
  least one step, at most `kMaxSteps` (100); a repeat points to an earlier step; repeat
  blocks may nest but not overlap, at most `kMaxRepeatDepth` (4) deep; counts 1 to 99. The
  engine never stops on a repeat step (`WorkoutEngine.hpp`).
- **Raw FIT fields the builder must fill** (`FitWorkoutWriter.hpp`: steps are written from
  `Step::raw`; `Libs/Source/Workout/Intervals.cpp:28-71` shows how UNA fills them):
  `durationType` and `durationValue` (ms for time, cm for distance; for a repeat, the index
  of the block's first step), `targetType`, `targetValue` (for a repeat, the count),
  `intensity`. For targets (`Libs/Source/Fit/FitWorkoutReader.cpp:673-699`):
  - speed: `targetType` Speed, `targetValue` 0, `customLow` and `customHigh` in mm/s;
  - heart-rate zone: `targetType` HeartRate, `targetValue` the zone, 1 to 5;
  - heart rate in bpm: `targetValue` 0, custom values bpm + 100.
- **Starting a workout** (RunLVGL `Service.cpp`, `startWorkout()`): after `mProgram` is
  filled, `mEngine.start(mProgram, 0, 0)`, `mLeadIn.stepStarted()`, `startWorkoutStep()`.
  The details screen receives `&mProgram`. UNA's `ScheduleState` is tied to the library's
  files and is not used: the plan keeps its own log.
- **Heart-rate zones** (`Service::hrBand()`): zone n is from threshold n−1 to threshold n
  of the watch's own thresholds.
- **The date** (`Service::localDate()`): yyyymmdd from the kernel's local time; 0 when the
  year is before 2020 (clock not set).
- **Alerts:** `ZoneAlerts` says when (`kQuietStartSec` 20, `kRepeatSec` 30); what plays is
  `Service::playZoneAlert()`, app code. So "too fast only" on easy steps is our change, not
  the SDK's.
- **Config fields:** `SDK::AppConfig` has `has`, `getInt`, `getString`, `setBool`, `setInt`,
  `setString`, `save` (`Libs/Header/SDK/AppConfig/AppConfig.hpp`). No pattern check on the
  watch. Patterns are an implicit full match and must not contain `^` or `$`
  (`Docs/app-config-fields.md` §3.2).
- **The link:** `kSlotCount` 5, `kMaxSlots` 8 (`ActivityLink.hpp:172-173`);
  `StreakNow.hpp:119` sizes an array by `kSlotCount`, so a new slot changes every app's copy.

### The pace model, checked

Details in `REVIEW.md` §1 and `PLANS.md` §2. In short: threshold 88% and interval 97.5% of
the fitness reproduce the published rows found (fitness 40, 45, 51) to the second; easy is
62.5-72.5%; repetition is interval less 6 s per 400 m; Daniels–Gilbert and Riegel agree
within 15 s at 10 km and the half from 18:00 to 36:00 for 5K. The scratch scripts that
produced the tables are not in the repository; TP1's `PaceModel` tests replace them.

### The build environment (this container)

- `cmake`, `ninja`, `g++` and `xvfb-run` are present. X Run's host tests configure, build
  and pass here: **200 tests**. TP1's test project can copy `hybridx-run/Tests/Host`.
- Not present: `arm-none-eabi-gcc` and `xdotool`. Not needed for TP1. For TP2's compile
  check and captures, install as Race NOTES 5.34 and Trail Quest's "Toolchain notes" say.
  Watch builds come from CI as always.
- Network: web search works; `test.knowt.com` and `cdbaca.github.io` are blocked, as the
  journal sites were for RACE_PREDICTIONS. Reading Daniels' table needs the book or an
  allowed source.

### Open, carried forward

| Item | Who | Blocks |
|---|---|---|
| Ten rows of Daniels' training-pace table, from the book, for the fixture file | Jon (or a session with access) | The store release, not TP1 |
| The plans' content (`PLANS.md` §9) | Jon, at the TP1 gate, from `plan_print` | Nothing: text files |
| The question to UNA (below) | Jon, if he wants it sent | Nothing |
| Race's T23: does the UNA app show and check config fields | Jon | TP2's phone set-up (the watch set-up covers it) |
| How X Run and X Training sit together on the store | Jon | TP7 |

### Draft question for UNA (not sent)

> I'm building a training-plan app for the watch: it generates a 10K or half-marathon plan
> on the watch from the runner's 5K time and runs each day's session with the SDK's workout
> engine. Before I go far: are training plans, or schedules delivered from the phone into
> `:/Workouts/Schedule`, on your roadmap for Running? If they are, could a third party
> supply the schedule? I'd rather build on yours than beside it.

---

## TP1: the pure core (15 November 2026)

Jon: "Let's start with the build, please go as far as you can." The plan was the one recorded
here at TP0 (order of work below), so it was followed without a separate plan-mode stop.

### What was built

All in `Software/Libs/Core` (namespace `XTraining`), plus the tools:

| Piece | Files | What it does |
|---|---|---|
| Dates | `Date.hpp` (header-only) | yyyymmdd as the SDK's `Ymd`, day numbers, weekdays, Mondays |
| Time strings | `TimeText.hpp` (header-only) | reads the phone's `mm:ss`, `h:mm:ss`, `YYYY-MM-DD` strictly (anything else is 0); writes paces and times |
| Paces | `PaceModel.*` | fitness from a race, the training paces, bands in mm/s, predictions, the half's gap; single precision |
| Plan types | `PlanTypes.hpp` (header-only) | sessions, weeks, plans, `isHard()` |
| The plans | `Tools/Plans/*.plan` → `gen_plans.py` → `Header/data/PlanData.hpp` (generated, header-only) | the three plans as constexpr tables; the generator refuses bad edits |
| Placing a week | `WeekLayout.*` | `PLANS.md` 4.2 on the runner's days |
| Calendar | `PlanCalendar.*` | the start rule, counting back from a race, dropping base weeks, locating a day |
| Schedule | `PlanSchedule.*` | the session on a day: trial and recent weeks, the race week round the race day, the level |
| Builder | `PlanBuilder.*` | levels and caps (`PLANS.md` 3.3, 4.1), the half's distance floor, and the `SDK::Workout::Program`, raw FIT fields included, with each step's alert mode and the session's estimates |
| State | `PlanState.*` | `plan.json`: set-up, calendar, a log of 84 days; bounded reader |
| Words | `PlanText.hpp` (header-only) | names, short names, purposes, "4 x 4 min"; ASCII |
| plan_print | `Tools/plan_print/` | a whole plan for any runner as Markdown; also writes the golden files |

### Decisions made while building (each cheap to change)

- **Every rep is followed by its recovery**, the last one included: `[rep, jog] x n`, then the
  cool-down. The alternative (UNA's `lastRest = false` layout) puts the last rep outside the
  repeat block, which breaks the engine's "rep 3 of 6" count (`Intervals.hpp`,
  `intervalsRepeat()`). The cost is one extra recovery of a minute or two.
- **Short reps** (repetitions) are `AlertMode::AtEnd`; **strides and 30-30s** are
  `AlertMode::None` with a band (the gauge has a value to show); **hills and time trials**
  have no target. **Easy steps** (easy, long, warm-up, cool-down, the 3 minutes between 30-30
  sets) are `FastOnly`. The service applies these modes in TP2.
- **Strides** are at repetition pace, ±8 s/km; their recoveries, and all jogs, have no target.
- **Time trials and races** start with a warm-up ended by a lap press; a time trial ends with
  an open cool-down; a race has none.
- **Level 1's hills** in place of repetitions are 45 s each, two thirds of the reps, at least 4.
- **Estimates:** a jog is easy-slow pace plus 45 s/km; an open warm-up counts as 15 minutes; a
  hill is threshold speed for half the distance, the jog down 1.5 times the rep.
- **The fitness figure** is computed in single precision. The tests show it reproduces the
  review's double-precision paces to the second.

### Verified

- **Host tests: 59, all passing** (`hybridx-training-host-tests`), and `plan-data-current`
  (the generated header matches the plan files): `ctest` 2 of 2. Also run under
  **AddressSanitizer and UBSan**: clean.
  - Paces: the published table rows (`PLANS.md` 2.2) within 3 s/km; the race table at 50
    within 10 s; Riegel within 1% at 10 km and the half for every 5K from 18:00 to 36:00;
    ordered and monotonic for every second from 12:00 to 59:59; the 22:00 example exactly.
  - Placement: all 392 choices of days: 196 with two quality days, 196 with one (7 of those
    the day before the long run); never quality the day after the long run; never two hard
    days running.
  - **The builder: 49,104 programs** (every session of every plan, the trial and recent weeks
    too, at every level, for every 5K from 15:00 to 45:00 by the minute, with each
    combination of easy-by-heart-rate and reps-by-distance). Each is checked against the FIT
    reader's validity rules and the caps, then **run to the end through the SDK's own
    `WorkoutEngine`** by a simulated runner; the steps it ran match `totals()`, and where no
    step is open the time matches the estimate within 2%.
  - **FIT round trip:** 4,752 programs (the 132 sessions at three 5K times, three levels and the four option combinations) written with the SDK's `FitWorkoutWriter` and read
    back with `FitWorkoutReader`: the same program, nothing degraded.
  - The state file: round trip, every prefix, every one-byte change from a set of nine
    characters, and nine bad files.
- **Watch compile check of the core** (Ubuntu's `arm-none-eabi-g++` 13.2, `-mcpu=cortex-m33
  -mfpu=fpv5-sp-d16 -mfloat-abi=hard`, `-Wall -Wextra -Wpedantic -Wconversion
  -Wdouble-promotion`, no exceptions or RTTI): no warnings in our code. The core calls no
  `malloc`, `new` or exception code; its only maths are `expf`, `sqrtf` and `lroundf`. About
  11 KB of code at `-Os`. The header-only parts compile on their own, as the GUI will use
  them. A compile check only: CI builds the watch app from TP2.
  - **Found by it:** `Date::weekday` had two overloads (a day number and a date). On the
    watch `uint32_t` is `unsigned long`, so a literal date matched neither exactly and did not
    compile; on the host it did. The day-number one is now `weekdayOfDay`.
- **CI:** a "HybridX Training host tests" step in the `host-tests` job (`ctest`, so the
  generated header is checked too). Not yet run: it runs on the push.

### The gate (TP1): what Jon reads

The nine golden files in `Tests/Host/golden/`: each plan for the three gate runners (5K
20:00 at level 3 on Tuesday, Wednesday, Thursday, Saturday and Sunday; 25:00 at level 2 on
Tuesday, Wednesday, Friday and Sunday; 32:00 at level 1 on Tuesday, Thursday and Sunday; long
runs on Sunday). Each has the paces, every week day by day with the steps and targets the
watch would set, and a summary. Correct the `.plan` files (notation: `Tools/Plans/README.md`),
or say what to change and I will.

What I checked when reading them as a coach:
- the level-1 half plan reaches 130 minutes (about 16 km at 7:45-8:40) because of the
  distance floor; without it the peak would have been 105 minutes, 13 km;
- level 3's five-day weeks keep quality on Tuesday and Thursday with an easy Wednesday
  between, and reach 60 minutes of quality in a 66 km week at the peak of the 10K plan;
- the race week counts the race as quality (the summaries' large last number).

### Not done, and carried forward

- Missed sessions and weeks, moves and pauses (`PLANS.md` 4.3), and recalibration (2.6): TP5.
- The glance's file: TP6.
- The book's table rows for the pace fixtures (TP0's open item).
- `SessionBuilder` keeps about 1.2 KB of running totals on the stack while it builds. Fine on
  the host; TP2 checks it against the service task's stack, or makes it a member.

## TP2: the app (next)

The order (brief section 11 and `REVIEW.md` E1):
1. Copy `hybridx-run/Software` verbatim, in its own commit.
2. Rename: folder `HybridXTraining`, name "X Training", development APP_ID `6E050D707D4D2D32`,
   tags `training-v*`, the simulator's app ID; CI builds it into `watch-apps`.
3. Strip what stays X Run's: the VO2max history, re-scoring and Trail ingest, the VO2 screens
   and profile, the bests page, the VO2max glance. Keep the window builder for the Run link
   (TP4).
4. The core into `libs.cmake`; the config fields (brief 5.1) in the manifest and a field table;
   `plan.json` read and written with `SafeFile`.
5. Today's session built into the service's `mProgram` and started through `startWorkout`'s
   three calls; the alert modes applied to `ZoneAlerts`; the result recorded at save.
6. `docs/experiments/sim_plan.sh` (a session end to end in the simulator) and `fit_check.sh`.
