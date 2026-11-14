# HybridX Plan for UNA Watch: Brief and Plan

**Owner:** Jon
**Status:** Brief only (14 November 2026). Nothing is built. Next: Jon's decisions
(section 14), above all the first one (section 2), then phase TP0.
**Read with:** `PLANS_DRAFT.md` (the pace model, the session library and the three draft
plans, week by week).
**Working name:** HybridX Plan, shown on the watch as **X Plan** (decision 14.2). The folder
on the watch would be `HybridXPlan`.

---

## 1. The idea

A running app that coaches you through a training plan. It records runs as X Run does
(GPS, pace, heart rate, laps, a FIT file that syncs to the UNA app and Strava), and it also
knows what today's run is and talks you through it step by step, as a Garmin does with a
structured workout.

- **Three built-in plans:** 10K (10 weeks), half marathon (12 weeks) and a VO2max block
  (8 weeks). Draft content in `PLANS_DRAFT.md`.
- **Set up on the phone, worked out on the watch.** In the UNA app you enter a recent 5K
  time (and, if you have one, a half-marathon time), the plan, the race date and how many
  days a week you run. Everything else is calculated on the watch: your training paces, the
  calendar, each day's workout and its targets. No server, no phone app of ours.
- **Today's workout on opening.** The first thing the app shows is today's session:
  "Intervals: 5 × 1 km at 4:12-4:24 /km", what it is for, and its steps. One press starts it.
- **A guided run.** Each step counts down its time or distance. The pace (or heart rate) is
  shown against the step's band, with a buzz when you drift out and another when you are
  back. Five beeps lead into the end of each step, and a card shows the next one.
- **The plan remembers.** Sessions done, part done and missed; a missed session is never
  stacked onto another day; a time trial (or a faster 5K) updates the paces.
- **Part of the family.** Each run counts in X Streak (with its training load), shows the
  Streak line on its Saved screen, and feeds X Run's VO2max. The VO2max plan reads X Run's
  VO2max to show what the block did.
- **Later: a glance** with today's workout, which is also the only "reminder" this platform
  allows (section 12).

What it is not: a plan builder, a coach that changes the plan from your heart rate, or a
treadmill app (v1 plans are outdoors; section 14.9).

## 2. Read this first: the overlap with UNA's own Running

On 14 October 2026 Jon retired HybridX Intervals because UNA was building structured
workouts into Running (`hybridx-race/docs/NOTES.md` 5.25). That is the closest precedent for
this app, and part of what made Intervals redundant is also here:

- **UNA's Running already runs structured workouts** (SDK v1.6.0): step targets, the pace
  gauge, zone alerts, lead-in beeps. They come from FIT files in `:/Workouts`.
- **UNA's Running already has a schedule.** Files in `:/Workouts/Schedule/` named
  `YYYY-MM-DD.fit` are listed as that day's workout, with "Today" first, and
  `ScheduleState` records whether it was completed (`Libs/Header/SDK/Workout/WorkoutLibrary.hpp`,
  `ScheduleState.hpp`). Today the files arrive only over USB; phone delivery is "to come"
  (Race NOTES 5.22, in the v1.6.0 review).

So UNA has the pieces of a training-plan feature, and a phone-side plan service that drops
dated workouts into `Schedule/` would be a natural next step for them. **What only this app
would do:** generate the plan and its paces on the watch from your own 5K, adapt it to what
you actually ran, count it in Streak with its training load, link it to X Run's VO2max, and
show today's run on a glance. Apps cannot write to `:/Workouts` (`Docs/shared-data.md`; Race
NOTES 5.26), so we cannot feed UNA's Running instead: the app must run the workouts itself,
which the SDK allows.

**Decision 14.1:** go ahead, or first ask UNA whether training plans or phone-delivered
schedules are on their roadmap (question 15.1). My recommendation: ask, and do TP0 and TP1
(desk work and the pure core, reusable whatever UNA says) while waiting; hold TP2 onwards for
the answer.

## 3. What it does (v1)

