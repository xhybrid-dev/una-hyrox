# Changelog

All notable changes to HybridX Workout are recorded here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and
the project uses [semantic versioning](https://semver.org/spec/v2.0.0.html).

Releases are tagged `workout-vX.Y.Z`; `Software/cmake/workout-version.cmake` strips the
prefix for the SDK's version script. The store release of the Streak family:
`hybridx-link/docs/STORE_RELEASE.md`.

## [Unreleased]

## [1.0.1] - 2026-11-16

### Added

- An EMOM's or AMRAP's minutes can be any whole minute from 1 to 60: on that settings screen L1
  and L2 scroll it down and up one minute at a time (32 is two presses from 30). R1 still steps
  through the usual lengths.

### Changed

- The activity link has a sixth slot, X Training's (`act_training.json`): the Streak line after a
  save also counts X Training's runs. Nothing else changes.
### Added

- **Open sets**, an eighth kind (Type on the main menu, next to Strength), from a user's request:
  nothing to set. R2 swaps a set for a rest and back, and each is timed up from zero, so the
  reps and the rest are whatever you make them (Strength's 3 sets and 60 s rest were too
  fixed). The live heading reads "Set 4" and "Rest" with no total; the summary says "4 sets".
  It saves as Strength (FIT training / strength_training, a set message for each set and
  rest) and counts in X Streak as Strength. At most 32 sets, since the file holds 64 laps;
  the 32nd set's R2 ends the plan as the last Strength set does, and End from the menu ends
  it at any time.

## [1.0.0] - 2026-11-09

The first store release. On the watch the app list calls it **X Workout**
(project name HybridX Workout; its folder stays `HybridXWorkout`).

### Added

- **Seven kinds of session**, chosen on the main menu with their settings: Free (laps by
  the button or a time auto lap), Strength (sets with a rest timer), EMOM, AMRAP, Tabata,
  Intervals (work, rest and rounds) and Hyrox-style Circuits (runs and stations).
- **On the wrist**: a face for each kind, with the step, a countdown that turns amber with
  three ticks in its last three seconds, and a different buzz for work, rest and the end.
- **Heart-rate zones and calories** (the SDK's Workout example's), and each step saved as a
  lap; strength sets as FIT `set` messages; the sport chosen per kind.
- **The family:** each saved workout is counted by X Streak as Strength, Hybrid (a circuit) or
  Workout, with its training load; the Saved screen shows the Streak line and "Load N".
