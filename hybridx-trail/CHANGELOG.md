# Changelog

All notable changes to HybridX Trail are recorded here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and
the project uses [semantic versioning](https://semver.org/spec/v2.0.0.html).

Releases are tagged `trail-vX.Y.Z`; `Software/cmake/trail-version.cmake` strips the
prefix for the SDK's version script. The store release of the Streak family:
`hybridx-link/docs/STORE_RELEASE.md`.

## [Unreleased]

The first store release, 1.0.0. On the watch the app list calls it **X Trail**
(project name HybridX Trail; its folder stays `HybridXTrail`).

### Added

- **Route navigation** from a GPX file: a route list with each route's distance and climb, a
  preview, then a heading-up (or north-up) map with your arrow on the route line, zoomed with
  one button from 300 m to 3 km, and a navigation face (to go, done, on or off course).
- **Off course**, 25 m or more from the line: a buzz, a banner on every face with the
  distance and an arrow back to the route; "Back on course" when you rejoin it, and "Route
  complete" at the finish. Turn cues ahead of each turn, and an elevation profile for routes
  with heights.
- **It is a run, too**: distance, pace, heart rate and laps, saved as a running activity that
  syncs to the UNA app and Strava.
- **Routes go in over USB**, into the app's `Routes/` folder (a phone sender is being tried,
  `Tools/RouteSender`, and is not part of this release).
- **The family:** each saved run is counted by X Streak, with its training load, and its steady
  minutes count towards X Run's VO2max; the Saved screen shows the Streak line and "Load N".
