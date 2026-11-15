# HybridX Training for UNA Watch: Brief and Plan

**Owner:** Jon
**Status:** Version 2 (14 November 2026), after the independent review (`REVIEW.md`). Phase
TP0 (decisions and desk checks) is closed in `NOTES.md`. Nothing is built. Next: TP1, the
pure core, on Jon's word.
**Read with:** `PLANS.md` (the pace model, the sessions and the three plans), `REVIEW.md`
(what the review found and changed), `NOTES.md` (decisions, verified SDK facts, the TP1 plan).
**Name (Jon, 14 November 2026):** shown on the watch as **X Training**. Project name HybridX
Training; app folder `HybridXTraining`; repository folder `hybridx-training/`; tags
`training-vX.Y.Z`.

---

## 1. The idea

A running app that coaches you through a training plan. It records runs as X Run does
(GPS, pace, heart rate, laps, a FIT file that syncs to the UNA app and Strava), and it knows
what today's run is and talks you through it step by step, as a Garmin does with a
structured workout.

- **Three built-in plans:** 10K (10 weeks), half marathon (12 weeks) and a VO2max block
  (8 weeks), each at three levels, for 3 to 6 runs a week on the days you choose.
- **Set up on the phone, worked out on the watch.** In the UNA app you enter a recent 5K
  time (and a half-marathon time if you have one), the plan, the race date and your running
  days. The watch calculates the rest: your paces, the calendar, each day's workout and its
  targets. No server and no phone app of ours.
- **Today's workout on opening:** "Intervals, 4 x 4 min at 4:12-4:24 /km", what it is for,
  and its steps. One press starts it.
- **A guided run.** Each step counts down. The pace is shown against the step's band, with a
  buzz when you drift out and another when you are back; five beeps lead into each change;
  a card shows the next step. On easy days the watch only ever says "too fast".
- **The plan remembers.** Sessions done, part done and missed; nothing is stacked; the key
  session and the long run come first; time trials and tune-up races update the paces.
- **Part of the family.** Each run counts in X Streak with its training load, shows the
  Streak line, and feeds X Run's VO2max.
- **Later: a glance** with today's workout, the only "reminder" the platform allows.

Not in v1: building your own plan, changing the plan from heart-rate data, treadmill plans.

## 2. The overlap with UNA's own Running

UNA's Running already runs structured workouts (SDK v1.6.0) and lists dated files in
`:/Workouts/Schedule/` as "Today" (`WorkoutLibrary.hpp`, `ScheduleState.hpp`); the files
arrive only over USB for now (Race NOTES 5.22, in the v1.6.0 review). A phone service that
fills that folder would be a training-plan feature, and that is how Intervals came to be
retired (Race NOTES 5.25).

**Jon's decision (14 November 2026): build it, as a new app.** What only this app does:
plans and paces generated on the watch from your own 5K, adapted to what you ran, counted in
Streak, linked to X Run's VO2max, shown on a glance. Apps cannot write to `:/Workouts`
(`Docs/shared-data.md`), so it runs the workouts itself.

Still worth doing alongside, and not blocking: ask UNA whether plans or phone-delivered
schedules are coming (the question is drafted in `NOTES.md`).

## 3. What it does (v1)

| # | Feature | Notes |
|---|---|---|
| 1 | Three plans, three levels, 3-6 runs a week | `PLANS.md` §4-7. Content lives in plain text files (section 11) |
| 2 | Paces from a 5K, and a half if given | Daniels–Gilbert equations; fractions checked against published table rows (`PLANS.md` §2) |
| 3 | Set-up on the phone, with a fallback on the watch | Config fields (section 5) |
| 4 | Today, This week, Plan, Paces screens | Section 8 |
| 5 | Guided workouts, dosed by time | The SDK's engine, gauges and alerts; RunLVGL's workout faces |
| 6 | Recording | Everything X Run records, plus the workout and its targets in the FIT file |
| 7 | The plan's memory | Section 9 |
| 8 | Recalibration | Time trials, tune-up races, a faster 5K found in a run, a new time on the phone |
| 9 | Family links | A new Streak slot, training load, the Streak line; X Run's VO2max both ways (section 10) |
| 10 | Glance (later phase) | Section 12 |

## 4. What the platform already gives us

Read from the SDK at the pin (`sdk-v1.6.2`, `6bd43f04`) and checked again in the review.

