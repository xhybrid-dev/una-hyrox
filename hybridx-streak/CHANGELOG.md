# Changelog

All notable changes to HybridX Streak are recorded here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and
the project uses [semantic versioning](https://semver.org/spec/v2.0.0.html).

Releases are tagged `streak-vX.Y.Z`; `Software/cmake/streak-version.cmake`
strips the prefix for the SDK's version script.

## [Unreleased]

The first store release, 1.0.0 (`hybridx-link/docs/STORE_RELEASE.md`). On the watch the app
list calls it **X Streak**; its folder stays `HybridXStreak`. Its glance is its own listing,
**X Streak Glance**.

### Added

- **Training load**: each session from the family carries its heart-rate training load
  (Edwards' score), and Streak turns the days into fitness (42 days), fatigue (7 days) and form
  (the difference), on a Training load screen; form's word (Fresh, Steady, Building or
  Overreaching) shows after two weeks of history, and on the glance when the week is on track.
- **The HybridX apps' summaries.** Each HybridX activity app (Run now; Race,
  Trail and later apps next) leaves its last 16 activities in the shared folder
  and Streak counts them on open, in the app and the glance. On a real watch this
  is the route that works: Streak cannot read other apps' activity folders (Gate
  0), so until UNA lets it see the built-in apps, only the HybridX apps count
  there. Slots for a bike app and a workout app are reserved already.
- **Automatic counting.** Activities are scanned on open and credited to their
  week. On a real watch that means the HybridX apps' summaries (above): Streak
  cannot read other apps' activity folders there, so the folder scan finds
  activities only in the simulator. Manual logging (today or yesterday) covers
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

- On a watch Streak counts the X family's sessions (X Run, X Trail, X Ride, X Workout and
  HybridX Race), which each leave a summary in the shared folder, and manual logs. It cannot
  read other apps' activity folders (Gate 0), so UNA's own apps are not counted until UNA
  offers a way.
- Nothing wakes a service on a schedule on this platform, so there is no background scan or
  reminder; everything recomputes on open.
