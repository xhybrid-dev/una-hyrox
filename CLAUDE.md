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
  - simulator: `hybridx-streak/Software/Apps/LVGL-GUI/simulator` (see its CMakeLists);
  - captures: `RECORD=1 hybridx-streak/docs/experiments/capture_screens.sh`.
