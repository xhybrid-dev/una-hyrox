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

## TP1: the plan for the pure core (not started)

To be confirmed in plan mode when TP1 starts. No app, no SDK service code, no screens.

**Order of work**

1. `Tests/Host` scaffold, copied from X Run's: the SDK's test framework, the Workout and FIT
   workout sources (for the round trip), the JSON reader and writer.
2. `PaceModel`: fitness from a race; paces; bands; predictions; the half's gap. Integer
   mm/s and ms out; floats inside (single precision; `exp` and `sqrt` only when a result
   changes, never per second). Tests: the secondary table rows (`PLANS.md` §2.2), the race
   table (RACE_PREDICTIONS §4.3), Riegel within 1%, monotonic in the 5K time, 5K from 12:00
   to 59:59 never produces a zero or overflowing pace.
3. The plan files and `gen_plans.py`: `Tools/Plans/10k.plan`, `half.plan`, `vo2max.plan`,
   one line per slot in the notation of `PLANS.md`. The generator refuses unknown codes, a
   week without a long slot, and two quality sessions beside a hard long run. Output:
   `Core/Header/data/PlanData.hpp`.
4. `WeekLayout`: `PLANS.md` §4.2, with all 392 combinations as a test.
5. `PlanCalendar`: dates, the start rule, counting back from a race, truncation.
6. `PlanBuilder`: recipe, paces and level into `SDK::Workout::Program`, raw fields included,
   the caps of `PLANS.md` §3.3, reps by time or by distance. Tests: the reader's validity
   rules; the caps; a FIT write and read gives the same program; totals match the recipe.
7. `PlanState`: the log and its JSON, bounded. (The rules for missed weeks are TP5.)
8. `PlanText`: names, purposes, the Today lines; ASCII only.
9. `Tools/plan_print`: a whole plan for any input as Markdown. Golden outputs in
   `Tests/Host/golden/` for the three gate runners in each plan (nine files), compared by a
   test, so a content change is a visible diff.

**The gate:** Jon reads the nine golden files and corrects the `.plan` files.

**Defaults assumed** (brief §14, "Proposed defaults"): time-dosed sessions, the level caps,
seven run-day switches, weeks from Monday, no fitness number shown.