| Need | What exists | Where |
|---|---|---|
| A workout as data | `SDK::Workout::Program`: up to 100 steps, repeats nested 4 deep, time, distance or open ends, speed or heart-rate targets. About 15 KB, one long-lived instance | `Libs/Header/SDK/Workout/WorkoutProgram.hpp` |
| Running it | `SDK::Workout::Engine`: fed active time and distance each second; ends a step exactly at its time or distance; `remainingMs`, `rep` of `reps`, `peekNext()`, `reachedEnd` | `WorkoutEngine.hpp` |
| Building one in the app | Allowed ("an app builder that keeps the same rules"); UNA's `buildIntervals()` is one. The rules are the reader's: `NOTES.md` | `WorkoutEngine.hpp`, `Libs/Source/Workout/Intervals.cpp`, `FitWorkoutReader.hpp` |
| Starting one | Fill `mProgram`, then `mEngine.start`, `mLeadIn.stepStarted`, `startWorkoutStep` | RunLVGL `Service.cpp`, `startWorkout()` |
| Targets | `SpeedGauge` (200 m window, 3 s/km deadband, hold-off and dwell), `HeartRateGauge`; a zone target resolves against the watch's own zones | `TargetGauge.hpp`; `Service::hrBand()` |
| When to buzz | `LeadInAlert` (last 5 s of a step), `ZoneAlerts` (out, every 30 s while out, back in; none in a step's first 20 s). What each alert plays is the app's code | `WorkoutAlerts.hpp`; `Service::playZoneAlert()` |
| The workout in the FIT file | `FitWorkoutWriter`, from each step's raw FIT values; FIT has Speed and HeartRate targets | `FitWorkoutWriter.hpp`, `FitProfile.hpp:74` |
| The screens | The workout details, the step card (3 s at each new step), the gauge face, the step face | RunLVGL `WorkoutFaces.hpp`, `TrackWorkoutStepScreen.hpp` |
| Phone settings | Up to 32 config fields (`string`, `int`, `float`, `bool`), read at launch, written back with `save()` | `Docs/app-config-fields.md`; `SDK/AppConfig` |
| Sharing | `../SharedData/` only; fixed names, never list | `Docs/shared-data.md` |

What it does not give: scheduled code (no reminders), writing to `:/Workouts`, a VO2max of
UNA's own, glances that open their app.

So the app is **plan logic and screens on machinery UNA has already built**, on top of
everything X Run already does.

## 5. What the runner enters, and where

### 5.1 On the phone (15 config fields)

The watch checks every value again: the watch-side reader has no pattern check, and the
phone's has not been seen working (Race's T23 is still open). Patterns have no `^` or `$`
(`app-config-fields.md` §3.2).

| id | Type | Label | Rule | Default |
|---|---|---|---|---|
| `plan` | int 0-3 | Plan | 0 none, 1 10K, 2 half marathon, 3 VO2max | 0 |
| `fiveK` | string ≤ 8 | Your 5K time | `mm:ss`, 12:00 to 59:59; empty = not set | "" |
| `fiveKRecent` | bool | 5K run in the last 6 weeks | Off: the plan starts with a time-trial week | true |
| `halfTime` | string ≤ 8 | Half marathon time (optional) | `h:mm:ss`, 0:55:00 to 3:59:59; empty = none | "" |
| `raceDate` | string ≤ 10 | Race date (optional) | `YYYY-MM-DD`; empty = the plan runs its length from its start | "" |
| `level` | int 1-3 | Your running now | 1 up to 20 km (12 mi) a week, 2 about 20-40 km, 3 over 40 km | 2 |
| `runMon` … `runSun` | 7 × bool | Run on Monday … Sunday | 3 to 6 on | Tue, Wed, Fri, Sun |
| `longRunDay` | int 1-7 | Long run day | 1 Monday to 7 Sunday; must be a run day | 7 |
| `easyByHr` | bool | Easy runs by heart rate | Easy and long runs use the watch's zone 2, not a pace | false |

Units, heart-rate zones and the date come from the watch (`app-config-fields.md` §1.1).

**When a value changes.** The app keeps its own copy of what it last used. A new 5K or half
is dated the day it is first seen and changes the paces from the next session. A new plan or
race date starts a new plan after a confirmation on the watch. New run days re-place the
weeks still to come.

### 5.2 On the watch

