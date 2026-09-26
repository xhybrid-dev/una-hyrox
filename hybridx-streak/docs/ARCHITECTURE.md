# HybridX Streak — Architecture

How the app is put together, and why. For what it is supposed to do, read
`UNA_STREAK_TRACKER_BRIEF.md`. For what was found while building it —
measurements, dead ends, every place the SDK and the brief disagree — read
`NOTES.md`.

## Overview

A streak is a weekly target — 3 sessions a week by default — kept
automatically from whatever the watch already records, from any app. Open
the app and it has already scanned the last two weeks of activity folders,
credited what qualifies, and judged whether the week is achieved. There is
also manual logging, for activity the watch never recorded at all.

What makes this more than a counter is that **crediting happens before
judging**. A found session is filed into its own week (which may not be the
current one — a recovered file, or one skipped mid-recording, can carry an
older start) before any week boundary is evaluated. That ordering, plus a
dedup ring rather than a single high-water mark, is what makes a crashed or
late scan safe to just run again.

The second shaping force is the platform, same as Race's: no MMU, a 256-byte
message pool that drops an oversized send in silence on the watch, a
wrapping millisecond clock, and — specific to Streak — **nothing wakes a
service on a schedule** (`service-lifecycle.md:18`). There is no background
tick. Everything is recomputed on open, which is also why the glance
projects the week live rather than reading a stale save.

## Architecture

Three binaries, not two: the main app (service + GUI), and a separate
glance — a service-only `.uapp` the kernel starts for the glances screen.

```
   HybridXStreakService.elf          HybridXStreakGUI.elf         HXStreakGlanceService.elf
 ┌──────────────────────────┐      ┌──────────────────────┐      ┌─────────────────────────┐
 │ StreakModel  (pure C++)  │      │ Model (GUI-side)     │      │ reads streak.json        │
 │ ActivityScanner, Fit-    │ <──> │ ScreenManager        │      │ (re-)scans, projects to  │
 │ SessionReader, Classifier│ msgs │ 13 screens, WheelMenu │      │ now, never saves          │
 │ AppConfig, StateCodec    │      │                       │      │                           │
 └──────────────────────────┘      └──────────────────────┘      └─────────────────────────┘
      owns state.json                   owns the pixels              owns nothing — a read-only
      + the public streak.json                                       view of the public copy
```

The service owns the truth, same rule as Race. The glance never writes —
it's the "single writer" rule from PLAN §4: only the main app's service ever
saves `state.json` or the public copy.

**The message set** (`Software/Libs/App/Header/Commands.hpp`) follows
Race's two rules — every struct `static_assert`ed against the 256-byte pool
block, and the ID space split by direction:

```
service -> GUI                   GUI -> service
0x01 HOME_VIEW                   0x80 CELEBRATE
0x02 MOMENTS                     0x81 LOG_MANUAL
0x03 WEEK_LIST                   0x82 WEEK_ACTION
0x04 APP_NAMES                   0x83 SHIELD_DECISION
0x05 TROPHIES                    0x84 SET_GOAL
0x06 GOAL_VIEW
```

`MOMENTS` carries up to a handful of `Streak::Events` — the news since the
GUI last heard (a session found, a step up, a summit, a shield offer) —
played once, in order, rather than the GUI inferring what changed by diffing
two views.

## Service

`Software/Libs/App/Header/Service.hpp`, `Sources/Service.cpp`.

### On open

1. the clock, floored at 1 Jan 2026 (`Service.cpp:59` — a `clockOk` flag
   the GUI shows a screen for; nothing is judged without it);
2. load `state.json` (falling back to `.bak` on a corrupt read);
3. read the AppConfig goal;
4. scan — at most 32 new files, the previous week plus the current week,
   stretched back to the week of the last open (at most 8 weeks);
5. credit each found session into its own week, then evaluate the boundary;
6. save `state.json` and the public copy
   `../SharedData/HybridX/streak.json` — the same codec, so the glance can
   read it without knowing anything about the private file;
7. send views to the GUI on `GUI_RUN`.

### The pure core

`Software/Libs/Core/` — `StreakModel`, `ActivityScanner`, `FitSessionReader`,
`Classifier`, `WeekMath`, `Summits`, `StateCodec`, `Goal`, `SafeFile`. No SDK
types beyond the `IFileSystem` interface the scanner and codec are written
against, no heap, clock and "now" always passed in rather than read — which
is what lets the host tests simulate months of weeks instantly, with no
watch and no ARM toolchain.

- **`ActivityScanner`** lists every app folder except its own and
  `SharedData`, looks in `Activity/YYYYMM/`, skips the file named in
  `.recording`, and dates each find from the local time in the filename
  cross-checked against the FIT session's UTC start.
