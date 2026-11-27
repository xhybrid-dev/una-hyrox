# X Clock: notes

A watch face for the UNA Watch that shows the HybridX family's data. **Phase F0 (the Clock Probe) is
built and at its gate**; no face has been designed or built yet. Read this at the start of each X Clock
phase, and work only on that phase.

- Probe steps for Jon: `PROBE.md`.
- The concept, shown to Jon on 26 November 2026 (a private page, 64-colour mock-up with ten states and the
  rule list): https://claude.ai/artifact/UyshHJqNEqoQuHCBsqEqjf. It is a concept, not a decision.

## Phases

| Phase | Work | Status |
|---|---|---|
| F0 | The Clock Probe: what a Clockface-type app can do on the watch | Built and verified in the simulator. **Jon's test next.** |
| F1 | The pure core: the face's mood rules, the week view, host tests | After F0 |
| F2 | The face: rim, time, simulator captures, a text-fit audit | After F1 |
| F3 | On Jon's wrist: a week of wear, the wrist subscription's battery cost | After F2 |
| F4 | The store listing | After F3 |

## Jon's wish list (27 November 2026)

What he wants the face to be able to show. Not designed yet; the probe collects what is needed to decide.

1. **VO2max progression.** X Run writes `../SharedData/HybridX/vo2max.json` (version 2: `vo2maxX10`, `runs`,
   `utc`, `beforeX10`, `trend[]`, up to ten runs). The probe reads it with a copy of Run's own glance
   reader (`RunVo2::Glance::makeView`) and draws the trend as bars.
2. **What is planned for today, if X Training is installed.** X Training writes `training_today.json`
   (`TodayFile`): the plan's days from this Monday for 14 days, with each day's `what` and `pace` already
   worded for display. "Installed" can only mean that the file exists with `plan` not 0: a face cannot see
   another app's folder (see "Known" below). The probe reads it with a copy of X Training's own reader.
3. **A ring of seven segments round the face, each filling in for that day's activity level, graded in
   colour from none to low to high.**
   - **"Activity level" is not defined yet, and it is the one real decision.** Three readings:
     1. the family's **training load** per day (`act_*.json`, the eighth number of each entry, Edwards'
        TRIMP; available for the past seven days now, only from our own apps);
     2. the watch's **all-day activity** (steps, active minutes, floors): only today's value is available,
        so a face must record each day itself, from the day it first runs;
     3. a blend.
   - The probe draws a **draft ring from (1)** with a placeholder grading (`ActivityLevel.hpp`: low from 1
     point, high from 100; a session with no heart-rate load counts two points a minute) and **logs (2)**
     every hour in `days.csv`, so the real figures choose the thresholds.
   - Not asked of Jon yet: decide after he has seen both.

## Known before the probe (so it does not repeat them)

- **A face is a TouchGFX app in every SDK example.** It has one screen, no buttons (the kernel owns them),
  no touch. The kernel lists every app of `APP_TYPE Clockface` at boot (`Docs/writing-a-clockface.md`).
  Whether an **LVGL** face is accepted is not documented. That is the probe's first question, and it is why the
  probe is itself an LVGL Clockface.
- **Only `../SharedData/...` is reachable** from an app on the watch; sibling app folders, `..` and absolute
  paths are not (Streak NOTES, Gate 0). `IDirectory::open()` on a missing path may report success, so `exist()`
  comes first. Listing `..` goes to a different volume. Whether **listing `../SharedData`** works is not shown
  (the family reads by fixed name, never by listing); the probe tries.
- **The shipped faces write no files** (`ClockfaceAnalogue`, `Peak` say so). Whether a face can read or write
  at all is unproven, though its service gets the same `Kernel` with `fs`.
- **Daily sensors give no history.** `STEP_COUNTER_DAILY`, `ACTIVITY_TIME_DAILY` and `FLOOR_COUNTER_DAILY`
  give today's value on connect and then on a change (Energy NOTES, Gate SL0 first look).
- **`WRIST_MOTION` is proven in an app** (X Sauna's backlight) and sent nothing in ten still minutes (the
  Energy Probe): it fires on a real raise. A face gets no screen-on signal (`Docs/service-lifecycle.md`
  section 6), so a motion event may be its only way to know the wrist rose. Its battery cost is unknown.