- **A set-up fallback** (Plan → New plan): plan, 5K time, race date (a week picker), run
  days. Written back with `AppConfig::save()` as Race does (Race NOTES 5.22, "Run length set
  on the phone was ignored"), so phone and watch agree. It also covers a USB install.
- **Settings:** easy runs by heart rate, reps by distance (off: reps are by time), alerts,
  lead-in beeps.
- **In the moment:** do tomorrow's session today, move today to tomorrow, skip, do
  yesterday's missed long run, pause the plan, end the plan.

## 6. The paces, in brief

Method, checks and examples: `PLANS.md` §2.

1. **Fitness from the 5K** by the Daniels–Gilbert equations. It stays internal: the runner
   sees a 5K equivalent and predicted times, never a number that looks like VO2max.
2. **Training paces:** easy 62.5-72.5% of the fitness, threshold 88%, interval 97.5%,
   repetition 6 s per 400 m faster than interval; half and 10K paces from the race
   equation. These reproduce the published table rows found, within 3 s/km, and the race
   paces agree with Riegel within 15 s (`REVIEW.md` §1). Ten rows from the book itself are
   still wanted as test fixtures.
3. **The half-marathon time, if given,** sets half pace and the easy paces, and the gap is
   put in words ("your endurance is behind your speed").
4. **Bands:** ±5 s/km at threshold and race paces, ±6 s/km for intervals; easy steps have a
   ceiling only.
5. **Nothing from X Run's VO2max** (section 10.3).

A 22:00 5K gives: easy 5:27-6:08 /km, half 4:48, threshold 4:40, 10K 4:34, interval 4:18,
repetition 4:03; predicted 10K 45:37, half 1:41:07 [computed].

## 7. How the plan becomes today's workout

```
 phone fields + watch set-up ──► PlanSetup (validated, dated)
                                     │
 5K / half / time trials ──► PaceModel ──► paces and bands (mm/s)
                                     │
 Tools/Plans/*.plan ─gen─► PlanData ──► PlanCalendar + WeekLayout ──► today's slot
   (text, a coach edits)                (start, race date, run days, moves, pauses)
                                     ▼
                         Session recipe: "INT 4 x 4", level 2
                                     │  PlanBuilder: paces, level caps, time or distance reps
                                     ▼
                      SDK::Workout::Program (the service's one instance)
                                     │
        X Run's workout mode: Engine, gauges, alerts, faces, FitWorkoutWriter
                                     │  on save
                                     ▼
        PlanState ──► act_training.json, training_today.json, training_vo2_*.json,
                      the Streak line
```

- **Calendar.** Weeks start on Monday. A plan set up from Monday to Wednesday starts that
  week (days already gone are "before the start", not missed); otherwise the next Monday.
  With a race date it is counted back from the race, dropping base weeks if short
  (`PLANS.md` §4.3). A session belongs to the day it started. Without a valid date there is
  no "today", only the plan.
- **Placing the week** on the runner's days: `PLANS.md` §4.2, checked on all 392
  combinations.
- **Building the Program:** minutes at an intensity become steps by time (or by distance,
  rounded to 100 m, with "Reps by distance" on). Interpreted and raw FIT fields are both
  filled, because `FitWorkoutWriter` writes the raw ones. A host test writes every session to
  FIT, reads it back with `FitWorkoutReader` and requires the same program.
- **Week and plan screens never build Programs;** their totals come from the recipes.

## 8. Telling the runner

British English, integer formatting, ASCII on the watch ("4 x 4 min") until the text-fit
audit proves a glyph, and every screen through the audit, as X Sauna, Ride and Workout.

### 8.1 Before the run

| Screen | Shows |
|---|---|
| **Main menu** | **Today** first, its hint the session; then Free run, This week, Plan, Paces, Workouts (UNA's library), Settings |
| **Today** | "TUE, WEEK 6 OF 10"; "INTERVALS"; "4 x 4 min"; "@ 4:12-4:24 /km"; "about 930 m each"; "9.2 km, 48 min"; "Builds your VO2max". One press starts it; the steps are a scroll away |
| **Rest day** | "Rest day" and the next session ("Fri: Cruise intervals") |
| **Offers** | "Do yesterday's long run today?"; "Intervals were missed: do them in place of today's session?" (the key-session rule) |
| **This week** | Seven rows from Monday: the session's short name and a mark for done, part done, missed, today, to come. Choosing a row shows its steps |
| **Plan** | "Week 6 of 10, Build"; a bar of weeks; "19 of 20 sessions"; "Race in 33 days, Sun 27 Dec"; X Run's VO2max and its change since the plan began |
| **Paces** | Each pace; "From your 5K of 22:00, 12 Nov"; "Predicted: 10K 45:37, half 1:41:07" |

### 8.2 During the run

X Run's (RunLVGL's) workout mode:
- a countdown, then the first step card ("WARM UP, 15 min, easy");
- the **gauge face**: live pace on a slow / in range / fast arc, with what is left of the
  step ("1:45 left");
- the **step face**: the target, the lap's average so far (each step is a lap), "Rep 3 of 4";
- a **step card** for 3 s at every change, with the same alert whether the step ran out or
  lap was pressed;
- **lead-in:** a beep a second over the last 5 s of a step;
- **zone alerts:** too slow is long and falling, too fast short and rising, back in range a
  rising triad, repeated every 30 s while out; none in a step's first 20 s. The rhythms
  differ, so they work on firmware without buzzer pitch (`playZoneAlert`);
- **easy steps** (easy and long runs, warm-ups, cool-downs, recoveries) play **"too fast"
  only** (`REVIEW.md` C3);
- **short reps** (strides, repetitions, 30-30s) end before alerts are allowed, so they are
  judged afterwards on the next step's card: "Last rep 1:36, target 1:34-1:40";
- X Run's other faces stay one button away.

### 8.3 After the run

The summary, then Saved with: "Session done" or "Part done"; "Week 6: 1 of 4 done";
"Next: Wed, Easy run 45 min"; the Streak line and "Load 64". After a time trial: "5K 21:41.
New paces from your next run", or, if slower, the question in `PLANS.md` §2.6.

## 9. The plan's memory

- **Done:** the engine reached the end (`reachedEnd`, read before `next()` when the runner
  ends it, as `WorkoutEngine.hpp` warns) and the activity saved. **Part done:** saved, at
  least half the planned time. **Missed:** the day passed with neither. A free run on a
  planned day counts only if the runner says so, for easy and long runs.
- **Rules** for missed sessions and weeks, moves and pauses: `PLANS.md` §4.3.
- **Where:** `plan.json` in the app's folder (the family's `SafeFile`): the set-up, the
  fitness with its source and date, and a log of at most 84 days. About 2 KB, integers only.
