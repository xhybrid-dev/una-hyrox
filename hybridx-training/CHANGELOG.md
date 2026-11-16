# Changelog

All notable changes to X Training (HybridX Training) are recorded here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and
the project uses [semantic versioning](https://semver.org/spec/v2.0.0.html).

Releases are tagged `training-vX.Y.Z`; `Software/cmake/training-version.cmake` strips the
prefix for the SDK's version script. The store release of the family:
`hybridx-link/docs/STORE_RELEASE.md`.

## [Unreleased]

### Added

- **A glance** ("X Training Glance", its own store listing, which needs the app): today's session on the
  glances screen. Line one says what it is ("Today: Intervals"; "Done: Intervals" in green once saved; "Rest
  day"), line two its "what" and target pace ("4 x 4 min @ 4:18/km") or the next session ("Tomorrow: Cruise"),
  and under them the week's seven days as pips and a bar of how far through the plan you are. It also says when
  the plan is paused, not started or complete, and "Open the app" when the app has not been opened for a
  fortnight. It shows what the app saved; it never writes.
- **`training_today.json`**: the app now leaves the glance its plan's days (the week so far and the next 14
  days) in `../SharedData/HybridX/`, each time the app opens and whenever the plan changes or a session is saved.
  Without the glance installed nothing reads it.

## [1.0.0] - 2026-11-16

The first store release. On the watch the app list calls it **X Training** (project name
HybridX Training; its folder stays `HybridXTraining`).

### Added

- **Training plans on the watch:** a 10K plan, a half-marathon plan and an 8-week VO2max block,
  each at three levels, on the runner's own run days. The runner enters a recent 5K (and, if they
  have one, a half) on the phone; the watch works out the paces (Daniels' tables, checked row by
  row in the host tests), the calendar from a start date or a race date, and each day's session.
- **Today:** the session, its target pace, its distance and time and what it builds; R1 starts it,
  and the steps are one scroll away. On a rest day, the next session.
- **The session runs on the SDK's workout engine:** a step card at every change, a pace gauge with
  a buzz when you drift out of range (easy runs warn "too fast" only), lead-in beeps, a held R2
  before a time trial or race, and each short rep (strides, repetitions, 30-30s) judged on the
  next step's card.
- **This week, Plan and Paces:** seven days from Monday with their marks, the weeks of the plan
  by phase, and every pace from the 5K with its band and the predicted 10K and half.
- **Setting a plan on the watch:** New plan sets the plan, the 5K, the level, the run days, the
  long run and the race date, and writes them back to the phone's settings.
- **Records every run as X Run does:** GPS, heart rate, laps (a lap per step), the FIT file, the
  run detail pages, cadence and step length, and the treadmill.
- **The plan remembers** (TP5): days gone are logged missed, a missed key session takes a later
  quality's place, a week less than half done is repeated, a missed long run is offered the next
  day; **Changes** on the main menu does tomorrow's session today, moves today's to tomorrow,
  skips, pauses or ends the plan; a time trial or race recalibrates the paces (faster applies,
  slower or a big jump asks).
- **Counts in X Streak** (its own slot, `act_training.json`) with its training load, shows the
  Streak line when it is saved, and leaves its steady minutes for X Run's VO2max
  (`training_vo2_*.json`). The Plan screen shows X Run's VO2max when X Run has one.
