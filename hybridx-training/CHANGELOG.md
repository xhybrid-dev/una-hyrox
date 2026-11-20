# Changelog

All notable changes to X Training (HybridX Training) are recorded here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and
the project uses [semantic versioning](https://semver.org/spec/v2.0.0.html).

Releases are tagged `training-vX.Y.Z`; `Software/cmake/training-version.cmake` strips the
prefix for the SDK's version script. The store release of the family:
`hybridx-link/docs/STORE_RELEASE.md`.

## [Unreleased]

### Added

- **How it went, and how you felt** (TP8): after a planned session that counts, Saved says how well it kept to
  its paces ("On pace 82%", or "Too fast, 40% on pace"), then asks how you felt (**Fresh, Good, Steady, Tired,
  Spent**, or skip). The summary has a **How it went** face: the share of the time in the target band, how many
  reps were in band, which side the misses were on and by how much (s/km or s/mi), and any fade in the later
  reps. **Plan, then L2: Form** shows the last eight sessions as bars with a dot for how you felt, and one line
  on the pattern. Paces are judged by the same zone your watch shows, quality steps only in a session that has
  them (a short rep by its average, as the step card says); a treadmill session is not judged, as its speed is
  assumed. Kept in `reviews.json`, not in the activity file.
- **A suggestion, never after one bad run:** after three sessions in a row too slow, Today offers to **hold the
  plan** (next week runs this week's plan again), with the reason it can give (tired, or fresh yet slow: the
  paces may be set too hard). Three too fast only says so. A refusal (the last weeks before a race, the plan's
  growth limit) says why. Not asked again until three more sessions.

### Fixed

- A plan set up in the middle of its first week no longer logs the days before it as missed, and a plan from
  1.0.0 is judged from the day it is first opened (these two went into 1.1.0 before its tag).

## [1.1.0] - 2026-11-17

The plan learns from what you do, a planned session can run on a treadmill, and a glance is ready for
today's session. (1.0.0 was published before the plan's memory was built.)

### Added

- **The plan remembers** (TP5): days gone are logged missed, a missed key session takes a later
  quality's place, a week less than half done is repeated, a missed long run is offered the next
  day; **Changes** on the main menu does tomorrow's session today, moves today's to tomorrow,
  skips, pauses or ends the plan; a time trial or race recalibrates the paces (faster applies,
  slower or a big jump asks).

- **Treadmill sessions:** the main menu's **Treadmill** switch now applies to today's planned session as well
  as a free run (until now Today ignored it and recorded outdoors, with GPS). With it on, Today starts with no
  GPS fix, and the speed and distance come from your cadence as in UNA's treadmill mode; after Save the
  console's distance is asked for, and the file is a treadmill run (Streak counts it as a Run). Where a step
  has a designated speed (intervals, cruise, reps, race pace) the belt is **assumed to be at it**: a 400 m rep
  at 14.0 km/h ends by itself after 103 s, a 3-minute rep has covered 700 m, and each lap has that distance;
  easy running, warm-ups and cool-downs use the cadence estimate, and a step that ends on a lap press stays
  so. Today, the step list and the step cards show the **belt speed** (km/h or mph) as the target. On a
  treadmill the pace buzzes ("too fast", "too slow") are off, and a time trial or race counts as a session
  but does not change your paces.

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
- **Counts in X Streak** (its own slot, `act_training.json`) with its training load, shows the
  Streak line when it is saved, and leaves its steady minutes for X Run's VO2max
  (`training_vo2_*.json`). The Plan screen shows X Run's VO2max when X Run has one.