- **Not judged in v1:** whether the paces were hit. Lap averages are in the FIT file.

## 10. The family

### 10.1 Streak and training load

- A new activity-link slot, `training` → `../SharedData/HybridX/act_training.json`, sport 1
  (run), with the training load as the 8th number.
- **It is a family change** (`REVIEW.md` E3). The link copies are byte-identical and
  `StreakNow` is sized by `kSlotCount`, so a sixth slot means a Streak release (to count the
  runs) and a rebuild of the other apps (for their Streak line to include them).
- The Streak line on Saved, as X Run.

### 10.2 X Run's VO2max, fed by training runs

X Run's VO2max counts runs within 42 days of the latest, so a runner using only X Training
would see it go stale. As Trail does (`trail_vo2_0..2.json`), X Training leaves each run's
steady minutes in `training_vo2_0..2.json`. Starting from X Run, the window builder is
already in the service; what changes is that the windows are saved for X Run instead of
scored. X Run needs a release to read the new files.

### 10.3 X Run's VO2max, shown

- On the Plan screen, and as the VO2max block's second measure, read from `vo2max.json`.
- **Never used for paces:** it is on the ACSM scale, 21% above Daniels' at 5:00 /km
  (RACE_PREDICTIONS §4.3). The block's main measure is its own 5K time trials.
- Without X Run installed the VO2max lines are simply absent.

## 11. Architecture

**Start from X Run** (`REVIEW.md` E1): a verbatim copy of `hybridx-run/Software` in its own
commit, then renamed, then stripped of what stays X Run's (the VO2max history and
re-scoring, the Trail ingest, the VO2 screens and profile, the VO2max glance, the bests
page), then the plan added. Kept: recording, UNA's workout mode, the detail faces, cadence,
treadmill free runs, the link, the load, the Streak line.

