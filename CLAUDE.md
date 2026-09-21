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
