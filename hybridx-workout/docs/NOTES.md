# HybridX Workout: notes

Gym and HIIT sessions on the UNA Watch that count in HybridX Streak. It builds on
the SDK's Workout example and adds the HybridX family hooks: the activity link and
the Streak line. Independent of the other apps.

## Why this app (28 October 2026)

Jon's request: "replicating and building on the SDK built-in Workout app to see what
we can add to the functionality there, and also integrate the streak count into
Workout". From his answers (28 October 2026):
- **Sessions:** strength and gym sets, HIIT timers, Hyrox-style circuits.
- **Additions:** training load now; rep counting later (X Velocity's counter).
- **Toolkit:** LVGL, from our own scaffold.

## Phases

| Phase | Work | Status |
|---|---|---|
| **W0** | Scaffold: the SDK example's plain workout (laps, zones, calories, auto lap) on X Sauna's base, with the strap, `act_workout.json` and the Streak line | **Done (28 October 2026)**, simulator-checked. Gate: Jon's watch test (below) |
| W1 | The session core, pure and host-tested: strength sets with rest; HIIT as EMOM, AMRAP, Tabata (8 x 20/10) and custom; Hyrox-style circuits from Race's stations | **Done (1 November 2026)**, host-tested. Gate: Jon agreed the rules |
| W2 | Service and FIT per type: sport and sub_sport (strength: training 10 / strength_training 20; HIIT: hiit 62 / hiit 70, amrap 73, emom 74, tabata 75; circuit: training 10 / cardio_training 26 or generic), FIT `set` messages as the rep hook; link sports Strength, Workout, Hybrid | **Done (1 November 2026)**, simulator- and decode-checked. Gate: the files decode; Jon's watch test after W3 |
| W3 | Screens: the type and its settings, live faces per type, the rest countdown, the summary | **Done (1 November 2026)**, simulator-checked. Gate: Jon reviews the captures (`docs/screens/w3-*.png`) |
| W4 | Training load (Edwards' TRIMP on the watch's zones, a 7-day total); the rep-counter hook documented | |

The FIT numbers above were read from Garmin's profile in fitdecode when the plan was
made; W2 decodes each back before relying on it.

## Decisions

- **Name:** HybridX Workout. Folder `hybridx-workout/`, `HybridX Workout` on the watch
  (15 bytes, exactly `app_merging.py`'s limit), `HybridXWorkout` as the build, file and
  app-folder name, tags `workout-vX.Y.Z`. Development APP_ID `A294992CFC56DBE9`, the
  first 16 hex of md5("HybridXWorkout"), as Sauna's and Knitting's were made; the portal
  issues the real one at release.
- **Base: X Sauna, not Race** (a change from the approved outline, which said Race; in the
  W0 plan, approved by Jon on 28 October 2026). Sauna has the current layout (`Libs/Core`,
  `Libs/App`, GUI, tests), a FIT writer proven on a watch with the dropped-seconds fix
  (`SecondGate.hpp`), screens that hold the LVGL pool down by building only what is on show,
  the text-fit audit and a key-driven capture. Race gave what Sauna lacks: the strap request,
  `writeActivityLink`, `sendStreakLine` and the Saved screen's Streak line. `SafeFile` is
  Race's, copied.
- **What is the SDK example's** (`una-sdk/Examples/Apps/Workout`), replicated:
  - **zones** on the watch's own thresholds (`RequestSystemSettings`; defaults 95, 114, 133,
    152, 171, 190; missing ones filled 20 bpm apart), with its rule: above threshold i is
    zone i + 1, capped at 5 (`Core/Header/HrZones.hpp`);
  - **calories:** each active second, MET x weight / 3600 kcal, with the example's MET per
    zone (1.0 below zone 1 or with no heart rate, then 2.5, 4.5, 7, 10, 12.5), and the
    resting part (1 MET) kept apart. The profile's weight, or the example's 75 kg. Integer
    here: MET in tenths, weight in grams, time in ms;
  - **auto lap** (its "time alert"): Off, 1, 5, 10, 15, 20 or 30 minutes of active lap time;
  - **the file:** sport and sub_sport generic, total calories on each lap and the session,
    `metabolic_calories` (FitProfile.hpp's field 196) on the session, the lap's resting
    calories as the example's developer field `resting_calories` (kcal), and the sidecar's
    `activity_type` "workout". Laps carry `lap_trigger` manual (0) or time (1) for an auto
    lap (public profile, checked in fitdecode). `sport_profile_name` "Workout" (Race 5.28's
    experiment; Sauna found Strava ignores it).
  - **Its zone colours** (`SummaryFaceZones.cpp`): cyan, green, yellow, orange, red.
- **Not replicated:** the example's phone-notifications switch (not an app feature we
  need), and its TouchGFX screens (we use LVGL).
- **Kept from Sauna:** R2 laps, R1 opens the action menu (Resume, Pause, End workout,
  Discard; End and Discard held 1.5 s), L1/L2 turn the faces; a 3 s lap lockout against a
  double tap (an assumption: no set, round or station is that short); the service keeps
  recording if the screen goes, and saves after 5 minutes away; crash recovery of the
  records; laps and the session written at stop. Sauna's `completed` flag (a session banked
  when the app was stopped) is not in the file; the log says so.
