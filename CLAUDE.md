# Workspace: HybridX Race for UNA Watch

This workspace holds two folders:
- `una-sdk/`: the UNA Watch SDK (read-only reference; never edit it). `UNA_SDK` points here.
- `hybridx-race/`: our app, a HYROX-format race timer for the UNA Watch.

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
  Routes go in the app's `Routes/` folder, over USB for v1.
- **Watch builds come from CI**, as for Streak.
- **Commands:**
  - host tests: `cmake -S hybridx-trail/Tests/Host -B hybridx-trail/build-tests && cmake --build hybridx-trail/build-tests && hybridx-trail/build-tests/hybridx-trail-host-tests`;
  - probe simulator: `hybridx-trail/Tools/Probe/Software/Apps/Probe-GUI/simulator`.
    Put GPX files in `Tools/Probe/Software/Output/Routes/` and run it from
    `build/bin`;
  - test routes: `python3 hybridx-trail/Tools/TestRoutes/make_test_gpx.py <folder>`.
