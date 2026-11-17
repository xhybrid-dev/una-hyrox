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
| APP_ID | `C790FB2FDF322688` (portal-issued, 16 November 2026). The development one, `6E050D707D4D2D32` (first 16 hex of md5 of `HybridXTraining`), is kept only for the packager to refuse |
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

## TP2: the app (15 November 2026)

### What was done, in four commits

1. **X Run copied verbatim** (`bbac03d`): `hybridx-run/Software` at `c9b6618`, its icons and
   manifest. X Run's `Libs/Core` is `Libs/RunCore` (X Training's own core already had the
   name); the VO2max glance was left out.
2. **Renamed** (`01cd895`): `Apps/HybridXTraining-CMake`, app folder `HybridXTraining`, "X
   Training" on the watch, development APP_ID `6E050D707D4D2D32`, `training-v*` tags,
   `HybridXTrainingSimulator`; `libs.cmake` builds X Training's core with RunCore, App and Link.
3. **X Run's own parts removed, the plan wired in** (this commit):
   - **Out of the service:** the VO2max history, its tuning log, re-scoring and window files,
     the Trail ingest, the public `vo2max.json`, the personal bests and `bests.json`, and the
     daily resting-HR sensor. **Out of the FIT file:** X Run's `vo2max` and `vo2max_recent`
     developer fields and its `max_met_data` (the UNA app makes a card of every developer
     field: X Sauna's finding). **Out of the GUI:** the VO2max main-menu item and the
     summary's VO2max page. X Run's VO2max and profile screens are still compiled but
     unreachable; TP3 deletes them with the screen rework. RunCore's files stay for now
     (`--gc-sections` drops what nothing calls).
   - **No shared file is written.** The activity link would write X Run's `act_run.json`,
     which breaks the link's one-writer rule, so `closeActivity()` no longer calls
     `writeActivityLink()` until X Training has its own slot (TP4). The Streak line is read
     as before.
   - **Phone settings:** the 15 fields of brief 5.1 (`AppConfigFields.cpp`,
     `Resources/app-manifest.json`; the SDK's `validate_app_config.py` accepts both and the
     patterns). The watch reads the strings again strictly (`TimeText`).
   - **The plan in the service:** `loadConfig()` reads the settings and names any problem
     ("Set your 5K in the UNA app", "Choose 3 to 6 run days", ...); `loadPlan()` reads
     `plan.json` (the family's `SafeFile`) and reconciles it: a different plan, level or race
     date starts a new plan from today (TP3 will ask first); new run days, a new 5K or half
     (dated today) and the options carry over. The half is ignored after 183 days.
     `updateToday()` works out today's session at every GUI start, and `startPlannedSession()`
     builds it into the service's `mProgram` and starts it with the three calls
     `startWorkout()` uses. At save, `recordPlannedRun()` logs it done or part done
     (`judgeSession()`, host-tested: at least half the planned time, so skipping through the
     steps with the lap button is not a session done).
   - **Alerts:** a planned session's easy steps only say "too fast", short reps and strides
     never alert live, and a held-back "too slow" has no "back in" (`zoneAlertAllowed()`).
     UNA's own workouts from the library alert as before.
   - **The GUI, minimally (TP3 is the real screens):** the service sends `PLAN_TODAY`
     (0x20, a fixed-size `PlanTodayInfo`), the model arms today's session as a workout
     (`kPlanSession`, -2), Start shows it ("Intervals 5 x 3 min"), and the countdown shows
     its name, what and target. The Workouts list's "Free run" row disarms it. The main
     screen's title is "TRAINING".
4. CI: the app in `watch-apps`, its settings declaration checked, X Training's copy of the
   link library in `check-link-copies.sh`.

### Verified

- **Host tests: 60** (one new: `judgeSession`), green.
- **Simulator** (`docs/experiments/sim_plan.sh`, with `fit_plan_report.py`): a pretend watch
  on which today is the first quality day of week 5 of the 10K plan.
  - The service read the settings and the plan, worked out the paces (easy 5:27-6:08, half
    4:48, threshold 4:40, 10K 4:34, interval 4:18, repetition 4:03 /km) and today:
    "Intervals 5 x 3 min, 4:12-4:24 /km, week 5 of 10".
  - Start showed "Intervals 5 x 3 min"; the countdown its name, "5 x 3 min" and the band;
    the first step card "WARM UP, 15 min, 5:27-6:08 /km"; the gauge face the live pace
    against the band and the time left.
  - Stepped through with R2, the workout completed; saved.
  - **The FIT file:** the workout "Intervals 5 x 3 min" with its five steps and their
    targets (warm-up 5:27-6:08, intervals 4:12-4:24 as speed bands, open jogs, the repeat,
    cool-down 5:27-6:08), twelve laps each linked to its step (warm-up, five intervals and
    jogs, cool-down) and the session-end lap; a running session; developer fields
    `batteryLevel`, `battVoltage`, `hr_source`, `hr_optical`, `hr_external`,
    `distance_estimate` and no VO2max.
  - `plan.json`: before `judgeSession`, the run was logged done (`[20261115,7,1,...]`); with
    it, a 72-second skip-through is "too short to count: 72 of 2925 s" and nothing is
    logged. No file appeared in `SharedData`.
- **Watch build, local compile check** (Ubuntu's `arm-none-eabi-gcc` with Race's syscall
  stubs: **not for a watch**): `HybridXTraining_0.0.0-dev.uapp`, 513,736 bytes, no warnings in
  our code.
- **CI, run 295 on `76ed3f2`: green.** ST's toolchain built `HybridXTraining_*.uapp` into
  `watch-apps` with every other app; the host-test job passed X Training's settings
  declaration, its host tests (with the plan-header check) and the link-copy check (now seven
  apps). TP1's run (294 on `c9b6618`) was green too.
- **Not verified:** anything on a watch; a whole session at real speed (49 minutes); the
  "done" path with the new rule end to end (the host test covers the rule); imperial units;
  a GUI started on a day after the service started (it re-works today at every GUI start).

### Found along the way

- A simulator run of the unmodified copy wrote X Run's `vo2max.json` into
  `Software/SharedData`, and the folder was not git-ignored, so it was briefly committed
  (amended out before the push). Exactly why the strip had to come before any watch install.
- The "Workout completed" screen takes the first button press for 5 s, so a script must wait
  before opening the action menu.

### The gate (TP2): what Jon tests on the watch

The `.uapp` comes from CI once this push's run is green: **Actions → Watch builds → the run
→ Artifacts → watch-apps → `HybridXTraining_*.uapp`**.

1. **Install:** connect the watch by USB, make the folder `Apps/HybridXTraining/`, copy the
   `.uapp` into it, eject safely, power-cycle. "X Training" is in the app list.
2. **Your plan, by USB** (until the app is on the store the phone cannot set it): open the
   app once, then copy this as `Apps/HybridXTraining/app_config.json`, with your own 5K, and
   your days (`true` for a day you can run):

   ```
   {"schema":1,"values":{"plan":1,"fiveK":"22:00","fiveKRecent":true,"halfTime":"",
   "raceDate":"","level":2,"runMon":false,"runTue":true,"runWed":true,"runThu":false,
   "runFri":true,"runSat":false,"runSun":true,"longRunDay":7,"easyByHr":false}}
   ```

   A new plan starts this week if set up Monday to Wednesday, else next Monday, so for a
   session today pick a race date or days that make today a run day of week 1 (`plan_print`
   shows which).
3. **Open the app:** Start should show today's session ("Easy run, strides 40 min", ...) or
   be a plain Start on a rest day.
4. **Run it:** Start, the countdown, then the steps with their targets; the easy steps should
   only ever buzz "too fast".
5. **Afterwards:** the activity in the UNA app and Strava (the workout's steps with their
   pace targets, a lap per step); `Apps/HybridXTraining/plan.json` with today logged
   (`...,1,` for done) if you ran at least half of it. Send me the `.fit` and `plan.json`.

## Coach review feedback (15 November 2026)

Jon passed on a second coach's review of `PLANS.md` (refinements, edge cases, answers to the open
questions, two watch-UX checks). Each point was tested against the plan data and the code before
anything changed. The result:

| Point | Done |
|---|---|
| VO2max week 1: 30-30s two days before the time trial | **Applied.** Week 1's Friday is `E+S 35`, so the baseline is run fresh. With a recent 5K, week 1's Friday is `HILLS 6 x 45 s` in place of the 30-30s (the first 30-30s are in week 2) |
| VO2max week 7: 30 minutes of interval-pace work before the retest | **Applied.** Friday is `30/30 2 x 8` (was `3 x 8`): the 25:00 runner's week 7 goes from 4:03 and 38.4 km to 3:52 and 36.6 km |
| "E+S ends right after a hard stride, with no cool-down" | **Not applied: it does not.** The builder's strides block is `6 x [Stride 20 s, Easy 1 min]`, so the last stride is followed by a minute's easy running, and a finished workout carries on as a free run until the runner saves |
| Three run days: the long run is 50-55% of the week | Nothing to permit: **no share-of-week check exists,** only the level's range and 150 minutes. Measured on the nine golden plans, the long run's biggest share is 29-36% (5 days, level 3), 33-42% (4 days, level 2) and 45-60% (3 days, level 1; the 60% is the half's week 10). **Applied:** with three run days a long run's fast finish is at most 20 minutes (`kThreeDayFinishMaxMin`; the half plan's `HM 30` becomes 20) |
| 32:00+ 5K: threshold meets 10K pace, so cap the 10K at the threshold | **Not applied: the premise is backwards.** Threshold is roughly one-hour race pace, so a 10K that takes over an hour *should* be slower than threshold; `min(10K pace, T)` would make the 10K pace faster than the model says. The crossover is at about 32:00 (6:40 both), not 33:30, and at 36:00 threshold is 7:27 and 10K 7:30. The note on the Paces screen the reviewer asks for is already planned (PLANS 2.3); it goes in with TP3 |
| Reps by distance: not for 30-30s, hills or tempo | **Applied.** Tempo was the one timed session that went by distance with the option on; it is always timed now. 30-30s, hills and strides always were |
| Half's gap: +10% long runs (to 135 min) when over 4% | **Not applied. Jon, 15 November 2026: words only;** the long runs stay as they are |
| Race day: alerts opt-in, or "too fast" for the first 2 km | **Agreed, TP3.** The race step alerts both ways today, where the draft says "alerts as in Settings", and there is no Settings screen yet. TP3 adds "Race alerts: Off / First 2 km, too fast / Both", default Off |
| After a plan: a recovery week at about 60% volume, then the choice | **Agreed, TP5** (plan memory) |
| Keep the HybridX coaching platform's labels | **Jon, 15 November 2026: the names stand** (Easy, Threshold, Interval, Repetition, Cruise, Strides, Tempo, Hills) |
| Lap-press protection on the warm-up before TT5, TT10 and RACE | **TP3.** R2 on that warm-up needs a hold of about 1 s, with a "Hold to start" hint. The GUI already receives the next step (`WorkoutStepInfo`) |
| Distinct "too fast" and "too slow" haptics | **Already so:** `Service::playZoneAlert()` (UNA's) plays too fast as a double click with short rising notes, too slow as a 750 ms alert with long falling notes, and back in as a strong click with a rising triad |

**Not enforced, though the review assumed it:** the "about four fifths easy" principle. It is by
design, not by a check. Time on steps faster than easy is 10-22% of the time in the nine plans
(the half's 25:00 runner 22%, race week included). A host test could pin an upper bound; say if
you want one.

Touched: `vo2max.plan` (and its generated header), `PlanBuilder` (`Scaling::runDays`,
tempo always timed), `PlanSchedule` and `plan_print` (pass the run-day count), `PLANS.md`, two
golden files and the review PDF. Two new tests; 62 in all.

## The TP1 gate: passed (15 November 2026)

Jon read the plans (`PLANS_REVIEW.pdf`), passed on a second coach's review, and accepted the
result: "those are ok with the long run and sessions". The plan content stands as committed in
`7d7c3b0`, with the two decisions above (no long-run boost for the half's gap; the session names
stay). The content can still change at any time: it is text in `Tools/Plans/`.

Still open from TP2's gate: Jon's first session on the watch (the steps are under "The gate
(TP2)").

## TP3: the screens (built, 15 November 2026; at its gate)

Today, This week, Plan, Paces and New plan; the Saved screen's lines; the short-rep result
on the step card; "reps by distance" and the other watch settings; the text-fit audit and
`capture_screens.sh`; X Run's unreachable VO2max screens deleted. Five steps, each committed
and pushed so CI makes a `.uapp`: (1) the main menu and Today; (2) This week, Plan, Paces;
(3) Settings and New plan; (4) during and after the run; (5) the audit, captures, clean-up.

### Jon's first install (15 November 2026): the plan was made, but nothing showed it

Jon installed the TP2 build and copied in `app_config.json` (the 10K plan, level 2, 5K 22:00,
Tue, Wed, Fri and Sun, long run Sunday). The app "doesn't load a training session, only asks
to load in a fit file on the Workouts menu". His `plan.json` shows the service did its part on
the watch: `"s":[1,2,86,6,0,0,0,1320,20261115,1,0,0]` is that set-up, dated that Sunday, and
`"c":[0,20261116,0,10]` a calendar of 10 weeks from Monday 16 November (set up on a Sunday, a
plan starts the next Monday). Monday is not one of his days, so his first session is Tuesday
17 November, "Easy run, strides, 40 min" (`plan_print`). TP2's main menu spoke of the plan only
on a day with a session (Start's hint); on any other day Start was plain, and the only
workout-like item, Workouts, is UNA's library of FIT workouts, empty on his watch. So the
watch test passed its first half (settings read, plan made and saved, on the watch) and showed
the gap TP3 fills first.

### Step 1: the main menu and Today

- **Main menu** (brief 8.1): Today, Free run, Treadmill, Workouts, Settings. Today's hint always
  says what today is: "Intervals 5 x 3 min", "Done: Intervals", "Rest day", "Starts Mon 16 Nov",
  "Plan complete", "Check your plan" or "No plan". Free run is X Run's Start (it reads "Start",
  with the workout's name, when a UNA workout is armed from Workouts). RunLVGL's Intervals is off
  the menu (decision 10); its screens go in step 5.
- **Today** (`WorkoutDetailsScreen`, `Source::Today`): on a day with a session, the week in the
  title ("WEEK 5 OF 10"), then the name, what it is for, what ("5 x 3 min"), the target and the
  estimate ("9.1 km, 49 min"); L2 scrolls the steps, drawn as for a UNA workout from the
  service's program; R1 starts it (the countdown, or the no-GPS question first). On any other
  day the state and the next session ("Next: Tue 17 Nov / Cruise intervals / 3 x 6 min"). A
  planned session is always outdoors: the treadmill switch applies to free runs only (decision 9). *(Reversed on
  16 November 2026: see "Treadmill sessions".)*
- **The service:** `PlanTodayInfo` gains today's date and the next session (`nextSession()`,
  from tomorrow, or from the plan's first day before it starts); the details request takes
  `kPlanDay` and a date (`buildPlanDay()`), so the step list is the program Start would run.
- **Fitting the round face:** a `TextStack` lays the lines out as a block and fits each to the
  width that keeps it clear of the buttons' arcs (radius 108), in a smaller face if need be. The
  first Today page put the purpose at the foot, where the face is narrow, and cut it; it now sits
  under the name. Three purpose lines were too long for one line anywhere and are shorter
  (`PlanText`, `PLANS.md` 3.1): "Easy enough to talk", "Easy, strides for form", "Endurance, then
  race pace".
- **Checked:** `capture_screens.sh` (seven pretend watches: before the start, a rest day, a
  session day with its steps and countdown, done, finished, no plan, a problem) into
  `docs/screens/tp3-*.png`, each looked at; `sim_plan.sh` (now Today, R1, R1) runs a session
  and saves the FIT file as before; 63 host tests; the watch target compiles.

### Step 2: This week, Plan, Paces

- **Main menu:** Today, Free run, Treadmill, This week, Plan, Paces, Workouts, Settings.
- **This week** (`WeekScreen`): the week's title ("WEEK 5 OF 10"; the first week before the plan
  starts, the last once it is over), seven rows from Monday, the day and the session's short
  name coloured by its mark: green done, amber part done, cyan today, white to come, grey not
  logged, dark grey rest. The chosen row's state in words above ("Today, key", "Part done",
  "Not logged"), so the colours are never the only sign. R1 opens the day: the same page as
  Today (`WorkoutDetailsScreen`, `Source::Day`, titled with the date), its steps a scroll away,
  R1 only on today's.
- **Plan** (`PlanScreen`): the plan in the title ("10K PLAN"), the week, its phase, level and
  days; a bar of the weeks coloured by phase (bright for the weeks run, dark for those to come,
  on the display's colour grid: LVGL's opacity blends off it and showed black and dark red);
  this week underlined; "2 of 4 this week", "2 of 19 so far", "Ends 20 Dec" (or "Race 24 Jan").
- **Paces** (`PacesScreen`), four pages: the paces (/km or /mi); the times they come from, each
  dated; the predicted 10K, half and marathon; and, when there is one, a note: "Threshold is
  about your 10K pace" (within 5 s/km, PLANS 2.3), "Your half matches a 34:47 5K: long runs
  matter most for you" (2.4), or that a half over six months old is not used.
- **Not missed, but not logged:** TP3 marks a past session with nothing logged "Not logged". A
  plan set up on a Wednesday starts that Monday, so its Monday and Tuesday would read "missed"
  though they were never asked for; telling them apart needs the set-up day, which `plan.json`
  does not keep. The missed rules and that day are TP5's (brief section 9).
- **The core:** `PlanProgress` (each day's mark, the week to show, the counts; three host tests),
  `DayMark` in `PlanTypes.hpp` for the GUI, `PlanText::phaseName`. The service's `dayInfo()` is
  Today's words for any day; `PLAN_REQUEST` asks for a day, the week, the overview or the paces,
  answered by `PLAN_DAY`, `PLAN_WEEK`, `PLAN_OVERVIEW` and `PLAN_PACES` (each under 256 bytes,
  asserted).
- **Checked:** `capture_screens.sh` gains a week with two days logged (This week, a day and its
  steps, Plan, the Paces pages) and a 32:00 runner with a slow half (the note); 32 screens, each
  looked at; 66 host tests; the watch target links.

### Step 3: Settings and New plan

- **Settings** gains three items after Lap Alerts: **Reps by distance** (a watch setting in
  `settings.json`; `plan.json` keeps a copy, and Today's estimates follow it), **Easy by heart
  rate** (the phone's field: the switch writes it back with `AppConfig::setBool` and `save()`, as
  Race does, so the phone shows what the watch uses) and **Race alerts**: Off (the default), First
  2 km, too fast, or Both ways (`settings.json`; the race step obeys it in step 4).
- **New plan** (Plan, R1; brief 5.2): a wheel of the choices with their values as hints: the
  plan (R1 turns 10K, half, VO2max), your 5K (minutes then seconds, 12:00 to 59:59), run in the
  last 6 weeks (a switch), the level (R1 turns 1 to 3, "2: 20 to 40 km/week"), the run days (seven
  switches; the long run moves to a run day if its day is switched off), the long run (R1 turns
  through the run days), the race (None, or a Saturday or Sunday 3 to 26 weeks on), then Next. The
  draft starts from the phone's set-up. Next shows the plan, level and 5K, the days, the long run
  and, from the service's `PlanCalendar`, "From Mon 16 Nov, 12 weeks" (or "8 of its 12 weeks", or
  why it cannot be: "The race is too close or past"); R1 starts it. Starting writes the plan's
  phone fields to `app_config.json` (not the half, nor easy by heart rate) and makes a new plan
  even when the plan, level and race are the ones already running (a restart).
- **A new plan from the phone now asks** (brief 5.1): when the phone's plan, level or race date
  would replace a plan under way, the service keeps the plan under way and Today says "New plan
  from your phone / Half marathon, level 2 / Start it?" (the main menu's hint: "New plan from
  phone"). R1 offers Start it or Keep mine. Keep mine writes the plan under way back to the
  phone's values, so they agree again. A first plan, or the phone ending a plan (plan none),
  still applies without asking. While the question waits, today's session is not offered.
- **Messages:** `PlanRequest::Setup` → `PLAN_SETUP` (the phone's set-up, the plan under way, and
  whether the phone asks); `NEW_PLAN` (Preview → `PLAN_PREVIEW`, Apply, KeepCurrent);
  `PlanTodayInfo::PhoneAsks`.
- **Checked** (`capture_screens.sh`, four more pretend watches, 57 screens, each looked at):
  - settings: reps by distance and race alerts saved in `settings.json`, and reps by distance
    copied to `plan.json`;
  - a New plan made on the watch (the half, level 3, 5K 21:02, a race on Sun 7 Feb): the phone's
    values written (`"plan":2`, `"fiveK":"21:02"`, `"raceDate":"2027-02-07"`, `"level":3`) and a
    12-week plan from Mon 16 Nov;
  - the phone's ask answered Start it (the half from Mon 16 Nov) and, on another pretend watch,
    Keep mine (`app_config.json` back to `"plan":1`, the 10K under way unchanged).

### Step 4: during and after the run

- **A short rep judged on the next card** (brief 8.2): a step whose alerts wait for its end
  (`AlertMode::AtEnd`: strides, repetitions, the 30-30s' fast halves) is judged from its lap when
  it ends (`RepJudge.hpp`, header-only, three host tests). A distance rep that ran its distance is
  judged by its time ("Last rep 1:36, aim 1:34-1:40"); a timed rep, or one ended by the lap button,
  by its pace ("Last rep 3:18 /km, aim 3:55-4:11"; per mile in imperial). The next step's card
  shows the aim in place of "Rep 2/6" and the result under it, in the gauge's colours (red too
  fast, green in the band, amber too slow); nothing under 20 m (no GPS).
- **R2 held on the warm-up before a time trial or race** (the coach review): the planned session's
  open warm-up before TT5, TT10 or RACE says "Hold R2 to start"; a tap does nothing; a hold of one
  second (a bar fills under the hint) starts the effort. The hold is Knitting's `HoldTimer`
  (copied with its 19 host tests): the kernel sends press, then a click only if the button comes up
  within 500 ms, then release, so that press's own click and release are swallowed after the hold
  fires, and the next step is not skipped. The service flags the step (`WorkoutStep::holdToStart`);
  a suspend drops a hold in progress.
- **Race alerts** (step 3's setting) on the race step: `raceAlertMode()` (pure, a host test): Off
  silences it, First 2 km lets "too fast" only in the step's first 2 km, Both keeps the plan's own.
- **The Saved screen's lines** (brief 8.3): after a planned session is saved, `PLAN_SAVED` says how
  it counts ("Done" green, "Part done" amber, "Not counted" grey, in place of the tick), the week
  ("Week 3: 1 of 4") and the next session ("Next: Tue, Easy"). They show for 3.5 s; a Streak line
  (and the load) follows in their place.
- **Today's target:** a time trial reads "flat out" and hills "strong, uphill" (they have no band;
  the page had shown the easy range). The easy range is now the range itself: "easy 5:27-6:08 /km"
  had read 6:07, a band round its middle rounding one end.
- **Checked:** `capture_screens.sh` gains a repetitions session (a 400 m rep ended by hand after
  about 15 s: "Last rep 3:18 /km", red, "aim 3:55-4:11") and a 5K time trial (a tap leaves the
  warm-up running and one session started; the hold starts the 5 km), both saved ("Not counted":
  under half the session in the simulator); `sim_plan.sh` still runs and saves its session with the
  workout and a lap per step; 89 host tests; the watch target links.

The coach review's three TP3 items are in: the Race alerts setting (step 3), R2 held before a
time trial or race (step 4), and the Paces note where threshold meets 10K pace (step 2).

### Step 5: the text-fit audit, the captures, the clean-up

- **The text-fit audit** (`docs/experiments/audit.sh`, about 30 seconds; HybridX Workout's
  `Audit.hpp`, `Audit.cpp` and a driver of our own, `AuditDriver.cpp`, simulator only). It walks
  the plan's screens with the widest text each can be given and measures every label of ours with
  LVGL on the real font: inside its box, clear of the bezel arcs (radius 109), and, new here, not
  cut with an ellipsis (the screens cut rather than overflow, which a box test alone would pass).
  - What it gives each screen:
    - Today, every kind of session with the widest "what" the three plans make at any level and run
      days, worked out from the plan data by the core itself, and paces as wide as a 59:59 5K's per
      mile;
    - Today's other states and the longest problems;
    - a day of the week; This week, every row the widest short name;
    - Plan at 24 weeks and 144 sessions;
    - the four Paces pages and both notes;
    - the New plan summary and its problem;
    - the step card's rep result;
    - the Saved lines.
  - Its first run found 21 problems, all fixed:
    - "Long run, fast finish" and "100 min, last 20 at half pace" now go over two lines at their
      commas on Today and a day's page, and the purpose, then the estimate, are left out when only
      that makes the page fit whole. The Next lines take the "what" up to its comma.
    - The shake-out's purpose is now "Ready for race day" (`PlanText`, `PLANS.md` 3.1).
    - Plan shows the phase alone (the level and days did not fit over the bar) and "Starts 30 Sep".
    - New plan's problems are two shorter ones ("Choose 3 to 6 run days", "Long run on a run
      day"), and a long one wraps.
    - The step card puts the aim on the wider line above the result.
    - The Saved lines sit higher, in the smaller face; "Less than half run".
    - The main menu's hint uses the name alone, or the short name, when "name what" is too wide.
  - Now: 0 problems. The wheel menus are the SDK's widget and are not measured; their hints are
    fitted to 180 px.
- **Deleted, now unreachable:** X Run's VO2max screens (`Vo2Screen`, `MenuVo2ProfileScreen`,
  `Vo2PickerScreen`) and RunLVGL's Intervals set-up screens (`MenuIntervals*`,
  `IntervalsPickerScreen`), with their screen IDs. The countdown's R2 now goes back to Today for a
  planned session. The track screens that workouts share stay. The watch build is 14 KB smaller.
- **Captures:** `capture_screens.sh` makes 15 pretend watches and about 75 screens into
  `docs/screens/tp3-*.png`:
  - before the start, a rest day, a session day with its steps, done, the plan over, no plan, a
    problem;
  - a week with days logged, with This week, a day, Plan and Paces;
  - a slow runner's notes;
  - the settings, New plan, the phone's ask answered both ways;
  - a repetitions session and a time trial, each saved.

  Each one has been looked at.

## Daniels' book (15 November 2026)

Jon supplied *Daniels' Running Formula*, 4th edition (Human Kinetics, 2022), as a PDF with a text
layer; its tables are images, read at 400 dpi. What version 2 had from secondary sources was
checked against it, and the model, the limits and the plans brought to it.

**Read:**
- chapter 4 and figure 4.1: each type of training's intensity, length, work to rest and share of
  the week;
- chapter 5: Tables 5.1 (race times by VDOT 30-85), 5.2 (training paces, VDOT 30-85) and 5.3
  (novices, VDOT 20-29), the "6-second rule";
- chapter 8: the white, red and blue plans, as a check on the limits at 3-5 days a week.

**The tables, all of them, in the host tests.** All 66 rows are in
`Tests/Host/fixtures/daniels_vdot.csv`, read by hand: 5K, 10K, half and marathon; E, M, T and I per
km; I and R per 400 m or 200 m. `DanielsTest.cpp` feeds each row's 5K to the model and compares.

**What the comparison showed, and what changed:**

| | Version 2 against the book | Now |
|---|---|---|
| Threshold (88%) | Within 1 s/km, VDOT 20-85 | Unchanged |
| Race predictions | 10K within 2 s, half within 9 s, marathon within 24 s (VDOT 30-85) | Unchanged |
| Easy (62.5-72.5%) | Fast end 2-3 s/km slow throughout; slow end up to 6 s slow (VDOT 30-38) | **63-73%**: within 3 s/km |
| Repetition (I less 15 s/km) | 4-9 s/km too slow for VDOT 30-48. Daniels: "a 7- or 8-second rule for runners in the 50 or 40 VDOT categories" | **I less 20 s/km at fitness 47 and below, 15 from 50**: within 4 s/km |
| Interval (97.5%), slow runners | 6-20 s/km too fast below VDOT 30: Table 5.3 keeps I about 20 s/km under T | **Eases to T less 20 s/km below fitness 31, all of it at 28**: within 2 s/km |
| Marathon pace | The predicted race's; Daniels' M training pace is slower below VDOT 45 (up to 18 s/km at 20) | Unchanged: the plans never use M; the test checks M from VDOT 45 |
| Interval jogs (60% of the rep) | Daniels: 1:1; his examples 3 min with 2 min jog, 4 with 3, 5 with 4 | **Three quarters of the rep** (3 min: 2:15) |
| Interval caps 12/18/24 min | Above 8% of the levels' weeks (level 2's 18 min is about 11% of 35 km); his red plan (4 days) has 15-16 | **10/16/20 min** |
| Repetition cap, level 2, 2.4 km | 7% of its week; he says 5% | **2.0 km** (`REPS 5 x 400 m`) |
| Tempo, level 3, 25 min | He says "20 min max" for a steady T run | **20 min** |
| Cruise, level 3, 40 min | His maximum broken into cruise intervals is 30 | **30 min** |
| Easy runs, level 1, from 20 min | "30 minutes be the minimum duration of any E run" | **30-35 min**; the VO2max block's `E 25` is `E 30` |
| Strides, cruise jogs, R recoveries, long run 150 min, I bouts to 5 min, R to 2 | As the book | Unchanged |

Two secondary figures were simply wrong: VDOT 40's easy range is 5:56-6:38 (not 5:59-6:44) and its
repetition pace 104 s per 400 m (not 108).

**Plan files changed** (so that level 2 as written is level 2 as run): the 10K's `REPS 6x400m` →
`5x400m` and week 7's `INT 6x3` → `5x3`; the half's week 7 `INT 6x3` → `5x3`; the VO2max block's
weeks 5 and 7 `INT 6x3` → `5x3` (the peak keeps its 90 s jogs, so it still progresses in density),
and its retest week's `E 25` → `E 30`. The golden plans and `PLANS_REVIEW.pdf` are regenerated;
92 host tests pass.

**For a 22:00 runner like Jon** (10K, level 2): easy 5:26-6:06 (was 5:27-6:08), repetition and the
strides' target 3:58 (was 4:03); threshold, interval, 10K and half paces unchanged. The first week
is unchanged.

**Left as it is, and why:**
- **The long run's share of the week.** The book confirms 30% under 40 miles a week; with 3-4 runs
  ours are 33-60% (PLANS 3.3). Jon kept the long runs (15 November); his red plan has a 40-50
  minute long run in a week of about two and three quarter hours, between a quarter and 30%. Still Jon's call.
- **The 30-30s** follow Billat (2001), not Daniels, whose short I bouts have recoveries shorter than
  the work; the hills are our own. Neither is in the book's tables.
- **M pace** for runners under VDOT 45, above.

## The gate (TP3): what Jon checks

1. **The captures:** `docs/screens/tp3-*.png` (or the contact sheet sent with this). Say what
   reads wrongly, what is missing, and any words you would change.
2. **A week of the plan on the watch**, from the CI build of the last push (Actions → Watch builds
   → watch-apps → `HybridXTraining_*.uapp`, into `Apps/HybridXTraining/`, keeping your
   `app_config.json` and `plan.json`). Each day:
   - Today: the session (or rest, with the next), and R1 starts it;
   - This week and Plan;
   - after a run, the Saved lines.

   Your first week (10K, level 2):
   - Tue 17 Nov: Easy run, strides 40 min (each stride's result on the next card);
   - Wed: Easy 35 min;
   - Fri: Cruise intervals 3 x 6 min;
   - Sun: Long run 60 min.
3. **To decide:** whether R1 on Plan for New plan (an amber arc, no word) is clear enough, and the
   four shortened purpose lines.

Not in TP3, as the brief phases it:
- TP4: the Streak slot and the Run link.
- TP5: missed sessions (a past day now reads "Not logged"), moves and offers, and recalibration
  after a time trial.
- TP6: the glance.

## TP4: the family (built, 15 November 2026; at its gate)

Brief §10 and REVIEW E3. Four steps, three commits, each built for the simulator and the watch.

### Step 1: the `training` slot (`d12c2ac`)
- **The link:** `hybridx-link`'s original gains `Slot::Training` =
  `../SharedData/HybridX/act_training.json` (`kSlotCount` 6), synced into all seven apps.
- **Streak:** it opens every slot already, so its only change is the name on This week.
  "HybridXTraining" ran into the arcs, so it shows as "X Training" (Streak NOTES "X Training joins").

### Step 2: X Training writes its slot (`7e35f40`)
- **The link file:** `Service::writeActivityLink` writes `Slot::Training` as "HybridXTraining",
  sport 1, with the load. It is called after the `.fit` closes, before the Streak line, which so
  counts the run just saved.
- **Found:** TP2 had removed X Run's per-second feed with the VO2max, and the run statistics went
  with it. Since TP2 the zones, best splits and halves were empty and the load was 0.
  `feedSecond()` restores the feed.
- `sim_plan.sh` now checks the link file against the `.fit`: app, start, timer, sport, and the
  load worked out again from the records.

### Steps 3 and 4: the Run link and X Run's VO2max shown (`54a7fa1`)
- **The Run link (brief 10.2):**
  - `feedSecond()` also feeds X Run's window builder, which X Training already held as a copy;
    `Utilities/check-vo2-link-copies.sh` keeps that copy identical to X Run's, in CI.
  - On save, `saveVo2Windows()` writes the run's kept minutes to the oldest of
    `training_vo2_0..2.json`, named by the `.fit`'s start. A treadmill run writes none.
  - X Run reads them behind its own mark (`vo2.json` version 5; Run NOTES "X Training's runs").
- **X Run's VO2max shown (brief 10.3):**
  - `RunVo2Max.hpp` reads X Run's `vo2max.json`. Every value is bounded; a file that is missing,
    bad or of a newer version gives no figure.
  - `plan.json`'s optional `"vo2s"` keeps the first figure seen on or after the plan's start.
  - The Plan screen's second line under the bar reads "VO2max 45.2 in X Run", or "VO2max 45.2, was
    44.6" once the figure has moved since the start (in any plan). Its wording names X Run; it is
    never on the Paces screen and never used for a pace.

### Verified
- **Host tests:**
  - X Training 95 (`RunVo2MaxTest`, plan.json's `vo2s`);
  - X Run 204;
  - Streak 160;
  - the link 52;
  - Trail 149.
- **Builds:**
  - the simulators of all seven family apps;
  - the watch targets of X Training (540,748 bytes) and X Run (521,020);
  - CI runs 311 and 312, green.
- **`sim_plan.sh`:**
  - every link check passes (load 5 of 5);
  - with `STEP_SECS=50` (11 minutes) the session left `training_vo2_0.json`: 5 minutes past the
    warm-up, max HR 144, its id the `.fit`'s start;
  - the load was 30 of 30.
- **X Run's `sim_trail_ingest.sh`, starts 4 and 5:**
  - two X Training runs counted once, as source 2, in time order;
  - one of them is older than Trail's mark, and still counted.
- **The text-fit audit:** 0 problems, with the widest VO2max lines.
- **The captures:** `docs/screens/tp4-vo2-plan10k.png` and `tp4-vo2-planblock.png`, looked at.
- **The family check** (`hybridx-link/docs/experiments/family_check.sh`, quick mode, 16 November
  2026): all 39 checks pass.
  - **X Training:** its summary matches its `.fit` (400 s, load 18 of 18). Its Streak line is
    "1 of 7"; the next seven saves read "2 of 7" to "8 of 7", the seventh and eighth with
    "1-week streak".
  - **Streak:** it found 6 files, 8 entries and 8 new, counted X Training's run once as a Run, and
    on its second open found nothing new.
  - **X Run:** X Training left 1 window (max HR 143), which X Run read once: `vo2.json` v5, training
    mark the `.fit`'s start. One window is too few for an estimate in the simulator, as expected.
  - **Three earlier runs failed, all on the check, not the apps:**
    - two presses of L2 reached Summary, not Save & End, mid-session;
    - Streak's target of 8 is above its most, 7;
    - X Run had no resting heart rate, so it rightly left the run waiting.

    One more run crossed midnight into a new Streak week, and failed only the checks that assume
    one day.

### The gate (TP4): what Jon tests
`hybridx-link/docs/FAMILY_WATCH_TEST.md` section 7 (steps 19 to 24). In short:
1. Install the new **X Streak**, **X Run** and **X Training** from one Watch builds run.
2. Do a session of at least 20 minutes in X Training, then:
   - Streak counts it once, listed as "X Training";
   - X Run reads its minutes once;
   - X Training's Plan screen shows X Run's figure.

**Releases (Jon's tags, after the gate):**
- **Streak** first, or with X Training: an older Streak never counts X Training's runs.
- **X Run** with the reader. Its `vo2.json` becomes version 5, which an older X Run cannot read,
  so do not downgrade it.
- **The others** (Race, Trail, Workout, Ride) only add X Training to their Streak line. A store
  update for them is optional.

**For Jon to decide:** whether Streak's This week should show every app by its watch name (X Run,
X Ride, ...), as it now shows X Training; today the others show their folder names.

## TP5: the plan's memory and recalibration (built, 16 November 2026; at its gate)

PLANS.md 4.3 and 2.6, as written, in a pure core (`PlanRules.hpp`), then the service and screens.
Jon read nothing of this yet: the gate is below.

### What was built
- **The rules** (`Software/Libs/Core/PlanRules.*`, with `PlanCalendar`'s week adjustments and `PlanState`'s
  memory): the days gone are logged **Missed** when the app opens; a **missed key session** takes the
  place of the week's later quality; a missed easy run is dropped; at a week's end, **less than half
  done is repeated** (with a race date it takes the next week's place and never the last two weeks of
  a 10K or three of a half; without one the plan grows, twice at most) and a week with **nothing done**
  repeats the last cut-back week; **offers** (yesterday's missed long run, once, if today is easy or
  rest; a missed key session in place of today's easier one); **moves** (tomorrow's session today,
  today's to tomorrow, skip), hard days kept apart (the nearer becomes easy), a day moved stays moved;
  **pauses** (nothing missed; 8 to 14 days repeat the last completed week; over 14 days, easy running and
  a long-day time trial until a trial is run, then the plan resumes where it was paused);
  **recalibration** (a time trial or race gives a 5K equivalent: faster applies, slower by more than 2%
  asks, a jump of more than 6% asks).
- **`plan_sim`** (`Tools/plan_sim`, golden `Tests/Host/golden/plan_sim.md`): 19 scenarios as Markdown.
  **This is TP5's gate.**
- **plan.json** (optional keys, so a file from before TP5 reads as before; buffer 3584 bytes): `adj`,
  `ov`, `ps`, `rt`, `rw`, `st`, `ak`, `od`.
- **The service**: `settleToday()` at each Today; `PlanRules::sessionOn` for every session shown or
  started; a time trial's or race's effort that ran its whole distance gives its time (`mTrialSec`,
  from the step that ended on its own); `recordPlannedRun` applies, asks or keeps and writes the new 5K
  back as the phone's field (`writeFiveK`); a new message, `PLAN_ACTION` (0x2A), for the answers.
- **The screens:** Today says Paused, Skipped, an offer ("Long run missed, Do it today?") or a trial's
  question ("5K trial 22:50, Paces from 22:00, Update them?"); R1 there opens `PlanOfferScreen`
  (Yes or No, Update or Keep mine). A new main-menu item, **Changes** (`PlanChangesScreen`): Tomorrow's
  (do it today), Today's (do it tomorrow), Skip today, Pause or Resume, End plan (R1 twice). The result
  is in Today's title (CHANGED, NOT ALLOWED, NO SESSION). This week marks missed days (orange); Saved
  says what a trial did ("5K 21:41, new paces").

### Decisions made while building (each cheap to change)
1. **Missed key session, no later quality this week:** PLANS has nothing for it; the brief's offers
   list does ("Intervals were missed: do them in place of today's session?"), so it is offered on the
   next easier session day that week (once; declined, not again).
2. **A pause of 8 to 14 days:** "the last completed week is repeated" is the week before the one the
   pause began in; the plan goes on from the week it was paused in, later (no race date), or the next
   week is repeated (a race date, which cannot move).
3. **Over 14 days:** every run day is easy, the long-run day is a 5K time trial, until a trial is run;
   with no race date the plan then resumes at the week it was paused in. With a race date the calendar
   stands (the weeks passed are lost).
4. **A day moved stays moved** (no moving it twice), and a move that would lose a hard session is
   refused.
5. **A trial counts only if its effort ran its whole distance** (a lap pressed early gives no time).
6. **One 5K only:** recalibration uses the trial's 5K equivalent for paces; the half's time is untouched.

### Verified
- 122 host tests (27 `PlanRules`: every rule above, the JSON, an invalid memory refused, a full log
  with every key within its buffer).
- The text-fit audit: 0 problems (the new Today statuses with every offered kind, the trial's question,
  the Saved trial lines).
- The simulator: `capture_screens.sh offer pause ask changes` (`docs/screens/tp5-*.png`, looked at): a
  missed long run offered and accepted (plan.json: the Sunday logged missed, an override on Monday), a
  plan paused and resumed, a 5K of 22:50 waiting and answered "update" (app_config.json's 5K and
  plan.json's 5K both 22:50; the paces slower), tomorrow's session swapped in and today's skipped.
  `sim_plan.sh` still saves its session.
- `sim_trial.sh`: a 5K time trial run end to end in the simulator (about 17 minutes; the simulator's GPS
  runs 5 m/s). It found two faults, both fixed: (1) the session was "too short to count" (976 of 2820 s:
  the estimate takes a typical warm-up and cool-down), so the trial was never used; a trial or race whose
  effort ran its whole distance now counts, however short its warm-up. (2) Today was not refreshed after a
  save, so the app said the session was still to do (and did not ask about the new 5K) until restarted; it
  is refreshed as the run stops. Result of the last run: the effort 948 s, "Planned session done", "Trial 948
  s = a 5K of 948 s: action 2" (ask: a 28% jump), plan.json holding `"ak":948` with the 5K still 22:00, and
  Saved reading "Done / 5K 15:48, a jump: ask / Next: Wed, Easy+str".
- The watch target links (553,016 bytes).

### Not verified
- Nothing from the rules has run on a watch. The "apply" and "keep" results and the answers were checked
  by the host tests and by the `ask` capture (update); the first real time trial (week 4) is the test of
  the whole path on a watch.

### The gate (TP5): what Jon reads and checks
**Jon agreed the plan and the rules on 17 November 2026** ("Yes I do agree with that plan we have for TP5 and the
rest of the build"), and bundles TP5 into his test release 1.1.0 (see "Release 1.1.0" below); the watch is still to
see it. What he was asked to read and check:
1. **`Tests/Host/golden/plan_sim.md`** (or `build-tests/plan_sim [n]`): the 19 scenarios. Say what a
   rule does that you would not want, and which of the decisions above to change.
2. **On the watch** (the CI build of the last push): the Changes menu; and, when it happens to you, a
   missed session (this week's rows go orange), the offer, and what week 4's 5K time trial does to your paces.
3. **To decide:** the six decisions above; and whether the long run's share of the week (Daniels, 30%)
   should be changed (left from the Daniels check).

## TP6: the glance and `training_today.json` (built, 16 November 2026; at its gate)

Brief section 12. Jon, 16 November: "Happy for you to progress with the Glance whilst I build the
release packages." Jon has seen none of it yet: the gate is below.

### What was built
- **The file** (`Software/Libs/Core/Header/TodayFile.hpp`, `Sources/TodayFile.cpp`):
  `../SharedData/HybridX/training_today.json`, written by the app only. `{"v":1,"plan":1,"wks":10,
  "ps":0,"s":start,"e":end,"f":first,"t":last,"d":[[date,kind,status,runWeek,"what","pace"],...]}`: the
  plan's span, the window the file covers (the Monday of the week it was written through 14 days
  after, so at most 21 rows and about 1.2 KB), and a row for each plan day in it. Integers only and
  none negative (the family's rule); the strings are printable ASCII with no quote or backslash. An
  ended plan, or none, is `{"v":1,"plan":0}`. Reading checks every value (a newer version, a repeated
  or out-of-order day, a day outside the window or the plan, an escape or an over-long string, a
  number where a string goes, any truncation) and anything wrong reads as no data.
- **`TodayFile::pick`**: the one place that decides what the glance says for a date: no file, no plan,
  no clock, **stale** (today outside the window: a file from a fortnight ago, or one from the future),
  paused, before the start, complete, or a session, done, skipped or a rest day. The glance and the tests
  both use it.
- **`TodayBuild::fill`** (pure): the file's data from `PlanState`, through `PlanRules::sessionOn`, so a moved,
  eased or rested day is already in the rows; the status from the log; the words from the new
  `PlanText::glanceWhat` (`100 min, 20 fast`, `40 min + strides`) and the pace as `4:18/km` or `6:55/mi`
  (the pace of a session's main steps; easy runs have none). The service passes a spare `Program` for
  the paces and none if it has no memory for it (then the file has no pace).
- **The service** (`Service::writeTodayFile`, called at the end of `updateToday`): every open, save, move,
  skip, pause, answered offer, new 5K and ended plan reaches it. It touches neither `mProgram` nor
  `mToday`, rewrites the file only when its text differs from the last write (an FNV-1a hash), writes with
  `SafeFile` (so a look during a save reads `.bak`), and does nothing while the watch has no date (what is
  there stands).
- **The glance** (`Software/Libs/Glance`, `Apps/HXTrainingGlance-CMake`): `Service` (the SDK's glance steps,
  Run's nearly as it is), `GlanceView` (the file and today's date from `localtime_r`, with Streak's 2026-01-01
  clock floor, to a view: the state, today's row, the next session, the week's seven pips), `GlanceLayout`
  (two lines and the strip, 11 controls; the longest wording that fits the band, and cut to fit as a last
  resort). It never writes. 32 KB as a watch build.
- **Release plumbing:** `Utilities/pack-store-zip.sh --glance` (Run's, and the app's phone-settings bounds
  check, which the copy from Ride had lost, restored); `Resources/Glance/` (manifest, icon, four store
  previews); `docs/store/glance/` (a banner and three cards, from `hybridx-link/Tools/Brand`); CI builds the glance,
  puts its `.uapp` in `watch-apps` and packages it as its own artifact, `training-glance-store-zip`.
- **Tools:** `build-tests/today_print <file> [date] [--expect State]` reads a file with the core's reader and
  says what the glance would show; `build-tests/glance_preview` and `docs/experiments/glance_preview.py` draw
  every state from the real layout code; `sim_plan.sh` and `sim_trial.sh` check the file before and after a session.

### Where it differs from the brief
- **Two lines and a strip, not three lines.** The brief's table had three text lines (the third being "Week
  6 of 10, race in 33 days"). Only 18 and 20 point faces draw letters and 25 + 23 + 23 px do not fit 60 px
  (Streak NOTES S4.2), so the week is seven pips and the plan's progress a bar (weeks begun of all), as
  Streak's beads did. The race countdown and "Week 6 of 10" are not on the face; the Plan screen has them.
  The brief's section 12 now says so.
- No `race` key in the file (nothing shows it), and no written-at time (nothing read it).

### Decisions made while building (each cheap to change)
1. **A 196 px band, centred** (22 px in from each edge). Streak's words ran 14 px in on the left and 8 on the
   right and looked right in Jon's photo; the mock-up's darkened edges are 34 px (the list's separators span
   172). A wider band would show more of the pace.
2. **"Easy" for an easy run with strides**, whose second line says `40 min + strides`. `Easy+str` is the app's
   week-list word and is cryptic on a glance.
3. **A past session with nothing logged is an amber pip**, the same as a missed one: the glance cannot settle it
   (only the app does) and the truth is that it was not done.
4. **Only a session's target pace is shown** (intervals, cruise, tempo, race pace); an easy run shows its time.
5. **Skipped today** is amber, with the next session on the second line; **part done** counts as done.
6. **The no-plan line is `Set a plan in UNA`**, the brief's longer sentence being 230 px.
7. **CI packages the glance as a dry run, in its own artifact, without `--expect-version`.** It carries the
   development App ID (`C4B5BAD6DC827E16`, md5 of `HXTrainingGlance`) until Jon has created its listing; with
   `--expect-version` the packager refuses that, which would have failed a `training-v*` tag build of the app.

### Verified
- **Host tests: 166 pass** (44 new): `TodayFile` (round trip of every kind, status and an empty string; the
  largest file fits; every truncation refused; a changed byte never gives a file that breaks the rules; each
  rule of the file; more than 21 rows), `pick` (every state, the window's edges, a week change, a file from the
  future), `TodayBuild` (a real plan, every plan at every level on every third day of its length, the log and
  moves in the rows, paused, no plan), `GlanceView` and `GlanceLayout` (every kind in every state with the
  longest words fits the band by the faces' real widths; the control budget; everything inside the area; the
  adaptation to a short, narrow or poor area; the bar).
- **`sim_plan.sh`**: the file is written as the app opens and reads back as "Session"; after the 3-minute
  stepped run (which does not count: under half the planned time) it still says Session, correctly.
- **`sim_trial.sh`** (a whole 5K time trial, about 20 minutes): the file reads back as "Session" before the run
  and **"Done" straight after the save**, with no reopening of the app, with the next session (Wed, Easy+str 35
  min) after it. The trial's 951 s was a 5K of 15:51 and the app's own question about the paces was
  pending: the glance still says Done, which is right.
- **The ARM compile checks** of the glance (32 KB `.uapp`) and the whole app; the packager in both modes (the
  app's with the bounds check, and the glance's).
- **The preview sheet** and the four store previews, looked at.

### Not verified
- **Never on a watch.** The platform facts are Streak's and Run's (area 240 x 60, 32 controls, text and filled
  rectangles, the 18 and 20 point faces), but this glance's own text boxes and strip have not been seen. Streak
  NOTES S4.2 still has "18 point text sits inside its 23 px box" open too.
- **That the glance's `localtime_r` is the app's local date** (the same call as Streak's glance, which Jon has
  seen show the right week; the app uses the kernel's time tracker). A zone change between the two would move
  the day by a look.
- **The glance's read of `../SharedData/HybridX/training_today.json`** is Run's `vo2max.json` read, repeated;
  the app's write is checked by the simulator, the read by the host tests and the layout by the previews.
- **The portal:** the glance's listing and App ID (Jon), and the portal's view of a glance manifest like the
  others (the packager checks the same rules as for Run's glance).

### The gate (TP6): what Jon looks at and does
1. **The previews:** `hybridx-training/docs/store/glance/` and `Resources/Glance/previews/` (and, for every state,
   `glance_preview.py`). Say if a state's wording, the pips, or the bar want changing, and whether the race
   countdown ("race in 33 days") is missed enough to give up a pip row or a bar for it.
2. **On the watch:** the CI build of the last push gives `HXTrainingGlance_0.0.0-dev.uapp` and the app. Follow
   `hybridx-link/docs/FAMILY_WATCH_TEST.md` section 8 (steps 25 to 28): open the app once, then look at the glance
   before and after a session, and photograph it.
3. **To decide and do:** create the "X Training Glance" listing in the portal and send me its App ID (until then
   the glance carries a development ID and CI packages it as a dry run); and whether to tag `training-v1.0.0`
   now (the app without the glance's file, which the field test does not need) with the glance arriving in 1.1.0
   (recommended), or wait for it.

## Treadmill sessions (16 November 2026, Jon's request; built, simulator-checked, not yet on a watch)

Jon turned the main menu's **Treadmill** switch on and Today still recorded outdoors with GPS. **Cause (traced):** the
switch worked for a **Free run** (X Run's code, kept in TP2), but a session from **Today** ignored it on purpose: brief
decision 9 ("outdoors only for plan sessions") was built into `Model::trackStart`, `Service::handleEvent(TrackStart)`
and `WorkoutDetailsScreen::startToday` (which also waited for a GPS fix), and nothing said why. Jon asked for X Run's
treadmill way in the plan too ("outdoors is encouraged for accuracy, but sometimes treadmills must be used") and gave
the rule for intervals: **unless corrected, assume the treadmill is set to the designated speed**, so "400 m at 12 km/h"
takes the time that distance takes at 12 km/h and the lap changes by itself; "until lap press" stays a lap press.
Decision 9 is reversed (brief, REVIEW and the TP3 note are marked).

### What a treadmill session is
- **The start:** with the switch on, Today goes straight to the countdown (no GPS fix, no no-fix question); GPS and
  pressure are let go; the stride model fixes its tier for the run; no outdoor stride learning, no VO2max windows (as X
  Run's treadmill run, unchanged). A UNA-library workout still waits, as in X Run.
- **Belt speed per step** (`SessionMeta::beltMmps`, set by `PlanBuilder`, host-tested for every session at every level):
  a step with a pace target that is not an easy band (intervals, cruise, tempo, strides, 30-30s, repetitions, race pace
  and the race) gets the **middle of its target band**; the 400 m jog after a repetition, which has no pace, gets the plan's
  jog pace. Easy steps, warm-ups, cool-downs, hills, a timed jog and a time trial's effort (no designated speed) get 0: the
  cadence estimate. Open steps stay lap presses.
- **The run's speed and distance** (`Service`, the 1 Hz treadmill block): on a step with a belt speed, speed = that and
  distance += speed x dt, **while the cadence says the runner is running** (the estimator's own validity and 5 s hold:
  step off the belt and the distance stops); elsewhere the estimator's speed and its own distance step. The distance
  counter, speed counter, records, laps, run statistics, the gauge and the **workout engine** all take that, so a
  distance step ends by itself when speed x time reaches it. The estimator keeps running alone (its distance and steps per
  cadence bin are what the SDK's stride model learns against).
- **After Save** the session is held for the console's distance (`TreadmillDistanceScreen`, as X Run). The run's total (the
  belt speeds and the estimate) is what the screen offers and what its band is taken from; **the entry is "unless
  corrected"**: it sets the file's distance and average speed, and the SDK still learns from it against its own
  estimate (tier 3 only, runs of 2 km or more). A run with no assumed step (a free run, an easy session) takes exactly the
  old path. The `.fit` is `sub_sport` treadmill, with the estimate in the `distance_estimate` developer field.
- **The plan** counts the session by time as for an outdoor one, **recorded when the run stops** (not when the file closes,
  which waits for the distance) so the Saved screen can show its lines before the distance screen. A treadmill **time trial
  or race** still counts, but **never recalibrates the paces**: its distance is an estimate (or the assumed race pace).
- **What the screen says:** with the switch on the target is the belt speed ("14.0 km/h", "7.5 mph"; an easy band as
  "easy 9.8-11.0 km/h") on **Today**, in the **step list** and on the **step card**. The live gauge keeps its pace arc and
  readout (the assumed pace on a designated step). Switching the switch refreshes Today at once.

### Decisions made while building (each cheap to change)
1. **Pace buzzes are off on a treadmill** (speed targets only): on a designated step the assumed speed is always in its
   band, and on an easy step the estimate is too rough to police. Heart-rate alerts still work. (`processWorkout`.)
2. **Easy steps use the cadence estimate**, not the middle of their band: the easy band is wide and the runner picks the
   speed in it; the console's distance after the run is the check (Jon: "for easy runs based on time, it's easy with a
   distance confirmation after the run").
3. **A distance jog gets the plan's jog pace** (the only way for it to end by itself).
4. **A treadmill trial or race never recalibrates.**
5. **The belt is assumed while you run, not while you stand:** with no cadence the distance stops.

### Verified
- **Host tests: 169 pass** (3 new): belt speeds for every session, level and runner (set exactly on quality speed steps and
  distance jogs, inside the step's band, never on open, easy, hill or trial steps), a 400 m rep implies the right time,
  and the speed text (km/h, mph, bands).
- **Simulator** (`TREADMILL=1 sim_plan.sh`; the `.fit` checked by `fit_treadmill_plan_report.py`):
  - **Intervals 5 x 3 min:** Today reads "14.0 km/h", the step card "14.0 km/h", the Treadmill switch and the distance
    screen are as X Run's; no GPS; the file is running / treadmill, 169 records with a distance and no position, the
    total is the entry (810 m, over the run's own 582 m) and the activity link carries it; **each rep lap = belt speed x its
    time** (252 m vs 252 m).
  - **Repetitions 5 x 400 m** (`AUTO=1 WEEK=2`, 22 minutes, the reps and jogs ended by themselves): the 15.1 km/h rep took
    **95 s and 400.0 m** every time, the 8.8 km/h jog **164 s and 400.0 m**: 10 laps, 4000 m vs 3996 m by the rule; the run's
    own total 4075 m against the cadence model's 4266 m; the entry 4310 m taken.
  - **A counted session, the distance skipped** (`AUTO=1 AUTO_SKIP_SECS=120 TREAD_SKIP=1 WEEK=2`, 26 minutes): the plan
    log says **Done** (`[20261116, 10, 1, ...]`) the moment the run stopped, **before** the distance screen; Today and the glance
    read Done; the same 10 laps (95 s and 400.0 m, 164 s and 400.0 m); with the distance skipped the file takes the run's
    own total (4812 m, `distance_estimate` the same, the cadence model's own 5003 m kept for the stride model); the activity
    link carries 4812 m as a Run.
  - **The Saved screen** on a treadmill session shows the plan's lines ("Not counted, Week 5: 0 of 4, Next: Tue, Easy+str" for
    a short run) before the distance screen, then "No map on a treadmill" in the summary.
  - **A Free run with the switch on** (`sim_treadmill_free.sh`, the path Jon may also have used): treadmill sub-sport, no
    positions, the entry taken, the link carrying it. Unchanged from X Run.
- **Watch compile check** of the whole app (556,912 bytes); the **text-fit audit** (0 problems: the belt-speed targets are no
  longer than the pace bands it already measures).

### Not verified
- **On a watch.** Whether the cadence-derived speed on a real belt agrees with the assumed one (tier 1 is a stride from your
  height); whether a real step off the belt drops the cadence fast enough to stop the distance; the feel of an interval that
  ends by itself on a treadmill. The console's distance after the run is the check, and Jon's test is
  `hybridx-link/docs/FAMILY_WATCH_TEST.md` section 9 (steps 29 to 35).
- **A treadmill time trial or race end to end** (the guard is a one-line condition in `recordPlannedRun`, read, not run: a 5K
  at the simulator's treadmill speed would take about an hour).
- **Imperial** targets on the screens ("7.5 mph"): the text is host-tested; the simulator ran metric.

## Release 1.1.0 (17 November 2026)

Jon published **1.0.0** before the update work, and the store takes a version once. What `training-v1.0.0` is: the merge of
PR #62 on the morning of 16 November (the same commit as `streak-v1.1.0` and `run-v1.1.0`): TP4 and before. It has **no**
plan memory (TP5), no glance file (TP6), no treadmill sessions, and not the release set-up (the portal's App ID, the
packager, the CI package). **1.1.0** is tagged on the branch's newest commit and carries all of those: the plan's memory
and recalibration (Jon agreed its plan and rules on 17 November), the file the glance reads, and the treadmill sessions.
The CHANGELOG's 1.1.0 holds them (the TP5 line moved out of 1.0.0, which the tag did not contain); the portal's "what's
new" text is in `hybridx-link/docs/STORE_RELEASE.md`. Not yet seen on a watch: TP5's rules, the glance, the treadmill
sessions: `hybridx-link/docs/FAMILY_WATCH_TEST.md` sections 7 to 10 are Jon's test. The glance has no portal listing or App ID
yet, so it is not part of this upload (CI packages it as a dry run).