```
hybridx-training/
  docs/                  brief, PLANS.md, REVIEW.md, NOTES.md, screens/, experiments/
  Tools/Plans/           10k.plan, half.plan, vo2max.plan, gen_plans.py
  Tools/                 plan_print (TP1), plan_sim (TP5)
  Software/
    Apps/HybridXTraining-CMake/      the watch build (Activity)
    Apps/HXTrainingGlance-CMake/     the glance (TP6)
    Apps/LVGL-GUI/                   X Run's GUI and the plan screens
    Libs/Core/                       pure C++, host-tested; header-only where the GUI uses it
    Libs/App/                        X Run's service and the plan
    Libs/Link/                       copy of hybridx-link (CI-checked)
    Libs/Glance/                     TP6
  Tests/Host/                        with golden plan_print outputs
```

**The plans are text** (`REVIEW.md` E2). `Tools/Plans/*.plan` hold the weeks in the
notation of `PLANS.md` ("INT 4 x 4", "L 100 (20 HM)"); `gen_plans.py` writes
`Core/Header/data/PlanData.hpp`, which is never edited by hand. Changing a session is a
one-line text edit, and the golden outputs show its effect in review.

**Core** (SDK use: `WorkoutProgram.hpp`, a plain struct, and the JSON reader and writer):

| Module | Does | Tests |
|---|---|---|
| `PaceModel` | Fitness from a race, paces, bands, predictions, the half's gap | Fixture rows of the published table (±3 s/km); the race table (±10 s); Riegel within 1% |
| `PlanData` (generated) | The plans' weeks and recipes | The generator refuses a bad file; every week has a long slot and at most one hard long run with one quality |
| `WeekLayout` | Slots onto the runner's days | All 392 combinations: the properties in `PLANS.md` §4.2 |
| `PlanCalendar` | Dates, the start rule, counting back from a race, truncation | Month and year ends, leap days, races too close or past |
| `PlanBuilder` | Recipe, paces and level → `Program`, with the caps | Every session, level and 5K from 15:00 to 45:00: valid by the reader's rules, within the caps, unchanged by a FIT write and read |
| `PlanState` | The log and its JSON; done, part done, missed | Corrupt, truncated and one-byte-changed files read as "no plan" |
| `PlanRules` (TP5) | Missed sessions and weeks, moves, pauses, recalibration | `plan_sim` scenarios |
| `PlanText` | Names, purposes, the Today, Saved and glance lines | Worst-case widths (the audit) |
| `TodayFile` (TP6) | The glance's file | Round trip; stale and future files |

**Service:** at GUI start, read the config and `plan.json`, settle missed days, build
today's Program into `mProgram`, send `PlanToday` and `PlanWeek` (fixed-size structs). A
planned start is `startWorkout`'s three calls. On save: `plan.json` first, then the link,
the VO2 windows, the today file and the Streak line, each only a warning if it fails. New
message IDs clear of X Run's.

**Memory:** one Program (15 KB), as now; plan data in flash (a few KB); `plan.json` read
into a heap buffer at open and freed; nothing allocated in the 1 Hz loop; the LVGL pool at
X Run's 56 KB, screens built only when shown.

**Builds:** CI builds the app (and later the glance) into `watch-apps` and checks the link
copies. Local builds are compile checks only.

## 12. The glance (TP6)

