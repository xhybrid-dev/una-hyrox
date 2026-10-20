# Workspace: HybridX Race for UNA Watch

This workspace holds two folders:
- `una-sdk/`: the UNA Watch SDK (read-only reference; never edit it). `UNA_SDK` points here.
  In Claude Code on the web, `.claude/hooks/session-start.sh` fetches it at
  the commit CI pins (`UNA_SDK_REF` in `.github/workflows/watch-builds.yml`)
  and sets `UNA_SDK`; bump the SDK there and new sessions follow.
- `hybridx-race/`: our app, a HYROX-format race timer for the UNA Watch.

SDK status: pinned at `sdk-v1.6.0` (`bd04a5b8`) since 14 October 2026. See
`hybridx-race/docs/NOTES.md` 5.21-5.28 before any SDK or firmware-compatibility work.

## Before any work
1. Read `hybridx-race/docs/HYBRIDX_RACE_BRIEF.md` in full at the start of each phase.
2. Read `hybridx-race/docs/NOTES.md` for decisions and findings so far (create it in Phase 0).
3. Check which phase we are in (Section 13 of the brief) and do only that phase's work.

## Standing rules
- The SDK repo (code and `Docs/`) is the source of truth for the platform. If it conflicts with the brief, follow the SDK, log the conflict in `NOTES.md` and tell Jon.
- Never invent SDK APIs, manifest keys, FIT profile numbers or HYROX pacing data. Ask.
- Use plan mode at the start of each phase. Stop at each gate and summarise: what was done, how it was verified, what Jon must test or decide.
- GUI toolkit is LVGL, starting from `una-sdk/Examples/Apps/RunLVGL`, unless Gate 0 fails.
- Keep race logic in pure C++ with an injected clock, covered by host tests. Build both the watch target and the simulator after every change.
- Embedded rules: no MMU (bounds-check everything), no heap churn in the 1 Hz loop, fixed-size message structs, integer formatting (no `%f`), unsigned deltas for the wrapping ms clock.
- Commit at the end of each phase with a clear message. Tag releases `vX.Y.Z` so the SDK's version script picks them up.
- UI text is British English.

## Common commands (confirm against `Docs/sdk-setup.md` in Phase 0)
- Build for the watch: `cmake -G "Unix Makefiles" -S hybridx-race/Software/Apps/HybridXRace-CMake -B hybridx-race/build && cmake --build hybridx-race/build`
- Host tests: see `Docs/unit-testing.md`
- Simulator: see `Docs/Simulator.md` and `Docs/Tutorials/RunLVGL/ARCHITECTURE.md`
- Race's pace plan in the simulator (two races, about 2 minutes): `hybridx-race/docs/experiments/pace_plan_capture.sh [out-dir] [last|target]`

## About Jon
Chartered engineer, runs HybridX (Hyrox coaching platform). Comfortable with TypeScript/Next.js, new to embedded C++. Explain build or toolchain steps he must do himself clearly and one at a time.

## HybridX Streak (`hybridx-streak/`)
A second app in this repo: a weekly-target streak that counts every activity
the watch records, from any app. It is independent of HybridX Race.
- **Docs:**
  - brief: `hybridx-streak/docs/UNA_STREAK_TRACKER_BRIEF.md`;
  - plan and phases: `PLAN.md`, §11 (S0-S6);
  - findings: `NOTES.md`;
  - look and voice: `DESIGN.md`;
  - the Gate 0 probe: `PROBE.md`.
  Read `PLAN.md` and `NOTES.md` at the start of each Streak phase, and work
  only on that phase.
- **Rules:** the same standing rules as Race apply: plan mode, gates, SDK as
  the source of truth, pure core with host tests, embedded rules, British
  English.
- **Starting points:** the LVGL GUI started from Race's scaffold. The app,
  its glance (`Apps/HybridXStreakGlance-CMake`) and the probe
  (`Tools/Probe`) are separate CMake projects.
- **Releases:** tag them `streak-vX.Y.Z`; `Software/cmake/streak-version.cmake`
  strips the prefix.
- **Watch builds come from CI** (`.github/workflows/watch-builds.yml`, artifact
  `watch-apps`). Local container builds use Ubuntu's compiler plus syscall
  stubs, so they are compile checks only and never go on a watch.