| # | Feature | Notes |
|---|---|---|
| 1 | Three plans: 10K, half marathon, VO2max | Draft week-by-week content in `PLANS_DRAFT.md` §5-7. Three levels each, 3 to 5 runs a week |
| 2 | Paces from a 5K time, and a half-marathon time if given | The Daniels–Gilbert equations (published 1979; already checked in `hybridx-run/docs/RACE_PREDICTIONS.md` §4.3). `PLANS_DRAFT.md` §2 |
| 3 | Set-up on the phone, with a fallback on the watch | UNA app config fields (section 5); the same values can be entered on the watch and are written back |
| 4 | Today's workout, this week, the whole plan, the paces | Screens in section 8 |
| 5 | Guided workouts | The SDK's workout engine, target gauges and alerts, and RunLVGL's workout faces (section 4) |
| 6 | Recording | As X Run: GPS, heart rate, laps (one per step), a FIT file carrying the workout and its targets |
| 7 | The plan's memory | Done, part done, missed, moved; no stacking; repeat or cut back a week after missed ones (section 9) |
| 8 | Recalibration | From the plan's own 5K time trials, a faster 5K inside any plan run (offered, not forced), or a new time on the phone |
| 9 | Family links | Streak (a new `act_plan.json` slot, training load, the Streak line); X Run's VO2max both ways (section 10) |
| 10 | Glance (later phase) | Today's workout and the week (section 12) |

## 4. What the platform already gives us

All read from the SDK at the pin (`sdk-v1.6.2`, `6bd43f04`); nothing here is assumed.

| Need | What exists | Where |
|---|---|---|
| A workout as data | `SDK::Workout::Program`: up to 100 steps, repeats nested 4 deep, time, distance or open ends, speed or heart-rate targets, intensity, names and notes. About 15 KB: one long-lived instance | `Libs/Header/SDK/Workout/WorkoutProgram.hpp` |
| Running it | `SDK::Workout::Engine`: fed active time and distance once a second; reports Auto or Manual step changes; interpolates the exact end; `remainingMs` / `remainingCm`, `rep` of `reps`, `peekNext()`, `reachedEnd` | `WorkoutEngine.hpp` |
| Building one on the watch | Programs may come "from an app builder that keeps the same rules" (`totals()` doc). `buildIntervals()` is UNA's own example of one | `WorkoutEngine.hpp`, `Intervals.hpp`, `Libs/Source/Workout/Intervals.cpp` |
| Pace and heart-rate targets | `SpeedGauge` (live speed blended into a 200 m window average, 3 s/km deadband, hold-off and dwell) and `HeartRateGauge` | `TargetGauge.hpp` |
| When to buzz | `LeadInAlert` (a beep a second over the last 5 s of a step) and `ZoneAlerts` (out of band, every 30 s while out, back in; quiet for the first 20 s of a step) | `WorkoutAlerts.hpp` |
| Text for steps | `stepLabel()`: "400 m 3:45-4:05 /km", "90 s rest"; km or miles | `StepLabel.hpp` |
| The workout in the FIT file | `FitWorkoutWriter` writes `workout` and `workout_step` from each step's raw FIT values; FIT now has Speed and HeartRate targets as well as Open | `FitWorkoutWriter.hpp`; `FitProfile.hpp:74` |
| The screens | RunLVGL (v1.6) has the workout list, details, a step card shown for 3 s at each new step, a gauge face (the live value on a slow / in range / fast arc) and a step face (target, lap average, rep) | `Examples/Apps/RunLVGL/.../WorkoutFaces.hpp`, `TrackWorkoutStepScreen.hpp` |
| Settings from the phone | Config fields: up to 32, `string` (with a pattern), `int`, `float`, `bool`; read at launch through `SDK::AppConfig`; the app may write them back | `Docs/app-config-fields.md` |
| Data from the watch | Units, the heart-rate zone thresholds, daily goals: do not re-ask for them | `app-config-fields.md` §1.1 |
| Sharing with other apps | `../SharedData/` only; open by fixed names, never list | `Docs/shared-data.md`; `hybridx-link/docs/ACTIVITY_LINK.md` |

What the platform does **not** give us:
- **No scheduled code.** Nothing runs at 7 am to remind you (`hybridx-link/docs/STORE_RELEASE.md`,
  Streak, "Not claimed": "nothing runs on a schedule on this platform"). The glance is the
  reminder.
- **No writing to `:/Workouts`**, so no feeding UNA's Running (section 2).
- **No VO2max from UNA.** The only VO2max on the watch is X Run's, in
  `../SharedData/HybridX/vo2max.json`, and it is on a different scale from Daniels' (section 10.3).
- **Glances do not click through** to their app (Streak NOTES S4.4).