- A second `.uapp`, `HXTrainingGlance`: display only, 240 × 60, 16 colours.
- It reads `../SharedData/HybridX/training_today.json`, written by the app when it opens,
  after each save and when the plan changes. It holds the next 14 days, so the glance stays
  right for a fortnight without the app being opened:
  `{"v":1,"plan":1,"wk":6,"wks":10,"race":20261227,"d":[[20261124,3,0,48,9200,"4 x 4 min"],...]}`
  (the exact format is TP6's).

  | State | Line 1 | Line 2 | Line 3 |
  |---|---|---|---|
  | A session today | TODAY: INTERVALS | 4 x 4 min @ 4:18 | Week 6 of 10, race in 33 days |
  | Done today | DONE: INTERVALS | Next: Wed, Easy 45 min | Week 6 of 10, 1 of 4 done |
  | Rest day | REST DAY | Tomorrow: Cruise 3 x 10 | Week 6 of 10, 2 of 4 done |
  | File older than 14 days | X TRAINING | Open the app to update | |
  | No plan | X TRAINING | Set up a plan in the UNA app | |

## 13. Phases and gates

Each phase starts in plan mode and stops at its gate.

| Phase | Work | Gate |
|---|---|---|
| **TP0** | Decisions, the independent review, desk checks, `NOTES.md` | **Done** (14 November 2026). Open items carried in `NOTES.md` |
| **TP1** | The pure core with host tests; the plan files and generator; `plan_print` with golden outputs | Jon reads `plan_print` for three runners in each plan (5K 20:00, level 3, five days; 25:00, level 2, four days; 32:00, level 1, three days) and corrects the plan files |
| **TP2** | The app: X Run copied verbatim, renamed, stripped; the config fields; today's Program run through the workout mode; `plan.json`. `sim_plan.sh` runs a session end to end; `fit_check.sh` decodes the FIT | Jon installs the CI build, sets a plan by USB, runs one real session |
| **TP3** | The screens (section 8), the easy-step alerts, the short-rep result; the audit; `capture_screens.sh` | Jon reviews the captures, then a week of the plan on the watch |
| **TP4** | Family: the `training` slot (Streak release, family rebuild), the Run link (Run release), VO2max on the Plan screen; the family check gains X Training | The family check; Jon's watch test |
| **TP5** | The plan's memory and recalibration; `plan_sim` | Jon reviews the scenarios and the rules |
| **TP6** | The glance and `training_today.json` | Previews, then on the watch |
| **TP7** | Field test (Jon, and ideally a level 1 and a level 3 runner, three weeks each), then the store release as the family's phase P | Jon's go |

## 14. Decisions

**Made (Jon, 14 November 2026):** build it; a new app; the name X Training.

**Proposed defaults TP1 will use unless Jon says otherwise** (each cheap to change):
1. Start from X Run (TP2).
2. The phone fields in 5.1, with seven run-day switches.
3. Sessions dosed by time; "Reps by distance" as a watch setting.
4. The level caps and tables in `PLANS.md` §3.3 and §4.1.
5. Easy steps alert for "too fast" only.
6. No fitness number on screen: a 5K equivalent and predictions.
7. Weeks start on Monday.
8. Recalibration: faster applies, slower asks, a big jump asks.
9. Outdoors only for plan sessions; treadmill stays for free runs.
10. UNA's Workouts library stays on the menu; RunLVGL's Intervals goes.

**Still Jon's:** the plans' content (`PLANS.md` §9); the ten table rows from Daniels' book;
whether to send UNA the question; later, how X Run and X Training sit together on the store
(`REVIEW.md` E9).

## 15. Questions for UNA

1. Are training plans or phone-delivered schedules planned for Running, and could third
   parties supply them?
2. Does the UNA app enforce a string field's pattern? (Race's T23 would show.)
3. Is there a size or RAM limit for an app?
4. Could a glance open its own app one day (Streak NOTES S4.4)?

## 16. Risks

| Risk | Effect | Mitigation |
|---|---|---|
| UNA ships plans in Running | Overlap, as with Intervals | Jon's decision; the differentiators in section 2; the core is reusable |
| Phone fields not shown or checked | No phone set-up | The watch set-up; the watch validates everything |
| An old 5K entered as current | Paces too fast | `fiveKRecent`; base weeks have little paced quality; time trial in week 4 |
| The pace fractions differ from the book | A few s/km | Checked against secondary rows; book fixtures before release |
| GPS on short reps | Unreliable live pace | No live alerts there; judged by rep time |
| Advice that hurts someone | Injury | Level 1 its own plan; caps; cut-backs; no stacking; the "not medical advice" text |
| Copied content | Copyright | Original plans; equations, not tables; the brand name "VDOT" not used |
| A family change for the Streak slot | Streak and others need releases | Planned as TP4, after the app works alone |
| Size and memory | The app does not fit | It starts at X Run's size; one Program; CI watch builds each phase |
| The watch's date unset | Wrong day | No "today" without a valid date |

## 17. Sources

`PLANS.md` §10 for the training science; `REVIEW.md` §1 for what was checked and how. The
SDK at `sdk-v1.6.2`: `Libs/Header/SDK/Workout/*`, `SDK/Fit/FitWorkoutReader.hpp` and
`FitWorkoutWriter.hpp`, `SDK/AppConfig`, `Docs/app-config-fields.md`, `Docs/shared-data.md`,
RunLVGL. The family's notes: Race NOTES 5.21-5.28 and 5.34, Intervals NOTES, Run NOTES and
RACE_PREDICTIONS.md, Streak NOTES S4.4, `hybridx-link/docs/ACTIVITY_LINK.md`, Trail NOTES
"Run link".