- **Commands:**
  - host tests: `cmake -S hybridx-streak/Tests/Host -B hybridx-streak/build-tests && cmake --build hybridx-streak/build-tests && hybridx-streak/build-tests/hybridx-streak-host-tests`;
  - simulator: `hybridx-streak/Software/Apps/LVGL-GUI/simulator` (see its
    CMakeLists). `build/` is the real app; `build-demo/` (`-DHYBRIDXSTREAK_DEMO=ON`)
    is the S0 design demo.
  - a pretend watch for the real simulator: `docs/experiments/sim_fixtures.sh
    <dir>` (real FIT files from `build-tests/make_fit`), plus
    `build-tests/make_state` for a history. Run the simulator from
    `<dir>/a/b/c/d/e`.
  - captures:
    - real app: `docs/experiments/capture_real.sh`, `walkthrough_real.sh`;
    - demo: `RECORD=1 capture_screens.sh`, `walkthrough.sh`;
    - glance mock-up: `glance_preview.py build-tests/glance_preview <out.png>`.

## HybridX Trail (`hybridx-trail/`)
A third app: breadcrumb route navigation for runners and trail runners. You
load a GPX, follow the line, and get a buzz when you go off course. It is
independent of Race and Streak.
- **Docs:**
  - brief, plan and phases (§6, T0-T5): `hybridx-trail/docs/HYBRIDX_TRAIL_BRIEF.md`;
  - findings: `NOTES.md`;
  - the Gate T0 probe: `PROBE.md`;
  - the draft request to UNA about phone delivery: `UNA_GPX_REQUEST.md`.
  Read the brief and `NOTES.md` at the start of each Trail phase, and work
  only on that phase.
- **Rules:** the same standing rules as Race apply.
- **Layout:** the pure route core (GPX reader, thinning, route maths) is
  `Software/Libs/Core`, shared by the probe (`Tools/Probe`) and the future app.
  Routes go in the app's `Routes/` folder: over USB for development, and from
  the phone over the watch's BLE File Transfer Service for release (NOTES,
  "Phone delivery over BLE"). The Android sender is `Tools/RouteSender`
  (plain Java, no Gradle; its README has the test plan).
- **Run link:** Trail leaves its runs' steady minutes in the shared folder for
  HybridX Run's VO2max (NOTES "Run link"; plan in `hybridx-run/docs/VO2_LINK_PLAN.md`).
  `Software/Libs/Vo2Link` holds **copies of Run's window code** (edit Run's, then
  `Utilities/check-vo2-link-copies.sh --sync`; CI checks they match).
- **Watch builds come from CI**, as for Streak. So does the Route Sender APK
  (`.github/workflows/route-sender.yml`, artifact `route-sender-apk`).
