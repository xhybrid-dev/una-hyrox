# Changelog

All notable changes to HybridX Trail are recorded here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and
the project uses [semantic versioning](https://semver.org/spec/v2.0.0.html).

Releases are tagged `trail-vX.Y.Z`; `Software/cmake/trail-version.cmake` strips the
prefix for the SDK's version script. The store release of the Streak family:
`hybridx-link/docs/STORE_RELEASE.md`.

## [Unreleased]

## [1.1.0] - 2026-11-24

The first field test's amendments (NOTES "T4").

### Added

- **Wrong way**: on the route but running it backwards (30 m back along it), a quick, insistent
  buzz and an orange "Wrong way / turn around" with an arrow the route's way; a reminder every
  minute; "Right way" once you are running it forwards again. Never on an out-and-back's
  turnaround, nor from GPS wandering while you stand (NOTES "T4.2").
- **Where routes go**: the route list's "Add routes" names the app's folder (from the store,
  `Apps/1C83FDDE4EA3C880`), and R1 on it shows the whole path.
- **Form after a save**: with X Streak installed (a version that leaves its training load,
  Streak NOTES "S10"), the Saved screen says where your form now stands, under the session's load
  ("Form now -18, building"), worked out by Streak's own model. Not while Streak is still learning
  (its first two weeks).

### Changed

- **Off course at 20 m** from the line (it was 50 m), and back on course within 12 m (it was 30 m).
- **The alert banner is a cap across the top** of the face, so the map below it still shows you
  and the route (it was a band across the middle, over your arrow).
- **After the finish** the map and the navigation face show the run's time and distance, still
  counting until you save (they showed "0 m to go" and looked stopped; the recording never was).

### Fixed

- "Back on course" shows again: the green banner was put straight back to "Off course" and then
  hidden, so only the buzz said so.
- On an out-and-back, turning a little before the GPX's turnaround no longer sends "to go" back
  up the outward leg: the return leg is taken (NOTES "T4.2").

## [1.0.1] - 2026-11-16

### Changed

- The activity link has a sixth slot, X Training's (`act_training.json`): the Streak line after a
  save also counts X Training's runs. Nothing else changes.

## [1.0.0] - 2026-11-09

The first store release. On the watch the app list calls it **X Trail**
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
