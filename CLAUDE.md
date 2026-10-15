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
entered or Tanaka from birth year and month). It exists to check the method
against a Garmin. It is independent of the other apps.
- **Docs:** findings, decisions, method, phases (R0-R5) and the review:
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
  - a run's FIT: `docs/experiments/fit_vo2_report.py <file.fit>`; a tuning
    log: `docs/experiments/vo2_log_report.py [--max-hr N] <vo2_*.csv>` (also shows
    what the raw GPS speed would have given, from the `scale` column).
  - simulator: `hybridx-run/Software/Apps/LVGL-GUI/simulator`.

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
  - a run in the simulator, with screenshots: `docs/experiments/sim_run.sh [shots-dir] [app_config.json]`.

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
