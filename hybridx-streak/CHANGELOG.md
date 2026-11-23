# Changelog

All notable changes to HybridX Streak are recorded here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and
the project uses [semantic versioning](https://semver.org/spec/v2.0.0.html).

Releases are tagged `streak-vX.Y.Z`; `Software/cmake/streak-version.cmake`
strips the prefix for the SDK's version script.

## [Unreleased]

### Added

- **By sport** (menu): this week's time, sessions and distance from every X app, kind by kind
  ("Run: 2 · 1 h 06 · 11.2 km"), the week in all, and last week's. Everything the apps recorded
  counts here, as for the training load; manual logs show as "By hand".
- **Your numbers** (menu): the rest of the X family in one place:
  - **today's plan** from X Training: today's session and pace, "Done", a rest day with the next
    session;
  - **VO2max** from X Run;
  - **weight** from X Weigh-in: its trend and the week's change, in its own unit.

  Each row says which app fills it ("Needs X Run") until that app has left its file.
- **Home** names X Training's session on a banked week ("Banked. Tempo today") rather than saying
  "Rest up." on a day the plan has a session to do.
- **Training load graphs**: R1 on the Training load screen opens a graph of the row: **fitness
  and fatigue** together over 12 weeks (fitness in cyan, fatigue in orange), **form** about a zero
  line, and the **daily load** over 4 weeks as bars. L1 and L2 move between them. Streak now keeps
  a daily history for them (a new optional part of the saved state); after updating, the graphs
  start with the last two weeks and fill in from there.
- **Form up front**:
  - Home has a second page, **Form** (L1 or L2 from the mountain): your form, large, with its
    word; fitness and fatigue in the graphs' colours; the last four weeks of the two lines. R1
    opens the graphs.
  - **Opens on** (Settings, and the UNA app): open on the Form page instead of the mountain. News
    from your sessions still plays on the mountain first.
  - **Training load** is now the menu's first row.
  - After the sessions an open finds, Home says where your form now stands ("Form now +12,
    fresh").
  - **The glance** shows form on banked weeks too ("Banked. Form +14"); only a shield waiting or a
    week at risk comes before it.
- The store listing gains three previews (By sport, Your numbers, the training load graph), and
  the menu's gives way to the Form page; the glance's listing gains its form line.

## [1.1.1] - 2026-11-20

(1.1.0 was tagged but never published; this is its first release.)

### Added

- X Training's runs count: its summaries are in a sixth slot of the activity link,
  `../SharedData/HybridX/act_training.json`, and count as runs.

### Changed

- This week's list names each app as it is named on the watch (X Run, X Trail, X Ride, X Workout,
  X Training; Race stays "HybridX"), not by its folder name, which ran into the arcs.

## [1.0.0] - 2026-11-09

The first store release (`hybridx-link/docs/STORE_RELEASE.md`). On the watch the app
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