- **The strap:** asked for when the screen opens (`RequestPrepare(HRM)`, as Race), released
  when the app leaves with no workout running. It only connects if external HR is switched
  on in the watch's settings. Start's hint shows where the heart rate comes from: "Strap
  connected", "Finding strap", "Strap lost", or "Wrist heart rate".
- **Streak:** after a save, the workout goes into `act_workout.json` as `Sport::Workout`
  (Streak's Workout kind), then the Streak line. W2 moves strength to `Sport::Strength` and
  circuits to `Sport::Hybrid`. No change to Streak: its reader already had the Workout slot.
- **Settings** (the auto lap) are remembered in `settings.json` in the app's folder
  (`{"v":1,"autolap":5}`, through `SafeFile`).
- **Lap names on the summary:** "Lap 3", or "Auto 3" for a lap the auto lap ended ("Lap 3
  auto" does not fit beside its time).

## W0: the scaffold (28 October 2026)

### What it does

- **Main menu:** Start (hint: the heart-rate source) and Auto lap (R1 steps through the
  choices; remembered). R2 exits; exits on 30 s idle.
- **Live:** a ring in the current heart-rate zone's colour (grey below zone 1 or paused),
  four faces: main (lap number, lap time large, total, heart rate, zone); totals (time,
  kcal, laps, auto lap); zones (time in each, zone 5 at the top); clock (time of day,
  battery). The lap toast: the new lap ("Auto lap" when the auto lap ended it), the one
  just finished with its time and average heart rate. A lap buzzes once.
- **Saved:** the Streak line in amber ("3 of 4 this week", "7-week streak") for 3.5 s when
  Streak has left its summary, else "It will sync to the UNA app"; then the summary:
  overview (time, laps, kcal), zones, heart rate and calories, laps three to a page.

### Files

- Core: `WorkoutModel.hpp` (Sauna's `SessionModel` without phases, temperature and
  recovery, plus zones, calories and the auto lap), `HrZones.hpp`, `WorkoutFit.hpp`,
  `WorkoutText.hpp`, `WorkoutSnapshot.hpp`, `WorkoutSettings.{hpp,cpp}`; Sauna's
  `LapTiming`, `SecondGate`, `HoldTimer`, `LapLockout` unchanged.
- App: `Service`, `ActivityWriter` (Sauna's, trimmed to the example's fields), `SafeFile`
  (Race's), `Commands.hpp` (Sauna's messages, plus `SettingsUpdate`, `SetAutoLap`,
  `AccessoryStatusUpd`, `StreakLineUpd`; all static_asserted under 256 bytes).
- `Software/Libs/Link`: the activity link, a copy (`check-link-copies.sh`).
- Icons: a placeholder dumbbell (`Resources/make_icons.py`).

### W0 verified (28 October 2026)

- **Host tests:** 74 pass, also under ASan and UBSan. Calories against hand-worked figures
  (80 kg, a minute in zone 3 and a minute below zone 1: 10.7 kcal, 2.7 resting; an hour in
  each zone at 75 kg), the example's zone rule on its default thresholds, time going to the
  zone known when it began, the auto lap (pauses not counted, a button lap restarting it,
  a full workout), the settings file.
- **The FIT file from the real writer** (`fit_check.sh`, four scenarios, decoded by
  fitdecode): full (four laps and a pause), autolap (laps of 60, 60 and 30 s, triggers time,
  time, manual), nohr (every second resting), crash (406 of 415 records recovered, no laps).
  In `full`, the session's 51 kcal and 9 metabolic kcal match the figures worked out again
  in Python from the records alone.
- **Every screen in the simulator** (`capture_screens.sh`, 25 frames in
  `docs/screens/w0-*.png`): the menu and the auto lap, the four faces, a lap and an auto lap
  with their toasts, pause and resume, End by holding R1, Saved with the Streak line, the
  summary pages, then a second workout discarded. Its file passes the checks above (lap
  triggers manual, time, manual); `act_workout.json` holds one entry with the `.fit`'s start
  and timer time and sport 5; the Streak line read "3 of 4 this week" and "7-week streak"
  from a pretend summary of 2 of 4 and a 7-week streak.
- **Text-fit audit** (`audit.sh`): 16 cases, 0 problems. It found three on the way: the
  calories on the totals face (now capped at 99999, as the clock is at 99:59:59), the lap
  toast's auto line, and the summary's lap rows (and that its laps check had been looking
  at an empty page; it now checks laps 61 to 63).
- **Family check** (`hybridx-link/docs/experiments/family_check.sh`, now four apps): Run,
  Race, Trail and Workout saved one after another; their lines read 1, 2, 3 and 4 of 7;
  Streak found four new and counted the workout as Workout; the second open found none new.
- **Watch target, local compile check:** both processes link, `HybridXWorkout_0.0.0-dev.uapp`
  318,040 bytes, no warnings from our code. Not installable: CI makes the one to install.
- **Not verified:** anything on a watch. In particular: the strap states on screen, the
  profile's weight and zones arriving (the simulator gives 75 kg and the defaults), calories
  as the UNA app shows them, how the UNA app and Strava title a generic workout, the auto
  lap's buzz, and the LVGL pool on the watch.

### For Jon at Gate W0

1. **Install** `HybridXWorkout_<version>.uapp` from the CI run's `watch-apps` into a new
   folder `Apps/HybridXWorkout`, then restart the watch.
2. **Open Streak once** if it has not been opened this week, so it leaves its summary.
3. **A short workout:** Start, a lap or two with R2, set Auto lap to 1 min first if you want
   to see one, pause and resume from the R1 menu, then End by holding R1.
4. **Look at:** the Saved screen's Streak line; the summary pages; the UNA app's activity
   (title, calories, laps, heart rate); then open Streak: "+1 Workout", counted once.
5. **Tell me:** whether Start said "Strap connected" with your strap on (external HR on in
   Settings), and whether the calories look like the built-in Workout's for a similar session.

## The family draft: the plan and its baseline (1 November 2026)

Jon asked (1 November 2026) for the activity apps that count in Streak to be built as an
installable draft family, even with today's limits, with a plan for building and testing
them. Decisions: **X Sauna stays out of the link** (his 27 October decision, confirmed);
**Workout is finished through W1 to W3** (strength sets, HIIT timers, Hyrox-style circuits,
their FIT output and screens) first; W4 (training load) is not part of the draft. No tags
and no portal uploads: the three development App IDs (Ride, Workout, Sauna) are still
development ones, and those steps are Jon's.

Phases: **A** baseline (this section); **B** W1, the session core; **C** W2, service and FIT
per type; **D** W3, the screens; **E** the family check extended and CI; **F** the family
watch-test script (`hybridx-link/docs/FAMILY_WATCH_TEST.md`). Each ends at a gate.

### Phase A baseline (1 November 2026, HEAD `bd7bea1`)

- **Host tests, all passing:** Link 40, Race 145, Streak 136, Trail 149, Run 200, Ride 94,
  Workout 74, Sauna 115.
- **Simulators:** Run, Race, Trail, Workout, Ride and Streak all build (Ninja, Debug) with
  exit 0.
- **Family check** (`family_check.sh`, quick mode, five writers): every line PASS. Each
  summary matches its `.fit` (start and duration). The Streak lines read 1 to 5 of 7. Streak
  found five new on the first open and none on the second, and counted Run, Hybrid, Run
  (Trail), Workout and Ride once each.
- **Not run here:** the watch targets (CI's job) and the `LONG=1` family check.
- **Container setup needed:** `apt-get install libsdl2-dev xdotool` and `pip install
  fitdecode` (a fresh container has none of them).

## W1: the session core (1 November 2026)

Jon agreed the rules at the end of Phase A ("Yes looks great"), as proposed:

| Type | The rule |
|---|---|
| Strength | Sets (default 3) of open work, each followed by a timed rest (default 60 s), except the last. The button ends a set (the rest begins) or skips a rest. A set ends by itself at its cap (default 5 min). |
| EMOM | N timed minutes (default 10). A cue at each minute and a warning 3 s before it. |
| AMRAP | One timed step, the cap (default 12 min). The button counts a round, and each round is a lap. |
| Tabata | 8 rounds of 20 s work and 10 s rest, fixed. |
| Custom | Work, rest and rounds (default 40 s, 20 s, 6). |
| Circuit | Rounds (default 8) of a run (default 1 km) then a station, in Race's order. The button moves to the next segment. |

### What was built

- **`SessionPlan.hpp`** (header-only; the GUI will use it): `SessionType`, `StepKind`, `PlanConfig`
  with each setting's range and default, `clamp()`, and `stepCount()` / `stepAt()`, which work
  each step out from the numbers, so there is no step array to hold or overflow. The ranges
  keep every plan within the model's 64 laps (the largest is EMOM at 60 minutes;
  `static_assert`s check each type). `kCircuitStations` is a **copy** of Race's `kStations`
  (`hybridx-race/Software/Libs/Header/RaceData.hpp`), so the apps stay independent at build
  time; a host test includes Race's header and checks the two match.
- **`PlanRunner.hpp`** (header-only, but the **service** runs it, so the timers keep going if
  the screen closes): `start()` starts the model and the plan, `press()` is the lap button,
  `tick()` follows each sample, and each returns a `Cue` (Work, Rest, Run, Station, Warning,
  Round, Done) for the service to buzz. `view()` gives the screens a `PlanView`.
- **`WorkoutModel`**: each lap now carries its step (`kind`, `round`, `station`), and
  `lapTimed()` ends a lap by time (`autoEnded`, so the FIT lap trigger is time as for the auto
  lap). 48 bytes a lap; the model is about 3.1 KB, still in static storage.
- **`WorkoutSettings`** version 2 holds the type and every number (`kMaxJsonBytes` 256, on the
  10 KB service stack as before). A version 1 file still reads: its auto lap is kept and the
  plan takes the defaults. A missing key keeps its default; every value is clamped.

### Choices made while building (for Jon to overrule)

- **The plan's clock is the active time,** so a pause stops it.
- **A timed step's end is anchored to its planned time, not to the tick that saw it.** A tick
  up to a second late makes that lap up to a second long and the next as much short, so the
  plan never drifts: ten EMOM minutes end at 10:00 of active time to within a tick.
- **The timers (EMOM, Tabata, custom) ignore the button.** Their clock is the point; a press
  does nothing (the screen will say so in W3).
- **At the end of a plan the workout pauses itself** and the last lap is the last step. The
  service will offer End and Discard, not Resume (W2), so nothing saves by surprise and no
  empty lap is added.
- **No rest after the last set or interval**, so Tabata is 3:50, not 4:00.
- **AMRAP's last lap is the part round** cut off by the cap; the rounds counted are the
  presses.
- **A plan switches the auto lap off**; it stays for a plain workout.
- **No warning before a strength set's cap**, only before timed steps longer than 3 s.
- **Undo is not in W1.** W0 has none, and nothing in the rules needs it.

### W1 verified (1 November 2026)

- **Host tests: 101** (74 before, 27 new: `SessionPlanTest` 11, `PlanRunnerTest` 12 and 4 more
  in `WorkoutSettingsTest`), also under ASan and UBSan. They cover every type's steps and
  cues, starts on either side of the 32-bit clock wrap, a pause in a timed step, a tick every
  1.3 s (no drift), a gap that crosses three steps, a strength set ending at its cap, the
  circuit's stations against Race's, clamping, the v1 file, and a second workout after a plan.
- **The simulator still builds** (the model's lap struct changed). Nothing uses the plan yet:
  that is W2 (the service and the file) and W3 (the screens).
- **Not checked:** anything on a watch.

## W2: the service and the file per kind of workout (1 November 2026)

### What the service does now

- **It runs the plan.** `startSession` starts the `PlanRunner` (which starts the model) with the
  remembered plan. Each second's tick, after the sample, ticks the runner; R2 goes to the runner
  when there is a plan (`press`), and to the model as before when there is not. The runner's cue
  comes back to `onCue`, which buzzes, lights the screen and sends the lap toast.
- **Each cue has its own buzz** (`buzzCue`; vibration only, so it works on any firmware):

  | Cue | Pattern |
  |---|---|
  | Work begins (set, minute, interval) | long double sharp click, strong |
  | Rest begins | short double click, medium |
  | Run begins (circuit), AMRAP round, plain lap | strong click |
  | Station begins (circuit) | double click |
  | 3 s left in a timed step | three sharp ticks a second apart (3, 2, 1) |
  | Plan done | 1 s alert |

  Whether they are distinct enough on the wrist is for Jon at the watch test.
- **The end of a plan.** The runner pauses the model; the service writes the FIT timer stop then,
  so the file's timer stops when the plan does. **Resume is refused** from then on (logged), so
  End and Discard are what remain. The screen will say so in W3.
- **Settings.** `SetPlan` (GUI to service, new) sets the next workout's kind and numbers; the
  service clamps and saves them. `SettingsUpdate` now carries the plan too. A workout already
  running keeps the plan it started with: the file, the summary and the link all read
  `sRunner.config()`, not the settings.
- **To the screens** (for W3): the snapshot carries a `PlanView` (step, round, station, time left);
  the lap toast says what began (`cue`) and what the lap was (kind, round, station); the summary's
  rows carry the same; `SavedSession` has the type, whether the plan ran to its end, and AMRAP's
  rounds. Every message is still `static_assert`ed under 256 bytes.

### The file

| Kind | sport / sub_sport | Profile name | Link sport |
|---|---|---|---|
| Plain | generic 0 / generic 0 | Workout | Workout (5) |
| Strength | training 10 / strength_training 20 | Strength | Strength (4) |
| EMOM | hiit 62 / emom 74 | EMOM | Workout (5) |
| AMRAP | hiit 62 / amrap 73 | AMRAP | Workout (5) |
| Tabata | hiit 62 / tabata 75 | Tabata | Workout (5) |
| Custom | hiit 62 / hiit 70 | Intervals | Workout (5) |
| Circuit | training 10 / cardio_training 26 | Circuit | Hybrid (6) |

- **Where the numbers come from.** `SDK/Fit/FitProfile.hpp` declares only generic and training of
  these. The rest are the public FIT profile's, read from the tables fitdecode 0.11.0 generates
  from Garmin's FIT SDK, the way Race 5.11 sourced its sub-sports; `fit_check.py` decodes each
  back by name ("hiit", "emom", ...). **To log for UNA** with Race 5.11's list: FitProfile.hpp
  lacks sport hiit (62), these six sub-sports, and the set message.
- **Lap intensity** (FitProfile.hpp's lap field 23, declared by the SDK): rest for a plan's rest
  laps, active for the rest. **Lap trigger** time for a step that ended by itself (a timed step,
  a capped set, the last step of a timed plan), manual for the button.
- **Set messages** (strength only): message 225 with timestamp (254: the profile's own number for
  this message), duration, set_type (active 1, rest 0), start_time and message_index, one per lap,
  each starting and ending with its lap. From fitdecode's profile, proven by decoding. **No
  repetitions:** nothing counts them yet (X Velocity's counter, later).
- **A fix on the way:** ending a workout that was paused wrote a second timer stop (stop, stop),
  which breaks FIT's start/stop pairs; a plan's end (a pause, then End) hit it every time, and so
  did a W0 workout ended while paused. `ActivityWriter` now tracks whether its timer is running
  and writes each start and stop once.
- **The link** (`writeActivityLink`): the sport above, so Streak counts a strength session as
  Strength and a circuit as Hybrid (its `LinkReader` already mapped both; a new Streak test,
  `WorkoutsKindsComeFromTheSportNotTheApp`, pins it). A host test ties the codes to
  `HybridXLink::Sport`.

### W2 verified (1 November 2026)

- **Host tests:** Workout **104** (3 new: each kind's sport and profile name, the link codes
  against `HybridXLink::Sport`, laps carrying their step kind; and the runner tests now pin
  the last lap's trigger), also under ASan and UBSan. Streak **137** (1 new).
- **The FIT file from the real writer** (`fit_check.sh`, ten scenarios, decoded by
  fitdecode): the four W0 scenarios unchanged, and one per kind through the real
  `PlanRunner`, each checked for its sport and sub_sport by name, profile name, every lap's
  timer time, trigger and intensity, the timer stopping at the plan's end (the 10 s before
  End are the last lap's pause), and for strength five set messages matching the laps
  (types, durations, start and end, no repetitions). All pass.
- **The real service in the simulator** (`sim_plan.sh`, all six kinds, the plan written into
  `settings.json`, driven by keys): each plan ran to its end and paused, a Resume afterwards
  was refused, End saved the file with its sport (Strength 10/20, EMOM 62/74, AMRAP 62/73,
  Tabata 62/75, Intervals 62/70, Circuit 10/26), its laps and intensity, start and stop
  events paired, sets for strength only, and `act_workout.json` had sport 4, 5, 5, 5, 5, 6
  with the `.fit`'s timer time. ALL PLANS PASSED.
- **No change to a plain workout:** W0's capture (`capture_screens.sh`, into a scratch folder)
  passes every check, Streak line included; the family check (five apps) passes 19 of 19.
- **The simulator builds** with no warnings from our code. The watch target is CI's.
- **Not checked:** anything on a watch, in particular whether the buzzes are distinct on the
  wrist, and whether the UNA app and Strava take the new sports and sub-sports (and what they
  call them), and the set messages. The screens still show every workout as a plain one: W3.

## W3: the screens for each kind of workout (1 November 2026)

### What there is

- **Main menu** (`w3-01` to `w3-09`): Start, **Type** and a third row. R1 on Type steps round
  the kinds (Free, Strength, EMOM, AMRAP, Tabata, Intervals, Circuit) and the service remembers
  it. The third row is the kind's: **Auto lap** for Free (as in W0), or **Settings** with the
  plan in a line as its hint ("3 sets, 60 s rest", "10 min", "12 min cap", "8 x 20 s / 10 s",
  "40 s / 20 s x 6", "8 rounds, 1 km").
- **Settings** (`PlanSettingsScreen`, new): the kind's numbers on the wheel, each with its value
  as the hint. R1 steps a value to its next choice, R2 goes back. Free and Tabata have nothing to
  set (Tabata's hint says what its fixed plan is). The choices (`PlanText.hpp`, `choicesOf`):

  | Setting | Choices |
  |---|---|
  | Sets | 1-6, 8, 10, 12, 15, 20 |
  | Rest (strength) | 15, 30, 45, 60, 90 s, 2, 3, 4, 5 min |
  | Set cap | 1, 2, 3, 5, 10, 15 min |
  | Minutes (EMOM), Time cap (AMRAP) | 5, 6, 8, 10, 12, 15, 20, 25, 30, 40, 45, 60 |
  | Work (intervals) | 10, 15, 20, 30, 40, 45, 60, 90 s, 2-5 min |
  | Rest (intervals) | 5, 10, 15, 20, 30, 45, 60, 90 s, 2 min |
  | Rounds (intervals) | 1-6, 8, 10, 12, 15, 20, 25, 30 |
  | Rounds (circuit) | 1-8 |
  | Run (circuit) | 100 m to 1 km in 100s |

  A value from an older file that is not in the list steps to the next one up.
- **The live main face for a plan** (`w3-10` to `w3-17`, `w3-23` to `w3-31`):
  - **The heading** is the step, in its colour: "Set 2/3" and "Work 3/8" lime, "Rest" and
    "Rest 3/8" sky, "Run 2/8" white, a station ("SkiErg", "Wall balls") amber, "Min 4/10",
    "Round 4". It is Medium 18, not the Free lap number's SemiBold 25: the face is narrow
    at the top.
  - **The big time** counts down a timed step (sky in a rest, amber in its last 3 s, with the
    countdown buzz) and counts up an open one (a strength set: its cap is a backstop, not a
    target; a circuit's run or station).
  - **The line under it:** the station's work ("1000 m", "100 reps"), the run's length, or
    the total.
  - **When the plan is over:** "Done" in green over the total time, and "R1 to end" in amber.
- **R2's hint** goes when R2 does nothing: the timers (EMOM, Tabata, Intervals), a pause, or
  a finished plan. The screen does not send the lap then.
- **The lap toast** says what begins now ("Rest", "Set 3", "Minute 5", "Work 4", "Round 4",
  "Run 3", "Sled push", "Done") in its colour, then the step just finished and its time ("Set 2
  00:45"). It is worked out from the cue and the finished lap (`formatCueHeading`), because the
  toast arrives before the snapshot of the new step.
- **The totals face** shows the Type in place of the auto lap for a plan.
- **The action menu once a plan is done** (`w3-18`) is End workout, Discard and Back, and opens
  on End: one R1 and a hold ends it. Pause has gone (the service refuses Resume anyway, W2).
- **The summary** (`w3-21`, `w3-22`, `w3-32`, `w3-33`) is titled with the kind. Its line says
  "3 sets", "10 minutes", "7 rounds" (AMRAP: the rounds counted), "8 of 8 rounds" (Tabata),
  "6 rounds", "8 stations" or "4 laps". A plan's lap rows are its steps ("Set 1", "Rest 1",
  "SkiErg", "Round 3") in their colours. The name takes the whole line and the time goes under
  it beside the heart rate: "Wall balls" and "Round 255" do not fit the 76 px a Free workout's
  "Auto 64" uses. A plan's rows leave out the lap's calories.

### Code

- `Core/Header/PlanText.hpp` (header-only, host-tested): the kind names, the station short
  names (mixed case: "Sled push", not Race's "SLED PUSH"), the settings and their choices
  (`planFields`, `choicesOf`, `nextChoice`, `stepField`, `formatField`), and the plan summary.
  Also every step label (`formatStepName`, `formatStepHeading`, `formatCueHeading`), the
  countdown (rounded up: 00:01 until the step ends), `countsDown`, `buttonCounts`, and the
  summary line.
- GUI: `Model` keeps the plan from `SettingsUpdate` and sends `SetPlan` (`nextKind`,
  `stepPlanField`); `MainScreen` (three rows); `PlanSettingsScreen` (new); `LiveScreen`
  (`redrawPlanMain`, the toast); `ActionScreen` (the done menu); `SummaryScreen` (title, line,
  rows); `Theme::stepColor`.
- `capture_screens.sh` (W0) steps past the new Type row to reach Auto lap; `sim_plan.sh` ends
  from the done menu.

### W3 verified (1 November 2026)

- **Host tests: 116** (12 new in `PlanTextTest`: every choice list rises, has at most 20 and
  is inside its range, every label, the countdown's rounding, buffers), also under ASan and
  UBSan.
- **Text-fit audit** (`audit.sh`): **0 problems** in 39 checks. The 16 W0 checks are unchanged;
  23 new ones give each kind its widest text: "Set 20/20" at 59:59, "Min 60/60", "Round 255"
  with 1:00:00 left, "Rest 8/8" amber, "Work 30/30", "Wall balls" with "100 reps", "Burpees",
  "Run 8/8" at 99:59:59, Done at 99:59:59, a paused plan, five toasts, the plan's totals, the
  done menu, and the summaries with 65535 AMRAP rounds and laps 61-63 of each kind's widest
  names. It found the lap rows' names too wide on the way (89-100 px in a 76 px box), hence
  the plan row layout above.
- **Every screen by key presses** (`capture_plans.sh`, into `docs/screens/w3-*.png`): the kind
  and its settings chosen on screen (Sets stepped from 2 to 3); a whole strength workout (a rest
  that ran out, one skipped, the end, the done menu, End, the summary and its laps); EMOM, AMRAP
  and Tabata looked at and discarded; and a whole circuit. Then both files decoded (Strength:
  training / strength_training, five laps active, rest, active, rest, active, five sets;
  Circuit: training / cardio_training, two laps), and `act_workout.json` held sports 4 and 6.
  The captures were looked at by eye; they read as meant.
- **Each kind through the real service again** (`sim_plan.sh`, ending from the done menu): all
  six pass, with the same sports, laps and link codes as in W2.
- **No change to a Free workout:** W0's capture passes every check, Streak line included (its
  screenshots are refreshed for the Type row).
- **The simulator builds** with no warnings from our code. The watch target is CI's.
- **Not checked:** anything on a watch, in particular the LVGL pool there (the screens still build
  only the face on show), how the colours and the 3 s amber read in daylight, and whether the
  buzzes are distinct on the wrist.