- **`FitSessionReader`** streams one `.fit` through a 512-byte buffer, no
  heap, checking the header, the CRC (via the SDK's `fitCrcUpdate`), and
  decoding only the `session` message's start time, sport, sub-sport and
  timer.
- **`Classifier`** maps (sport, sub-sport, app folder) to one of
  `{Run, Ride, Walk, Strength, Workout, Hybrid, Row, Other}` — the app
  folder wins for known apps, so a HybridX Race file is `Hybrid` regardless
  of the generic sport FIT records.
- **`StreakModel`** holds the applied and pending goal, the committed
  figures (streak, weeks achieved, longest, shields, lifetime, best week,
  badges), the live current week, a 64-key dedup ring, and the 52-week
  history. A week is achieved the moment it meets its target — the climber
  steps up mid-week, as the design shows — and committing at the boundary
  only handles misses (a trial week's miss doesn't break the streak; a
  running streak's miss offers shields before resetting).
- **`StateCodec`** is JSON via the SDK's `JsonStreamWriter`/`Reader`, saved
  through the crash-safe `.tmp` → flush → `.bak` → rename sequence
  (`SafeFile`), loaded with every value clamped and a `.bak` fallback.

### Settings

`Resources/app-manifest.json` declares five `configFields` — weekly target,
week start, what counts, minimum minutes, one-per-day — matched against the
C++ table in `Software/Libs/App/Sources/AppConfigFields.cpp`.
`validate_app_config.py --check --check-bounds` keeps the two from drifting,
same mechanism as Race's.

## GUI

`Software/Apps/LVGL-GUI/`. LVGL v9.5, started from Race's scaffold, the
Summit look (DESIGN.md).

Home plays the service's `Moments` on open — session toasts, then a step
up, then a summit/shield/fresh-start switch if one applies — before
settling on the live view. Thirteen screens
(`gui/src/screens/`): Home, Menu, This week, Log a session, Trophy case,
Settings, a value picker, Clock (the clock-not-set case), plus the Summit,
Shield and Fresh-start moment screens, a confirm (tick/cross) screen and the
shared `ScreenManager`/`Screen` base.

`ScreenManager` deletes the old screen **before** building the new one (the
opposite order from Race's `switchNow()`), because two menu screens alive
together peaked the LVGL pool at 89%; deleting first brought the peak to
62%, fragmentation 2%, across every screen and moment.

## Data

### The public copy

The glance can't share the main app's private state, so the service writes
a second copy of the same `StateCodec` JSON to
`../SharedData/HybridX/streak.json` after every save. The glance only ever
reads this file — it never touches `state.json`.

### The glance

`Software/Libs/Glance/` — a service-only binary,
`HXStreakGlance` (`APP_TYPE Glance`, its own `APP_ID`, no icon —
`app_merging.py` only requires `-normal_icon` for non-Glance types).

On `EVENT_GLANCE_START`: read the public copy, scan at most 4 new files,
project the week to now **without saving** — the single-writer rule means
only the main app's service ever writes state.

`GlanceLayout` (`Software/Libs/Glance/Header/GlanceLayout.hpp`) is a pure
function from the projected view plus the watch-reported area/control
budget to a set of control specs: full (a line-drawn mountain and flag,
plus three lines), compact (two lines), tiny (one line), and a clock-unset
state. Every control stays inside the reported area and budget, and every
text is at most 32 bytes, checked across 8 areas × 6 budgets × 7 states in
the host tests.

## Build

```
Software/Apps/HybridXStreak-CMake/CMakeLists.txt        the main watch build
Software/Apps/HybridXStreakGlance-CMake/CMakeLists.txt  the glance watch build
Software/Libs/libs.cmake                                shared Core/App/Glance sources
Software/Apps/LVGL-GUI/lvgl-gui.cmake                    GUI sources
Software/Apps/LVGL-GUI/simulator/CMakeLists.txt          the PC build (build/, plus build-demo/
                                                          for the S0 design demo)
Tests/Host/CMakeLists.txt                                host tests
```

Everything resolves through `$UNA_SDK`; the app lives entirely outside the
SDK checkout and never modifies it. `<version>` comes from a git tag via the
SDK's version script, matched with the `streak-v` prefix
(`Software/cmake/streak-version.cmake`) rather than Race's `apps-v`, so the
two apps' releases don't collide on the same tag namespace.

Build the main app, the glance, **and** the simulator after every change —
their source lists are maintained separately in `libs.cmake`.

## Simulator

`Software/Apps/LVGL-GUI/simulator/` builds the real app (`build/`) and a
separate design-demo build (`build-demo/`, `-DHYBRIDXSTREAK_DEMO=ON`) side
by side. The mock file system is plain host-directory traversal rooted at
the working directory, so a pretend watch is a scratch tree of real `.fit`
files (`docs/experiments/sim_fixtures.sh`, using the host tool
`make_fit`, which drives the SDK's real `FitWriter`) — run the simulator
from five directories down (`<x>/a/b/c/d/e`) so its own `Activity/` folder
doesn't collide with the fixtures.

The simulator cannot run glances at all, so there's no equivalent build for
`HXStreakGlance` — its layout is instead previewed by drawing
`GlanceLayout`'s real output directly (`glance_preview` +
`docs/experiments/glance_preview.py`, `docs/screens/glance-preview.png`).

What the simulator **can** prove: the scan, the model's rules, the state
file, every screen and moment. What it cannot prove — whether one app can
read another's `Activity/` folder on real hardware, and the glance on a
real display — is `docs/PROBE.md` and Gate 4.
