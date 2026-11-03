# Changelog

All notable changes to HybridX Ride are recorded here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and
the project uses [semantic versioning](https://semver.org/spec/v2.0.0.html).

Releases are tagged `ride-vX.Y.Z`; `Software/cmake/ride-version.cmake` strips the
prefix for the SDK's version script. The store release of the Streak family:
`hybridx-link/docs/STORE_RELEASE.md`.

## [Unreleased]

The first store release, 1.0.0. On the watch the app list calls it **X Ride**
(project name HybridX Ride; its folder stays `HybridXRide`).

### Added

- **Outdoor rides** with GPS speed, distance and climb (the barometer), and heart rate:
  saved as a cycling activity that syncs to the UNA app and Strava.
- **Auto-pause** outdoors (the SDK Cycling example's detector), on by default: it pauses when
  you stop and starts again as you ride off, with one buzz and two.
- **Auto laps** by time (1 to 30 minutes) or, outdoors, by distance (1 to 25 km).
- **Indoor rides** on the turbo with heart rate and time, free or as **Intervals** (work, rest
  and rounds) or **Tabata** (8 x 20 s / 10 s), each step a lap with a countdown and buzzes.
- **Heart rate from a strap**, when one is paired in the watch's settings.
- **The family:** each saved ride is counted by X Streak, with its training load; the Saved
  screen shows the Streak line and "Load N".
