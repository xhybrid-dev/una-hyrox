# Changelog

All notable changes to HybridX Streak are recorded here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and
the project uses [semantic versioning](https://semver.org/spec/v2.0.0.html).

Releases are tagged `streak-vX.Y.Z`; `Software/cmake/streak-version.cmake`
strips the prefix for the SDK's version script.

## [Unreleased]

Everything below becomes 0.1.0 once it has run on a watch (Gate 6).

### Added

- **Automatic counting.** Every activity the watch records, from any app, is
  scanned on open and credited to its week — no separate logging step for
  the apps you already use. Manual logging (today or yesterday) covers
  anything the watch never recorded.
- **A weekly target**, default 3 sessions from a Monday, adjustable, with an
  optional minimum duration (10 minutes by default, 0 = off) and an optional
  "one per day" cap so several short sessions can't fill a week alone.
- **Live achievement.** A week steps up the mountain the moment it meets its
  target, not at the week boundary — the boundary only judges a miss.
- **Shields.** One earned per four achieved weeks, capped at two, offered
  automatically on a miss rather than resetting the streak outright. A trial
  week's miss never costs a shield or breaks a streak that hasn't started.
- **The classifier**: Run, Ride, Walk/Hike, Strength, Workout, Hybrid (from
  HybridX Race), Row/Erg or Other, by sport and by which app recorded it.
- **Trophy case**: five summits (climbed at set streak lengths), session
  badges at 10/50/100/250, and the longest streak and best week kept
  alongside the live ones.
- **Screens**: Home (with the moments since last opened played as toasts, a
  step-up, then a summit/shield/fresh-start switch), Menu, This week (with
  why a session doesn't qualify, and leave-out/undo), Log a session, Trophy
  case, Settings, and a Clock-not-set screen for when the watch's time isn't
  usable yet.
- **A glance**: a separate service-only binary that reads the app's public
  summary and projects the week live, in full, compact or tiny layouts
  depending on what the watch reports for the glances screen.
- **Phone-editable settings** through AppConfig: weekly target, week start,
  what counts, minimum duration, one-per-day.
- **Store packaging**: `Utilities/pack-store-zip.sh` builds a validated
  portal zip for the main app.

### Notes

- Automatic counting depends on one app being able to read another's
  `Activity/` folder — unconfirmed on real hardware until Jon runs the Gate
  0 probe (`docs/PROBE.md`). If it can't, everything above still stands
  except automatic counting of *other* apps' files.
- The glance ships side-load/CI-only for now, not as its own store listing —
  see `docs/NOTES.md` S5.1.
- Nothing wakes a service on a schedule on this platform, so there is no
  background scan or reminder; everything recomputes on open. A scheduled
  reminder, if the probe shows the phone doesn't delete synced files, is
  deferred to P1 — see `docs/PLAN.md` §10.