The consequence: this app is mostly **plan logic plus screens on top of machinery UNA already
built and proved**. The engine, gauges, alerts, step faces and FIT writing are reused, not
rewritten. That is what makes it worth doing despite section 2.

## 5. What the runner enters, and where

### 5.1 On the phone (config fields)

Proposed (decision 14.4). All optional except the 5K; the watch checks every value again,
because the phone's pattern checks are documented but not yet seen working (Race's T23 is still
open: `hybridx-race/docs/ON_WATCH_TESTS.md`).

| id | Type | Label | Rule | Default |
|---|---|---|---|---|
| `plan` | int 0-3 | Plan | 0 none, 1 10K, 2 half marathon, 3 VO2max (config fields have no enum: X Weigh-in's `sex` does the same) | 0 |
| `fiveK` | string ≤ 8 | Recent 5K time | `m:ss` or `mm:ss`, 12:00 to 59:59; empty = not set | "" |
| `halfTime` | string ≤ 8 | Half marathon time (optional) | `h:mm:ss`, 0:55:00 to 3:59:59; empty = none | "" |
| `raceDate` | string ≤ 10 | Race date (optional) | `YYYY-MM-DD`; empty = start next Monday and run the plan's length | "" |
| `level` | int 1-3 | Your running now | 1 up to 20 km (12 mi) a week, 2 about 20-40 km (12-25 mi), 3 over 40 km (25 mi) | 2 |
| `daysPerWeek` | int 3-5 | Runs a week | 3, 4 or 5 | 4 |
| `longRunDay` | int 1-7 | Long run day | 1 Monday to 7 Sunday | 7 |
| `easyByHr` | bool | Easy runs by heart rate | Easy and long runs use the watch's zone 2 instead of a pace band | false |

Eight fields, well under the 32 limit. Units, heart-rate zones and the date come from the watch.

**When a value changes.** The app keeps its own copy of what it last used. A new 5K or half
time is dated the day the app first sees it, and the paces change from the next session; a new
plan, race date or days-a-week starts a new plan after a confirmation on the watch ("Start the
half-marathon plan? Your 10K plan ends").

### 5.2 On the watch

- **A set-up fallback** (Plan → New plan): plan, 5K time (RunLVGL's pickers, `PickerLogic.hpp`),
  race date (a week picker: "in 12 weeks, Sun 7 Feb"), days a week. Written back to the values
  file with `AppConfig::save()`, as Race writes its run length back (Race NOTES 5.22, "Run
  length set on the phone was ignored"), so phone
  and watch agree. This also covers a USB install, which has no values file.
- **Settings:** easy runs by heart rate, alerts on or off, lead-in beeps on or off.
- **In the moment:** skip today, move today to tomorrow, do yesterday's missed long run today
  (once), pause the plan (illness or holiday), end the plan.

## 6. The paces, in brief

Full method, worked examples and open points: `PLANS_DRAFT.md` §2.

1. **Run fitness from the 5K.** Solve the Daniels–Gilbert equations (oxygen cost of a speed,
   and the fraction of VO2max held for a race's duration) for the runner's 5K. These are the
   equations behind Daniels' tables; RACE_PREDICTIONS §4.3 checked they reproduce the
   published race table to within 10 s. A 22:00 5K gives 44.5.
2. **Training paces from the fitness:** easy (a range), threshold, interval and repetition
   paces from fixed fractions of it; marathon, half and 10K paces from the race equation. The
   fractions are provisional until checked against the published tables (TP0).
3. **The half-marathon time, if given,** sets the half's race pace and the long-run paces
   directly, and the gap between the two fitness figures says whether endurance or speed is
   behind ("your half is slower than your 5K suggests: long runs matter most for you").
4. **Bands, not single paces:** each target becomes a speed band for the SDK's `SpeedGauge`
   (proposed ±5 s/km around threshold and race paces, ±6 s/km for intervals, the full range
   for easy; decision 14.6).
5. **No paces from X Run's VO2max** (section 10.3). It is shown, and it can flag a 5K that
   looks out of date.
6. **Train at today's fitness, not the goal.** There is no goal-time field in v1 (decision 14.5).

Example, from a 22:00 5K ([computed], provisional fractions): easy 5:22-5:57 /km, half
marathon 4:48, threshold 4:40, 10K 4:34, interval 4:18, repetition 4:01.

## 7. How the plan becomes today's workout

```
 config fields + on-watch set-up ──► PlanSetup (validated, dated)
                                        │
 5K / half / time-trial results ──► PaceModel ──► Paces (mm/s bands)
                                        │
 PlanTemplates (flash tables) ──► PlanCalendar ──► today's slot: week 6, day "Q1"
   10K · Half · VO2max × level            │   (start, race date, days a week,
                                          │    long-run day, moves, pauses)
                                          ▼
                              Session (e.g. INT 5 × 1000 m, 2:30 jog)
                                          │  PlanBuilder: + paces, + level caps
                                          ▼
                           SDK::Workout::Program (the service's one instance)
                                          │
              RunLVGL workout mode: Engine + SpeedGauge/HeartRateGauge
              + LeadInAlert/ZoneAlerts + workout faces + FitWorkoutWriter
                                          │  on save
                                          ▼
              PlanState: done / part done ─► act_plan.json, plan_today.json,
                                             plan_vo2_*.json, the Streak line
```

- **Calendar.** A plan starts on a Monday (or the runner's week start; decision 14.7). With a
  race date it is counted back from the race; if fewer weeks are left than the plan has, the
  first base weeks are dropped, down to a minimum (10K 6 weeks, half 8); closer than that, the
  app says "Too close to the race for this plan" and offers the last weeks only.
- **Placing the week's sessions** (relative to the long-run day L): rest on L+1, the first
  quality session on L+2, an easy run on L+3, the second quality on L+5, another easy run on
  L+6 (5 days). Never two quality days within 48 hours. Details in `PLANS_DRAFT.md` §4.
- **Building the Program.** Every session is a short recipe (warm-up, a repeat block, recovery
  jogs, cool-down) turned into SDK steps, with **both** the interpreted fields and the raw FIT
  fields filled, because `FitWorkoutWriter` writes from the raw ones
  (`FitWorkoutWriter.hpp`; `buildIntervals` does the same for its open steps). A host test
  writes every session of every plan to FIT and reads it back with `FitWorkoutReader`, and
  requires the same program.
- **The week and plan screens never build Programs.** Their totals come from the recipe
  directly, so there is still only one 15 KB Program in memory.

## 8. Telling the runner: screens, cues and words

All text British English, integer formatting, and checked against the round face by a
text-fit audit, as X Sauna, Ride and Workout do.

### 8.1 Before the run

| Screen | What it shows |
|---|---|
| **Main menu** (RunLVGL's wheel) | **Today** first, its hint the session ("Intervals"), then Free run, This week, Plan, Paces, Workouts (UNA's library, kept from RunLVGL), Settings |
| **Today** | Day and week ("TUE · WEEK 6 OF 10"), the session name large ("INTERVALS"), one line of what ("5 × 1 km"), the target ("@ 4:12-4:24 /km"), the totals ("11.0 km · 56 min"), and the purpose ("Builds your VO2max"). One press starts it; the steps are a scroll away (RunLVGL's details screen) |
| **Rest day** | "Rest day" and the next session ("Fri: Tempo 25 min"). Free run is still on the menu |
| **Missed yesterday** | Only for a missed long run, and only when today is easy or rest: "Do yesterday's long run today?" (yes swaps the two; offered once) |
| **This week** | Seven rows, Monday first: the day, the session's short name, and a mark: done (green tick), part done (amber), missed (grey cross), today (white dot), to come (outline). Choosing a row shows that day's steps |
| **Plan** | Name, "Week 6 of 10 · Build", a bar of weeks with this one lit, sessions done ("19 of 20"), "Race in 33 days · Sun 27 Dec", and X Run's VO2max with its change since the plan began |
| **Paces** | E, M, HM, T, 10K, I, R paces, then their source: "From your 5K of 22:00, 12 Nov" |

### 8.2 During the run

RunLVGL's workout mode, unchanged in behaviour:
- **Countdown, then the first step card** ("WARM UP · 15 min · easy").
- **Gauge face:** the live pace on a three-part arc (slow, in range, fast) with a pointer, and
  what is left of the step under it ("620 m to go" or "1:45 left").
- **Step face:** the step's target, the lap's average so far (each step is a lap), and "Rep 3
  of 5".
- **Step card** for 3 s at every new step, with the same alert whether the step ran out or
  you pressed lap (RunLVGL `Service.cpp`, `onWorkoutStepEnded`): "RUN 3/5 · 1000 m ·
  4:12-4:24".
- **Lead-in:** a beep each second over the last 5 s of a time or distance step.
- **Zone alerts:** too slow is long and falling, too fast short and rising, back in range a
  rising triad, each played twice; again every 30 s while out of the band. None in the first
  20 s of a step (`ZoneAlerts::kQuietStartSec`). The pitch differs only on new enough
  firmware, and UNA chose rhythms that tell them apart without it (`playZoneAlert`).
- **Short reps** (strides, 200-400 m repetitions, 30/30s) end before alerts are allowed, so they
  show their target and are judged at the end instead: the next recovery's step card reads
  "Last rep 1:36 · target 1:33-1:40" (proposed; decision 14.6).
- The usual X Run faces (time, distance, heart rate, laps) stay one button away.

### 8.3 After the run

The summary, then the Saved screen with:
- the session's verdict: "Session done" (reached the last step), "Part done" (saved, but
  ended early), or nothing for a free run;
- the week: "Week 6: 1 of 4 done";
- the next session: "Next: Wed · Easy run 45 min";
- the Streak line ("3 of 4 this week · 8-week streak") and the training load ("Load 64"), as
  every family app.

After a **5K time trial**: "5K 21:41 · new paces from your next run" (or, if slower: "5K 22:30:
paces kept. Keep them, or update?"; decision 14.8).

## 9. Tracking runs and the plan's memory

- **Done** means the engine reached the end of the session (the SDK's `reachedEnd`, read before
  `next()` when the runner ends it, as `WorkoutEngine.hpp` warns) **and** the activity saved.
  **Part done**: saved, at least half the planned time, not reached the end. **Missed**: the
  day passed with neither. A free run on a planned day does not count unless the runner says
  so on the Saved screen ("Count as today's easy run?", easy and long runs only).
- **Never stack.** A missed session is dropped, not added to another day. The one exception is
  the missed long run in 8.1.
- **Missed weeks** (draft rules, for Jon; `PLANS_DRAFT.md` §4.4): a week under half done
  repeats (taken out of the base weeks, never the taper); a week with nothing done comes back
  as its cut-back version; a pause of 8-14 days repeats the last completed week; over 14 days
  asks for a 5K time trial before quality sessions return.
- **Where it lives.** `plan.json` in the app's folder (the family's crash-safe `SafeFile`):
  the set-up, the fitness and its source and date, and a log of at most 84 entries (12 weeks ×
  7 days: date, session, status, start UTC). About 2 KB, integers only (the link's rule:
  the SDK's JSON writer misprints signed values on the host).
- **What is not tracked:** whether the run was really at the pace asked. The lap averages are
  in the FIT file and on the step face; the plan does not grade them in v1.

## 10. The family

### 10.1 Streak and training load

- A **new slot** in the activity link, `plan` → `../SharedData/HybridX/act_plan.json`, sport 1
  (run). The link has room (5 of `kMaxSlots` 8 used), but adding a slot means a **Streak
  release** (ACTIVITY_LINK.md, "Adding an app", step 1).
- Training load: Edwards' TRIMP from the watch's zones, the 8th number, as Run scores it.
- The Streak line on the Saved screen (`Service::sendStreakLine()`).
- Streak keys sessions by the app's folder, `HybridXPlan`, so a plan run is never counted twice.

### 10.2 X Run's VO2max, from plan runs

Without this, someone who runs only with X Plan would see X Run's VO2max go stale (it counts
runs within 42 days of the latest; Run NOTES "VO2max switched on"). Trail already solves it:
it leaves each run's steady minutes in `trail_vo2_0..2.json` and Run counts them when it
opens (Trail NOTES "Run link"). X Plan would do the same with `plan_vo2_0..2.json`, using a
copy of Run's window code (`Libs/Vo2Link`, checked by CI), which needs a **Run release** to
read the new files. Easy and long runs give most of the steady minutes; intervals few.

### 10.3 X Run's VO2max, shown in the plan

- **Shown** on the Plan screen, and as the VO2max plan's second measure ("VO2max 45.1, +0.6
  since week 1"), read from `vo2max.json`.
- **Not used for paces.** It comes from the ACSM running equation, which puts 21% more oxygen
  cost on 5:00 /km than Daniels' curve; fed into Daniels' tables it gives times far too fast
  (RACE_PREDICTIONS §4.3: a 3:28 marathon instead of 4:00 for the same runner). The VO2max
  plan's **main** measure is therefore its 5K time trials, which are on the paces' own scale.
- **A cross-check, later** (decision 14.10): convert it onto Daniels' scale (§4.3's two-step
  conversion) and, if it is well above the 5K's fitness, suggest a time trial ("Your 5K looks
  old: run a time trial?"). Not before X Run's Garmin comparison (Run Gate R) says how far it
  can be trusted.

## 11. Architecture

The house pattern (Run R0, Trail T2, Intervals P3a): **copy RunLVGL verbatim at the pin first**,
in its own commit, so every change is a readable diff against UNA's original; then add.
RunLVGL rather than a copy of X Run, because X Run's VO2max engine stays X Run's (10.2), but
each addition below is ported from X Run, which already has it.

```
hybridx-plan/
  docs/                 this brief, PLANS_DRAFT.md, NOTES.md (from TP0), screens/, experiments/
  Software/
    Apps/HybridXPlan-CMake/       the watch build (APP_TYPE Activity)
    Apps/HybridXPlanGlance-CMake/ the glance (TP6)
    Apps/LVGL-GUI/                RunLVGL's GUI + the plan screens
    Libs/Core/                    pure C++, host-tested, header-only where the GUI uses it
    Libs/App/                     RunLVGL's service + the plan
    Libs/Link/                    copy of hybridx-link (CI-checked)
    Libs/Vo2Link/                 copy of Run's window code (CI-checked), TP4
    Libs/Glance/                  TP6
  Tests/Host/
  Tools/                          plan_print (TP1), plan_sim (TP5)
```

**Core** (no SDK headers except `WorkoutProgram.hpp`, a plain struct, and the JSON reader and
writer, as Run's core):

| Module | Does | Tests |
|---|---|---|
| `PaceModel` | Fitness from a race; paces and bands; race predictions; the endurance gap | Fixtures from the published race table (VDOT 30-70, ±10 s, as §4.3 did) and the training-pace table (TP0) |
| `PlanTemplates` (`Header/data/`) | The three plans as `constexpr` tables: week, phase, slot, session recipe, per level | Every week has its long run; no two quality days within 48 h at 3, 4 and 5 days; cut-back weeks lower than the weeks either side |
| `PlanCalendar` | Dates (`Ymd`, as the SDK's), weekdays, plan start from a race date, the truncation rule, moves and pauses | Month and year ends, leap days, a race date in the past or too close |
| `PlanBuilder` | Session + paces + level → `SDK::Workout::Program`, raw FIT fields included; the quality caps | Every session of every plan, level and fitness 25-70 builds a valid program within the SDK's limits, and survives a FIT write and read unchanged |
| `PlanState` | The log and its JSON, bounded; done / part done / missed; the rules of section 9 | Corrupt and truncated files read as "no plan", every prefix and one-byte change (Intervals' parser tests) |
| `PlanText` | Session names, purposes, the Today and Saved lines, the glance lines | Worst case widths against the real fonts (the text-fit audit) |
| `TodayFile` | The glance's file (section 12) | Round trip; a file from the future or the past |

**Service** (RunLVGL's, plus):
- at GUI start: read the config and `plan.json`, settle any missed days, build today's
  Program into the existing `mProgram`, send `PlanToday` and `PlanWeek` (fixed-size message
  structs; no pointers to data that can change);
- start: RunLVGL's `startWorkout` path with a "planned" flag and the session id;
- each second: unchanged (the engine, gauges and alerts already run there);
- save: record the result in `plan.json`; then the activity link, the VO2 windows, the today
  file and the Streak line, each a warning if it fails and never in the way of the save
  (the link's rule 4);
- message IDs from 0x20 up, clear of RunLVGL's (Run R6a moved its own after a collision).

**GUI:** RunLVGL's screens plus Today, This week, Plan, Paces, New plan, and the Saved
screen's extra lines. Screens build only what is on show and free before building
(RunLVGL's own pool crash, fixed by Intervals P3b.3, Run R2, Trail T3.2).

**Memory.** One Program (15 KB) as RunLVGL already has; templates in flash (estimate under
4 KB); `plan.json` under 2 KB, read into a heap buffer at open and freed; no allocation in
the 1 Hz loop. The LVGL pool stays at RunLVGL's 56 KB. The `.uapp` should land near X Run's
517 KB; the watch's limit for an app is not documented (question 15.3).

**Builds:** CI builds the app (and later the glance) into `watch-apps`, tags `plan-vX.Y.Z`,
and checks the link and Vo2Link copies. Local builds are compile checks only.

## 12. The glance (phase TP6)

- A second `.uapp`, `HybridXPlanGlance`, as Run's VO2max glance and Streak's: display only,
  240 × 60, 16 colours, drawn on `EVENT_GLANCE_START` and the tick.
- It reads `../SharedData/HybridX/plan_today.json`, which the app writes when it opens, after
  every save and whenever the plan changes. It holds **the next 14 days**, so the glance stays
  right for a fortnight without the app being opened (it cannot run on a schedule):
  `{"v":1,"plan":1,"wk":6,"wks":10,"race":20261227,"d":[[20261124,3,0,56,11000,"5 x 1 km"],...]}`
  (date, session kind, status, minutes, metres, a short label; the exact format is TP6's).
- It works out today from the watch's date:

  | State | Line 1 | Line 2 | Line 3 |
  |---|---|---|---|
  | A session today | TODAY · INTERVALS | 5 × 1 km @ 4:18 | Week 6 of 10 · Race in 33 days |
  | Done today | DONE · INTERVALS | Next: Wed · Easy 45 min | Week 6 of 10 · 1 of 4 done |
  | Rest day | REST DAY | Tomorrow: Tempo 25 min | Week 6 of 10 · 2 of 4 done |
  | File older than 14 days | X PLAN | Open X Plan to update | |
  | No plan | X PLAN | Set up a plan in the UNA app | |

  Compact and tiny layouts for smaller areas, as the other glances.
- Preview by `glance_preview.py` from the real layout code (the simulator cannot run glances).

## 13. Phases and gates

Each phase starts in plan mode and stops at its gate with what was done, how it was checked,
and what Jon must test or decide. Phases are **TP0-TP7** (TP for training plan; P, R and W
are taken).

| Phase | Work | Gate |
|---|---|---|
| **TP0** | Decisions (section 14). Check the training-pace fractions and the quality limits against Daniels' Running Formula (Jon's copy, or a session that can reach the sources). Ask UNA (section 15). Start `NOTES.md`. No code | Jon signs off the pace model, the placement rules and the three plans' shape |
| **TP1** | The pure core (section 11) with host tests, and `Tools/plan_print`: a whole plan for any input as Markdown, with every session's steps, paces and weekly totals | Jon reads `plan_print` output for three runners (5K 20:00 level 3 five days; 25:00 level 2 four days; 32:00 level 1 three days) in each plan, and corrects the content |
| **TP2** | The app: RunLVGL verbatim, then identity, icon, CI; the config fields; today's Program built and run through RunLVGL's workout mode; `plan.json` written at save. `docs/experiments/sim_plan.sh` runs a planned session end to end; `fit_check.sh` decodes the FIT (workout steps with their speed targets, a lap per step, the session) | Jon installs the CI build, sets a plan by USB values file, and runs one real session |
| **TP3** | The screens (section 8): Today, This week, Plan, Paces, New plan, the Saved lines, the short-rep result. Text-fit audit; `capture_screens.sh` | Jon reviews the captures, then a week of plan runs on the watch |
| **TP4** | Family: the `plan` slot (with a Streak release), training load, the Streak line; the Run link (with a Run release); X Run's VO2max on the Plan screen. The family check gains X Plan | The family check passes; Jon's watch test (FAMILY_WATCH_TEST.md gains X Plan) |
| **TP5** | The plan's memory and recalibration (section 9): missed sessions, moves, pauses, repeat and cut-back weeks, time trials, a faster 5K found in a run. `Tools/plan_sim` replays months of made-up compliance and prints what the plan did | Jon reviews `plan_sim` cases and the rules |
| **TP6** | The glance and `plan_today.json` | Glance previews; then on the watch |
| **TP7** | Field test: Jon (and ideally two runners of other levels) follow a plan for at least three weeks. Then the store release (packager, previews, the brand colourway, the portal App ID), as the family's phase P | Jon's go for the store |

TP0 and TP1 are independent of decision 14.1; TP2 onwards is where the overlap with UNA
matters.

## 14. Decisions for Jon

1. **Go, or ask UNA first** (section 2). Recommendation: ask; do TP0-TP1 meanwhile.
2. **The name.** "X Plan" (6 bytes; folder `HybridXPlan`)? Alternatives: "X Coach", "X Train".
3. **A new app, or a mode of X Run?** Recommended: a new app, as asked. A mode inside X Run
   would avoid a new Streak slot, the Run link and a third RunLVGL copy, but makes X Run larger
   and couples their releases.
4. **The phone fields** (5.1): this set? A `level` of 1-3, or a weekly distance in km?
5. **A goal time?** Recommended: none in v1; train at current fitness, update it by time
   trials. (A goal could be shown, never used for paces.)
6. **Bands and short reps:** the band widths in section 6, and "judge short reps at the end"
   (8.2).
7. **The week:** starts Monday, or follows Streak's week-start setting?
8. **Recalibration:** automatic when faster (and how much at once), always asked when slower?
   A faster 5K found inside a run: offered, or ignored?
9. **Treadmill:** outdoors only in v1? (X Run's treadmill speed comes from cadence; pace bands
   on it would be shaky. Time-based targets would work.)
10. **The VO2max cross-check** (10.3): wanted, after Run's Gate R?
11. **The plans' content** (`PLANS_DRAFT.md`): the session mix, lengths and levels, the
    quality limits (§3.3 there, and the clash it shows at lower mileage), the long-run cap at
    3-4 runs a week, warm-ups with or without a pace target.
12. **Keep UNA's Workouts library and RunLVGL's Intervals** on the menu? Recommended: keep
    Workouts (it costs nothing and lets a coach's USB files run), drop Intervals.

## 15. Questions for UNA

1. Are training plans, or phone-delivered schedules (`:/Workouts/Schedule/`), planned for
   Running? If so, will third-party apps or services be able to supply them?
2. Does the UNA app show string config fields with a pattern, and does it enforce the pattern?
   (Race's T23 on a store install would answer part of it.)
3. Is there a size limit for an app's `.uapp`, or for its RAM?
4. Could a glance one day open its own app (Streak NOTES S4.4)?

## 16. Risks

| Risk | Effect | Mitigation |
|---|---|---|
| UNA ships plans in Running | The app duplicates theirs, as Intervals did | Decision 14.1; the differentiators in section 2 |
| Phone fields not shown or not checked | No way to set up a plan from the phone | The watch set-up (5.2); the watch validates everything |
| Training-pace fractions not as published | Paces off by a few s/km | TP0 check; fixture tests against the published table |
| GPS on short reps | Pace for a 200-400 m rep is unreliable live | No live alerts there (the SDK's quiet start makes it so); judged by rep time at the end |
| Advice that hurts someone | Injury from too much too soon | Conservative progression, cut-back weeks, never stacking missed sessions, a store text that says it is not medical advice and to stop with pain |
| Content copied from a published plan | Copyright | Original plans built from principles (`PLANS_DRAFT.md` §1); equations, not tables, in the app; the name "VDOT" not used (it is a brand) |
| Size and memory | The app does not fit or the GUI runs out of pool | One Program; templates in flash; screens built on show; watch builds from CI each phase |
| The watch's date unset or wrong | Wrong day's workout | Today shown only with a valid date; otherwise the plan without "today" |

## 17. Sources

- Daniels J, Gilbert J. *Oxygen Power: Performance Tables for Distance Runners*. 1979. The
  oxygen-cost and fraction-held equations, as quoted and checked in
  `hybridx-run/docs/RACE_PREDICTIONS.md` §4.3.
- Daniels J. *Daniels' Running Formula*, 3rd ed. Human Kinetics, 2014. Training intensities and
  quality limits: **from secondary sources and memory here; to be read in the book at TP0**.
- The SDK at `sdk-v1.6.2`: `Libs/Header/SDK/Workout/*`, `Libs/Header/SDK/Fit/FitWorkoutWriter.hpp`,
  `FitProfile.hpp`, `Docs/app-config-fields.md`, `Docs/shared-data.md`, RunLVGL.
- The family's notes: Race NOTES 5.21-5.28 and 5.34, Intervals NOTES, Run NOTES and
  RACE_PREDICTIONS.md, Streak NOTES S4.4, `hybridx-link/docs/ACTIVITY_LINK.md`, Trail NOTES
  "Run link".
- Training-science sources for the plans: `PLANS_DRAFT.md` §9.
