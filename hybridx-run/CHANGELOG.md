# Changelog

All notable changes to HybridX Run are recorded here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and
the project uses [semantic versioning](https://semver.org/spec/v2.0.0.html).

Releases are tagged `run-vX.Y.Z`; `Software/cmake/run-version.cmake` strips the
prefix for the SDK's version script. The store release of the Streak family:
`hybridx-link/docs/STORE_RELEASE.md`.

## [Unreleased]

## [1.1.0] - 2026-11-16

### Added

- X Training's runs count towards the VO2max, as Trail's do: X Training leaves each run's steady
  minutes in `../SharedData/HybridX/training_vo2_0..2.json`, and X Run reads them when it opens,
  judges them by its own rules and adds them to the history (X Training's TP4; NOTES "X
  Training's runs").

### Changed

- `vo2.json` is version 5: a second mark (`"training"`) and a third source (2, X Training).
  Versions 1 to 4 still read; an older X Run cannot read version 5.
- The activity link has a sixth slot (`act_training.json`): the Streak line also counts X
  Training's runs.

## [1.0.0] - 2026-11-09

The first store release. On the watch the app list calls it **X Run**
(project name HybridX Run; its folder stays `HybridXRun`).

### Added

- **Runs** outdoors with GPS (from the SDK's RunLVGL): distance, pace, heart rate, laps and
  climb, saved as a running activity that syncs to the UNA app and Strava.
- **VO2max, worked out on the watch** from steady running: the ACSM running equation from
  pace and gradient, scaled by your heart-rate reserve, with your maximum heart rate entered
  or estimated from your birth year and month (Tanaka). A figure, a trend and the runs behind
  it on the VO2max screens; X Trail's steady runs count towards it too.
- **Treadmill mode**: speed and distance from your cadence (the SDK's stride model), with the
  console's distance entered after the run; no VO2max from the treadmill.
- **Run detail**: cadence and step length (live and in the summary), time in each heart-rate
  zone, best 1 km, 1 mile and 5 km, the two halves, and uphill-adjusted pacing and drift.
- **The family:** each saved run is counted by X Streak, with its training load (Edwards'
  heart-rate score), and the Saved screen shows this week's count, the streak and "Load N".
- **The VO2max glance**, its own store listing: the figure, its change, and the last 10 runs.
- **Phone settings**: birth year and month, maximum and resting heart rate, a tuning log.
