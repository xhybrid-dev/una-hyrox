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
| W1 | The session core, pure and host-tested: strength sets with rest; HIIT as EMOM, AMRAP, Tabata (8 x 20/10) and custom; Hyrox-style circuits from Race's stations | |
| W2 | Service and FIT per type: sport and sub_sport (strength: training 10 / strength_training 20; HIIT: hiit 62 / hiit 70, amrap 73, emom 74, tabata 75; circuit: training 10 / cardio_training 26 or generic), FIT `set` messages as the rep hook; link sports Strength, Workout, Hybrid | |
| W3 | Screens: the type and its settings, live faces per type, the rest countdown, the summary | |
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