- **A suspended GUI keeps its RAM, and a launch that does not fit fails** rather than evicting anything
  (`Docs/service-lifecycle.md` section 8). The SDK has no free-memory call, so this can only be tested by
  opening the big apps with the face resident (PROBE.md, test B).
- **AppConfig reaches an app through the phone and the store.** A probe installed over USB cannot test it, so
  it is left for the first store-listed build.
- **The display is an LCD with a PWM backlight** (`Docs/architecture-deep-dive.md`): assumed that dark pixels
  save no power. Not measured.
- **A face's service must leave** when its GUI goes (`COMMAND_APP_NOTIF_GUI_STOP`) or after a five second
  grace if none comes up. So the probe stops logging when the user chooses another face.

## What the probe is

`Tools/Probe` (started from the Energy Probe's layout, `x-energy/Tools/Probe`). **App name `ClockProbe`**
("Clock Probe" on the watch), `APP_TYPE Clockface`, version 0.1.0, development APP_ID `D3D9144125A884DD`
(first 16 hex of md5("ClockProbe")); not autostarted. A throwaway tool, never for the store.

- **Service** (`Software/Libs/App`): reads the clock once a turn and sizes its wait from it; on start, on each
  GUI resume (30 s apart at most) and every hour it **scans** the shared folder. It lists `../SharedData` and
  `../SharedData/HybridX`, asks after twelve known files by exact name, hands each to the family's own reader,
  and keeps the full text of a scan only when something changed (a signature of each file's size and time).
  It listens for wrist raises and gestures when the day's mode says so, keeps the GUI's tick reports, reads
  the battery gauge every minute, and writes the watch's day figures once an hour. It sends the screen
  **snapshots** (never deltas: the GUI's queue holds ten and is drained only while on screen).
- **GUI** (`Software/Apps/LVGL-GUI`): one screen, no buttons. A ring of seven `SDK::LVGL::Draw::arc`s on the
  rim (Monday at twelve o'clock; the SDK's arc helper takes the 12-o'clock-clockwise angles the TouchGFX faces
  use), the time, seven rows with a light each, and the VO2max trend as bars. It counts frames with a 1 ms
  LVGL timer (one firing per kernel tick) and reports to the service every 50, and at each resume and suspend.
- **Pure core** (`Software/Libs/Core`, host-tested): `FileProbe` and `Scan` (what the file system says, step by
  step), `WeekView`, `ActivityLevel`, `WristLog`, `FrameWatch`, `WristMode`, `ProbeState`, and `ProbeText` (every
  log row and screen line, integers only). `KnownFiles` is the list of paths.
- **Copies of the family's readers**, byte for byte, checked in CI: `Libs/Link` (`hybridx-link`, by
  `check-link-copies.sh`), `Libs/Training` (X Training's `TodayFile`, `Date`, `PlanTypes`) and `Libs/Vo2` (Run's
  `GlanceView`, `Vo2Config`, `Vo2WindowConfig`) by `x-clock/Utilities/check-copies.sh`. Streak's in-memory
  directory tree for the host tests (`TreeFileSystem`) is a copy too.
- **Not read for content:** `weighins.csv` (X Weigh-in's log), `streak.json` (Streak's state) and `stride.json`
  (the SDK's) are asked about for size and age only; a host test fails if any is ever opened. The watch's
  height and weight are never logged.

### The wrist mode

To measure what listening for a wrist raise costs, the probe subscribes `WRIST_MOTION` and
`GESTURE_RECOGNITION` on days with an even day number and not on odd ones, and reads the battery gauge every
minute (current in hundredths of a milliamp: the whole watch averages about 0.55 mA, Energy NOTES). A
`mode.txt` beside the probe (`on`, `off` or `auto`) pins it. The minute row says which mode it was.

### The logs (`Apps/ClockProbe/Logs`)

| File | Holds |
|---|---|
| `events.csv` | start, stop, the GUI's resume and suspend, every wrist raise (with how long since the GUI last resumed and last reported), gestures, charging, the watch's settings, each scan's one line, failures |
| `scan_<date>.txt` | the listing of both folders, each file's line (exists, size, stamp, read, verdict) and **its real content**, whenever something changed |
| `min_<date>.csv` | a row a minute: the mode, the battery gauge, today's steps / active minutes / floors, the wrist and gesture counts, the GUI's reports, rate and longest gap, resumes, suspends |
| `days.csv` | the watch's day figures once an hour: the history the ring needs |
| `state.txt` | how many times the service has started |

`docs/experiments/probe_report.py <Logs>` turns them into the answers.

## Verified (27 November 2026, in the container)

- **Host tests: 71 pass** (`x-clock/build-tests/x-clock-host-tests`), with the family's real writers making
  the fixtures and the family's real readers parsing them: the scan of a whole pretend watch, of an empty one,
  of one with a corrupt file, the privacy rule (nothing private opened), the week view across time zones and
  week edges, every log row and screen line, and the wrap of the tick clock.
- **A real finding from the tests:** X Training's own reader refuses a `training_today.json` whose rest days
  have `runWeek` 0 (it wants 1 to 255 on every row). My first fixture was wrong, the reader was right.
- **Another:** Run's `makeView` returns false for a valid version 2 file with `runs` 0 (no runs yet), so "no
  runs yet" and "bad file" cannot be told apart from it. The probe's row says "VO2: no data" for both; the
  dump shows the real text.
- **Simulator, five pretend watches** (`docs/experiments/sim_probe.sh [full|noplan|none|bad|empty]`; the
  pretend files come from `make_family`, in the real format, dated to the machine's clock): all checks pass.
  The full watch finds eleven of the twelve known files (the pretend watch has no `streak.json`) and parses the nine it reads, lists both folders, never opens `weighins.csv`, logs three key-press
  raises with their timing, writes a minute row (mode, battery, 9.9 frames a second, 5 s longest gap) and a clean
  stop. A watch with no files, with no HybridX folder, and with every file unreadable are told apart on the
  screen (red, amber, grey) and in the log. Screens: `docs/screens/f0-*.png`.
- **Text fit:** `docs/experiments/audit.py` measures every row's worst case with the real Poppins 14 glyph
  advances against the disc: all fit. It found two real faults, both fixed: a plan line over 20 characters
  reached the ring, and I had allowed twelve VO2max bars where X Run keeps ten.
- **Watch compile** (Ubuntu's `arm-none-eabi-gcc` with syscall stubs: a **compile check, never to go on a
  watch**): the service and GUI link and the package builds, `ClockProbe_0.1.0.uapp` 203 KB. Memory (what
  `arm-none-eabi-size` says; the stack is extra): **GUI 164 KB code, 0.9 KB data, 139 KB working memory,
  plus its 24 KB stack; service 32 KB code, 0.7 KB data, 36 KB working memory, plus its 10 KB stack.** About
  210 KB of RAM while the face is loaded. That is the figure test B (PROBE.md) puts against the big apps.
  The LVGL pool, the 57.6 KB frame and the stripe are most of the GUI's.
- **CI** builds it (`clock-probe`), copies it into `watch-apps`, runs the host tests and the audit, and
  checks the copies.

## Not verified (Jon's gate)

Everything that needs the watch:
- that the face appears in Settings, Watch Face and draws;
- that a face may read `../SharedData/` and write its own folder, and what listing returns;
- that a wrist raise reaches it, and whether the face is ticked while the screen is dark;
- that `BATTERY_METRICS`, the daily sensors and the gesture sensor connect for a face (the simulator has none
  of them, so those connections log as failed there);
- the battery cost of listening for the wrist;
- whether the big apps still open with it resident;
- that refresh-on-resume works (the face re-reads on a resume: the SDK's pattern for a face, never run here
  with real saves).

## Decisions and open questions

- **Name.** The probe is "Clock Probe". The face's name is Jon's: "X Clock" was used in the concept, "X Face"
  is shorter. Not decided.
- **Toolkit.** LVGL, as the family. If the probe does not run as a face, the fallback is TouchGFX from the SDK's
  Retro example, which needs Jon's Windows Designer. Decide only if F0 fails.
- **Activity level.** See the wish list. Decide after F0's logs.
- **Which of the concept's moods ship.** Decide at F1.

## Design notes kept for F1 and F2

- **Refresh on the resume edge.** The family writes its files only when an activity is saved, which ends with
  leaving the app, which resumes the face. So the face needs no timer to stay current: re-read on resume and at
  midnight. The probe's GUI asks the service to resend everything on each resume, as `Docs/writing-a-clockface.md`
  requires.
- **Reuse the family's readers, not new ones.** F1 builds on the copies the probe already holds.
- **Never subscribe to what the face does not draw.** A `HEART_RATE` subscription keeps the optical sensor
  powered; the probe takes none.