- **Commands:**
  - host tests: `cmake -S hybridx-trail/Tests/Host -B hybridx-trail/build-tests && cmake --build hybridx-trail/build-tests && hybridx-trail/build-tests/hybridx-trail-host-tests`;
  - probe simulator: `hybridx-trail/Tools/Probe/Software/Apps/Probe-GUI/simulator`.
    Put GPX files in `Tools/Probe/Software/Output/Routes/` and run it from
    `build/bin`;
  - test routes: `python3 hybridx-trail/Tools/TestRoutes/make_test_gpx.py <folder>`.
  - app simulator: `hybridx-trail/Software/Apps/LVGL-GUI/simulator` (routes
    in `Software/Output/Routes/`, from `Tools/TestRoutes/make_sim_routes.py`);
  - captures: `hybridx-trail/docs/experiments/capture_screens.sh [out-dir]`
    (every screen, into `docs/screens/`); `sim_run.sh` for a quick run.
  - Route Sender (Android APK, with host tests):
    `hybridx-trail/Tools/RouteSender/build.sh`. It uses `$ANDROID_HOME` if
    set, else Ubuntu's `android-sdk-platform-23 aapt apksigner zipalign
    dalvik-exchange` (dl.google.com is blocked in the container).

## HybridX Intervals (`hybridx-intervals/`) — RETIRED (14 October 2026)
No further work: UNA is building structured workouts into Running (see
`hybridx-race/docs/NOTES.md` 5.25). Code and notes stay for reference only.
A fourth app: structured workouts (warm-up, work and rest steps with pace or
heart-rate targets, repeats, cool-down) for running. It is independent of Race,
Streak and Trail.
- **Docs:** findings, decisions and phases: `hybridx-intervals/docs/NOTES.md`
  (P0-P3, gates); the workout file format: `WORKOUT_FILE.md`; the retired
  P0 probe: `PROBE.md`. Read `NOTES.md` at the start of each Intervals phase.
- **Rules:** the same standing rules as Race apply.
- **Layout:** the pure core (model, parser, engine, runner, store, text) is
  `Software/Libs/Core`; the service (from RunLVGL) `Software/Libs/App`; the GUI
  `Software/Apps/LVGL-GUI`. A workout is always a JSON file in the app's
  `Workouts/` folder: over USB now, from the phone over BLE FTS later (Trail's
  Route Sender shows how).
- **Watch builds come from CI**, as for Streak and Trail.
- **Commands:**
  - host tests: `cmake -S hybridx-intervals/Tests/Host -B hybridx-intervals/build-tests && cmake --build hybridx-intervals/build-tests && hybridx-intervals/build-tests/hybridx-intervals-host-tests`;
  - simulator: `hybridx-intervals/Software/Apps/LVGL-GUI/simulator`;
  - example workouts: `hybridx-intervals/Tools/TestWorkouts/`;
  - a run in the simulator: `docs/experiments/sim_run.sh [workout] [seconds]`;
    every screen: `docs/experiments/capture_screens.sh [out-dir]` (into
    `docs/screens/`).

## HybridX Run (`hybridx-run/`)
A fifth app, and a test: the SDK's RunLVGL copied, plus a VO2max estimate
worked out live during the run (ACSM running equation, %HRR ≈ %VO2R, max HR
entered or Tanaka from birth year and month), with a VO2max glance. It
exists to check the method against a Garmin. It is independent of the other
apps.
- **Docs:** findings, decisions, method, phases (R0-R6) and the review:
  `hybridx-run/docs/NOTES.md`. Read it at the start of each Run phase.
- **Rules:** the same standing rules as Race apply.
- **Layout:** the pure VO2max core `Software/Libs/Core` (its only SDK use is
  the JSON reader and writer); the service (from RunLVGL) `Software/Libs/App`;
  the GUI `Software/Apps/LVGL-GUI`.
- **Watch builds come from CI**, as for the others. Tags `run-vX.Y.Z`.
- **Commands:**
  - host tests: `cmake -S hybridx-run/Tests/Host -B hybridx-run/build-tests && cmake --build hybridx-run/build-tests && hybridx-run/build-tests/hybridx-run-host-tests`;
  - simulator: `hybridx-run/Software/Apps/LVGL-GUI/simulator`;
  - a run in the simulator: `docs/experiments/sim_run.sh [seconds] [shots]
    [app_config.json]` (`SEED=1` for a history); the VO2max screens:
    `docs/experiments/capture_vo2.sh [out-dir]` (into `docs/screens/`);
  - Trail's runs counted by Run (synthetic files): `docs/experiments/sim_trail_ingest.sh`
    (needs the simulator and `build-tests/make_trail_run`);
  - a run's FIT: `docs/experiments/fit_vo2_report.py <file.fit>`; a tuning
    log: `docs/experiments/vo2_log_report.py [--max-hr N] <vo2_*.csv>` (also shows
    what the raw GPS speed would have given, from the `scale` column).
- **The VO2max glance** (R6): `Software/Apps/HybridXRunGlance-CMake` with
  `Software/Libs/Glance`; it reads the app's `../SharedData/HybridX/vo2max.json`.
  Preview (the simulator cannot run glances):
  `docs/experiments/glance_preview.py build-tests/glance_preview <out.png>`.

## X Weigh-in (`x-weighin/`)
A sixth app: an on-watch weight tracker in kg. Weigh in on the watch, see the
smoothed trend (a tracker: no goals or warnings for now). Height (and the starting weight) come from
the UNA profile; birth date and sex are phone config fields. The data stays on
the watch: no phone app, and no activity FIT files, so nothing syncs to UNA or
Strava. It is independent of the other apps.
- **Docs:** findings, decisions and phases (W0-W4): `x-weighin/docs/NOTES.md`.
  Read it at the start of each Weigh-in phase.
- **Rules:** the same standing rules as Race apply.
- **Layout:** started from the SDK's Waypoint tutorial (LVGL). The pure core
  `Software/Libs/Core`; the service `Software/Libs/App`; the GUI
  `Software/Apps/LVGL-GUI`. The glance is a second `.uapp`
  (`Apps/XWeighInGlance-CMake`, `Software/Libs/Glance`, as Streak's): it reads
  the log's SharedData copy and never writes.
- **Watch builds come from CI**, as for the others. Tags `weighin-vX.Y.Z`.
- **Commands:**
  - host tests: `cmake -S x-weighin/Tests/Host -B x-weighin/build-tests && cmake --build x-weighin/build-tests && x-weighin/build-tests/x-weighin-host-tests`;
  - simulator: `x-weighin/Software/Apps/LVGL-GUI/simulator` (settings in
    `Software/Output/app_config.json`);
  - a run in the simulator, with screenshots: `docs/experiments/sim_run.sh [shots-dir] [app_config.json]`;
  - every screen in the simulator, and a weigh-in round trip:
    `docs/experiments/capture_screens.sh [out-dir]` (into `docs/screens/`);
  - a sample log: `build-tests/make_log <out.csv> [days] [seed]`; a log's trend
    day by day: `build-tests/trend_csv <weighins.csv>`; its chart:
    `docs/experiments/trend_chart.py <trend.csv> <out.png>`;
  - the glance (the simulator cannot run glances): a mock-up from the real
    layout code, `docs/experiments/glance_preview.py build-tests/glance_preview
    <out.png>`;
  - the store packages (two listings: the app, and the glance, which needs
    it): `Utilities/pack-store-zip.sh` and `--glance` (CI artifact
    `weighin-store-zip`); the glance's previews: `glance_preview.py
    build-tests/glance_preview <out.png> --store Resources/Glance/previews`;
    the steps: `docs/SUBMISSION.md`;
  - store images (previews, mock-ups, marketing, one set per listing):
    `docs/experiments/store_images.py build-tests/glance_preview docs/store`.

## X Velocity (`x-velocity/`)
A seventh app, at the brief stage: a gym tool. It counts reps, measures bar speed
per rep (velocity-based training, with a buzz when the set slows past a cut-off),
and runs a jump test (height from flight time). It is independent of the other apps;
its rep counter is meant to be reused by Race for wall balls and burpees later.
- **Docs:** brief, plan and phases (V0-V5): `x-velocity/docs/X_VELOCITY_BRIEF.md`;
  the desk study and findings: `NOTES.md`; the Gate V0 probe plan and Jon's
  gym-session protocol: `PROBE.md`. Read the brief and `NOTES.md` at the start of
  each Velocity phase, and work only on that phase.
- **Rules:** the same standing rules as Race apply. Recorded probe sets are the
  host-test data: never tune the counter on a set without adding it as a test.
- **Watch builds come from CI**, as for the others. Tags `velocity-vX.Y.Z`
  (the name is still Jon's decision, brief §11).

## Trail Quest (`trailquest/`)
A ninth app, and a game: a Game Boy-style, turn-based trail-running adventure.
You run "The Ridge" (5 stages), choose routes at forks, meet original creatures
("Trail Spirits") that become running partners, and win short duels with pace and
grit; each stage ends with a boss. Planned extra: Daily Trail (a run seeded from
the date). It is independent of the other apps. Original only: no Pokémon names,
creatures, moves, art or type chart (brief section 9).
- **Docs:** brief, plan and phases (Q0-Q6): `trailquest/docs/TRAILQUEST_BRIEF.md`;
  findings, decisions and the gate records: `NOTES.md`; look and voice:
  `DESIGN.md`. Read the brief and `NOTES.md` at the start of each phase, and work
  only on that phase. Where the brief and `NOTES.md` disagree, `NOTES.md` is right.
- **Rules:** the same standing rules as Race apply. Also:
  - the GUI runs at **10 frames a second** and gets one button code per frame, so
    the game is turn-based and any timing input needs zones of at least two frames;
  - the buzzer's **pitch** (SDK v1.6.0) plays only on new enough firmware, so every
    sound must work as rhythm alone; the vibro carries the meaning;
  - the display is round: keep text inside the circle;
  - colours are on the display's grid (each channel 0x00, 0x40, 0x80 or 0xC0);
  - the GUI process links none of the library sources, so **anything the GUI uses
    from `Libs/Core` must be header-only**; the simulator hides this, the watch
    build catches it.
- **Layout:** the pure core `Software/Libs/Core`: the timing beat, the feedback
  table, and (Q1) the game itself: data tables in `Header/data/` (moves,
  spirits, items, stages; header-only), the duel, encounters, the run flow
  (`Run`), the save, and the Daily Trail seed. Every chance comes from one
  seeded `Rng`, so a seed and the inputs replay a game exactly. `Session` (Q2)
  is the game as the service runs it; `View.hpp`, `Layout.hpp`, `TextFit.hpp`
  and `Narrate.hpp` are what the GUI draws from, and every string is
  host-tested to fit the round face clear of the bezel arcs. The service
  `Software/Libs/App` runs the Session and answers each command with
  snapshots; the GUI `Software/Apps/LVGL-GUI` (eight screens, a screen manager
  with no heap, the sprite expander) draws them. The story pages (Q3) are
  `Header/data/Story.hpp`, shown by `StoryScreen`. No glance. Art is drawn by
  `Tools/Art/gen_art.py` into `assets/sprites/sprites.c`; never edit that file.
- **Watch builds come from CI**, as for the others. Tags `trailquest-vX.Y.Z`.
  Local watch builds are compile checks only (the stub recipe is in `NOTES.md`).
- **Commands:**
  - host tests: `cmake -S trailquest/Tests/Host -B trailquest/build-tests && cmake --build trailquest/build-tests && trailquest/build-tests/trailquest-host-tests`;
  - balance (built with the tests; `docs/BALANCE.md` has the targets and how to
    read them): every spirit, many duels, `build-tests/balance_sim [n] [--md out]
    [--csv out]`; whole games, `build-tests/run_sim [runs] [casual|good|menu]
    [--md out]`. Re-run both after changing any number in `Libs/Core`;
  - simulator: `trailquest/Software/Apps/LVGL-GUI/simulator` (a Ninja build in
    its `build/`; run `build/bin/TrailQuestSimulator`; keys 1-4 are L1-R2);
  - screens, cut to the round face: `docs/experiments/capture_screens.sh [out-dir]`
    (a scripted stage 1 with its story pages and stage 2's start, every screen,
    into `docs/screens/q3-*.png`);
    `docs/experiments/tune_balance.py` retunes the stage softening and boss stamina; `docs/experiments/sim_play.sh [seed]` plays stages 1-3 to the clear (beat off)
    and checks the save is picked up again; `capture_boss.sh [stage] [out-dir]`
    starts just before a stage's boss and captures its warning and twist. All
    replace the save in `Software/Output`; `TRAILQUEST_SEED` fixes the simulator's seed;
  - art: `python3 trailquest/Tools/Art/gen_art.py` (writes the sprites and a
    preview sheet); icons: `python3 trailquest/Resources/make_icons.py`.

## Knitting (`knitting/`)
An eighth app: a knitting counter. Press a button for every row, group rows into
sets (rows and sets, like reps and sets), and see the rows in this set (big), the
set number and the total rows (small). Buttons: R1 counts a row (hold: undo a row),
L1 starts the next set, L2 resets (hold 1.5 s), R2 exits. It is an activity-free utility: no GPS, no FIT files,
nothing syncs. It is independent of the other apps.
- **Docs:** findings, decisions and phases (K0-K5): `knitting/docs/NOTES.md`.
  Read it at the start of each Knitting phase, and work only on that phase.
- **Rules:** the same standing rules as Race apply.
- **Layout:** as X Weigh-in: the pure core `Software/Libs/Core` (header-only where
  the GUI uses it), the service and messages `Software/Libs/App`, the GUI
  `Software/Apps/LVGL-GUI`, the watch build `Software/Apps/Knitting-CMake`.
- **Watch builds come from CI**, as for the others. Tags `knitting-vX.Y.Z`. App ID
  `CDA5282FFDCE502F` (portal-issued). The store package (`Utilities/pack-store-zip.sh`,
  CI artifact `knitting-store-zip`) and the release steps: `docs/SUBMISSION.md`.
- **Commands:**
  - host tests: `cmake -S knitting/Tests/Host -B knitting/build-tests && cmake --build knitting/build-tests && knitting/build-tests/knitting-host-tests`;
  - simulator: `knitting/Software/Apps/LVGL-GUI/simulator` (keys 1-4 are L1, L2, R1, R2);
  - a run in the simulator, with screenshots: `docs/experiments/capture_screens.sh [out-dir]`
    (into `docs/screens/`). It needs Xvfb, xdotool and ImageMagick. Also
    `resume_check.sh` (the saved count) and `hold_check.sh` (the holds; key
    down, wait, key up, as on the watch); `store_previews.sh` (the four store
    previews, into `Resources/previews/`); store images (watch mock-ups in teal
    and white, cards, banner): `UNA_SDK=... python3 docs/experiments/store_images.py docs/store`.
  - a local watch compile check needs Race's `syscall_stubs.c` built with
    `-mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb`, and `pyelftools`.
