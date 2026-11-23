# HybridX Streak — Notes

The running log for the streak tracker: SDK findings, decisions, and every place
the SDK and the brief (`UNA_STREAK_TRACKER_BRIEF.md`) disagree. Same conventions
as `hybridx-race/docs/NOTES.md`: evidence first, file:line citations into
`una-sdk/` at the pinned commit `a7a995a1`, nothing claimed that wasn't read.

---

## Exploration pass (24 September 2026)

Brief §5 asks for the six §4 questions to be answered from the SDK before any
scope is committed. All six have an answer. Two change the brief's plan: there
is no background wake at all (§4 Q2), and cross-app data goes through a
sanctioned shared folder rather than through reading other apps' files
(§4 Q3).

### E.1 Q1 — Is there a complication, widget or glance surface?

**Yes: the glances screen. Not a clockface complication.**

| Surface | What it is | Fit for a streak |
|---|---|---|
| **Glance** (`APP_TYPE Glance`) | A service-only app the kernel starts when the user opens the glances screen. It receives `EVENT_GLANCE_START` / `_TICK` / `_STOP` (`Docs/service-lifecycle.md` §3.3) and draws a small form made of **text, image, line and rectangle** controls (`Libs/Header/SDK/Glance/GlanceControl.hpp:185-203`), in 16 fixed colours and 11 Poppins sizes (`GlanceControl.h:46-75`). Width, height and the control budget are reported at runtime by `RequestGlanceConfig` (`Messages/CommandMessages.hpp:573-585`) | **Good.** "2 of 3 this week · 7-week streak" plus a row of rectangles as week beads |
| **Home widget** (`SDK::HomeWidget`) | A label and/or progress bar on the kernel home screen, pushed by a *running* service "for the duration of an ongoing activity" (`Libs/Header/SDK/HomeWidget/HomeWidget.hpp:52-71`). Timer uses it for a live countdown. It is **the kernel's single home-widget slot** (`Docs/Examples/Timer-Architecture.md:149`) | **Poor.** Needs a permanently resident service, and it would hold the one slot other apps use for live activities |
| Clockface complication | No such API found. Clockfaces are whole apps (`APP_TYPE Clockface`) | None |

**One app cannot reliably be both.** `APP_GLANCE_INTF` exists for "a non-Glance
app that genuinely serves glance data" (`Docs/sdk-setup.md:327-330`), but the
lifecycle doc says to set `APP_TYPE Glance` for anything the glances screen
should start and **not** to rely on `APP_GLANCE_INTF` alone
(`service-lifecycle.md:152-155`, §10). No example sets it on a non-Glance app.
So the glance is a **second `.uapp`**, a service-only companion to the main app.

### E.2 Q2 — Can a service wake on a schedule?

**No.** Verbatim: "**Nothing wakes a service because time has passed.**"
(`service-lifecycle.md:18`), and "A periodic wake-up of your own | **No.
Nothing paces a service**" (§4.1).

The only way to make something happen at a time is a service that is already
running and waits with a bounded timeout — permanently, from boot, via
`APP_AUTOSTART On` (Alarm's pattern, §3.2) — and then takes the screen with
`REQUEST_APP_RUN_GUI`. The doc lists what that costs: a wake-up per period, RAM
held for as long as the watch is on, and residency that is explicitly **not
promised** ("Do not rely on being allowed to stay resident with nothing to do",
§1.1). Its own recommendation for state that is "a function of a start
timestamp and the current time" is **recompute on open** (§9).

A streak is exactly that kind of state. The brief's "evaluate lazily" (§2.3)
is not a workaround here; it is the design the SDK tells you to use.
Consequence: **no nudges in the MVP.** An opt-in reminder is possible later, at
the cost above (see PLAN §8).

### E.3 Q3 — Is app storage sandboxed? Can one app read another's files?

**Sandboxed by default, and there is a sanctioned way out of it.**

- Every app has a sandbox root, `/Apps/<AppName>/`. Relative paths resolve
  there; the SDK's own code calls it "the app's sandbox root"
  (`Libs/Header/SDK/AppConfig/AppConfig.hpp:155`,
  `Libs/Source/AppConfig/AppConfig.cpp:529-531`). Two builds of the Files
  tutorial "each keep their own `settings.json` in their own app directory"
  (`Docs/Tutorials/Files/ARCHITECTURE.md:40`). Absolute paths on the volume
  look like `/Apps/Workout/Activity/202607/activity_...fit`
  (`Docs/BLE-File-Transfer-Service.md:20-21`).
- **The SDK itself steps outside the sandbox.** `StrideLut::kDefaultPath =
  "../SharedData/stride.json"` (`Libs/Header/SDK/Calibration/StrideLut.hpp:67`):
  the Running app's outdoor stride calibrator writes it, and the Treadmill app
  reads it. `OutdoorStrideCalibrator.cpp:296` creates the `SharedData`
  directory if it is missing. That is `/Apps/SharedData/`, a folder no single
  app owns, used by first-party code for exactly this job: one app writes,
  another reads.
- **Reading another app's own folder is not demonstrated anywhere.**
  `../Running/Activity/...` would use the same `..` mechanism, so it plausibly
  works, but nothing in the SDK does it and the kernel might refuse it. It
  needs a one-line probe on a real watch before anything depends on it.
- **There is no FIT reader in the SDK.** `Libs/Header/SDK/Fit/` has a writer,
  CRC, profile and base types, nothing that parses. Nor is there any message
  to ask the kernel for its activity history: the full message catalogue
  (`Messages/MessageTypes.hpp`) has no such request, and
  `CommandAppNewActivity` is one-way, app to kernel
  (`CommandMessages.hpp:135-142`).
- But activity apps built on the SDK template name their files with the
  **local start time**: `Activity/YYYYMM/activity_YYYYMMDDTHHMMSS.fit`
  (`Examples/Apps/Workout/Software/Libs/Sources/ActivityWriter.cpp:344-361`,
  using `localtime_r`). A directory listing (`IDirectory::readNext`,
  `IFileSystem.hpp:313`) is therefore enough to *date* a session without
  parsing it. It is not enough to know its type or duration.

**What this does to Tier B.** "Read other apps' files" splits into three
routes of very different certainty. PLAN §6 takes them in order.

### E.4 Q4 — What is the per-app data file size limit?

**No per-app limit exists in the SDK; the limits are self-imposed.**

- The only hard cap is AppConfig's values file: 8192 bytes
  (`AppConfig/AppConfig.hpp:67`, `Docs/app-config-fields.md:759`), which the
  reader holds in RAM whole. It applies to the phone-editable settings file,
  not to an app's own files.
- `IFileSystem` has no quota or free-space call (`IFileSystem.hpp`, whole
  file). The volume is FatFs (`OutdoorStrideCalibrator.cpp:296` comment).
- The SDK bounds its own shared file by choice: `kMaxStoreBytes = 16 KB`
  "to bound transient heap allocation" (`StrideLut.hpp:72-75`). That is the
  real constraint: RAM to read the file, not space to store it.
- Our state is small. 52 weeks of history plus goal and totals come to under
  2 KB as JSON. We cap the reader at 4 KB and treat anything larger as corrupt,
  as `StrideLut` does.

### E.5 Q5 — How does the kernel expose local date and time zone?

**Through standard libc, and it follows zone changes live.**

- `time(nullptr)` returns UTC. `localtime_r(&utc, &tm)` gives the local
  calendar, weekday included. Every clockface does exactly this
  (`Examples/Apps/ClockfaceAnalogue/Software/Libs/Sources/Service.cpp:22-27`,
  then `tm_mday` / `tm_wday` at 165-166).
- `localtime_r` uses the system zone *as it is now*. The SDK says so, and warns
  it "reflects real-time timezone changes and is therefore NOT monotonic"
  (`Libs/Header/SDK/Metrics/MonotonicTime.hpp:85-91`). Travel moves the local
  date immediately.
- `RequestSystemSettings` carries language, units, 12/24 h, date order, HR
  zones and the user's daily step/floor/active-minute targets. It carries **no
  time zone and no first day of the week** (`CommandMessages.hpp:195-216`).
  **The week-start day has to be our own setting** (S1).

Two edge cases the brief's §2.3 hints at ("the same class of edge case"), made
concrete:

1. **Clock not set.** After a flat battery, and before the phone syncs,
   `time()` can be far in the past. The model must refuse to log or roll
   periods while UTC is before a sanity floor, and say so, rather than write
   history into 1970.
2. **Time going backwards.** Flying west, or a manual clock change, can put
   "now" into an earlier local day than the last event. Periods must never
   un-roll: the last evaluated period is monotonic, and anything earlier is
   credited to it.

### E.6 Q6 — Does the manifest support a lighter app type?

**Yes: `Utility`.** `APP_TYPE` is exactly one of `Activity`, `Utility`,
`Glance` or `Clockface` (`Docs/sdk-setup.md:322`). Alarm, Stopwatch and Timer
are all `Utility`. The streak app records no FIT activity, so `Utility` is
right, with a `Glance` companion (E.1).

### E.7 Other findings that shape the plan

- **The platform already has daily goals** (steps, floors, active minutes, in
  `RequestSystemSettings`), and an `ACTIVITY_TIME_DAILY` sensor. It is tempting
  as an auto-logging source, but it only reports *today*, live. An app that
  recomputes on open cannot recover yesterday's value, so that route needs a
  resident service. Not used.
- **The repo question has a precedent.** UNA merged its separate `una-apps`
  repository *into* `una-sdk` (`Docs/una-apps-archive-README.md`,
  `Docs/merge-apps-release-walkthrough.md`), so every example app now shares one
  repo and one `apps-v*` tag family.
- **Version tags are the one real cost of a shared repo.** `una-app.cmake:149`
  hard-codes the `apps-` prefix, so one `apps-v0.2.0` tag would version both
  apps. But `una-app.cmake:141-143` honours an externally set `BUILD_VERSION`,
  and `una-version.sh` takes the prefix as an argument (lines 32-33). A
  `streak-v*` tag family is a few lines in our own CMakeLists, with no SDK edit.
- **Nothing found about shipping two `.uapp`s as one store listing.**
  `deploy.md` and `app-config-json.md` describe one binary per package. Main
  app plus glance means two installs unless UNA says otherwise. Question for
  UNA (PLAN §10).

### E.8 Where the brief and the SDK disagree

| Brief says | SDK says | Resolution |
|---|---|---|
| §2.2 Tier B "only works if app storage isn't sandboxed" | It is sandboxed by default, `..` demonstrably leaves it, and reading another app's folder is unverified | Superseded by Jon's clarification below: automatic detection is now the core, gated on the S0 probe (PLAN §3) |
| §4 Q1 "complication, widget, or glance" as one idea | Three different things. Only the glance fits, and it must be its own `.uapp` | Two packages: main app plus glance |
| §2.2 Tier A "optionally time it with a simple start/stop" | Possible without a resident service: persist the start UTC and derive elapsed time (lifecycle §9) | Timing is deferred to P1, **with** the duration threshold it exists to serve; alone it has no purpose |
| §4 Q2 hopes for a scheduled wake | There is none | Lazy evaluation throughout; any reminder is P1, opt-in, and costs a resident service |
| `CLAUDE.md` "starting from `RunLVGL`" | `RunLVGL` is an `Activity` app; the streak app is a `Utility` | Start from `hybridx-race`'s own LVGL scaffolding (itself derived from RunLVGL and already fixed on our toolchain), stripped of race screens. Needs `CLAUDE.md` updated when the project starts |

---

## Jon's clarification, and what it changes (24 September 2026)

> "This is to take all activities recorded by the watch and automatically
> contribute towards the streak counter. This needs to operate independently
> from the HybridX Race app — but if we can have added benefit between the two
> that's a bonus."

The brief had automatic detection as a stretch (Tier B) and manual logging as
the MVP (Tier A). **That is now reversed.** Automatic counting of every
activity the watch records, from any app, is the product. Manual logging is the
fallback for training the watch did not record. HybridX Race gets no special
treatment: its activities are counted like any other app's, and anything extra
between the two apps is optional in both directions.

That makes one question decide whether the product can exist at all: **can an
app read other apps' activity files on a real watch?** E.9 gathers everything
the SDK can say about it without hardware.

### E.9 Automatic detection — what the SDK shows

**Every activity app writes to the same layout.** All seven SDK activity apps
and HybridX Race construct their writer with the same folder name,
`ActivityWriter(mKernel, "Activity")`: Cycling, HRMonitor, Hiking, Running,
RunLVGL, Treadmill, Workout (each `Examples/Apps/<App>/Software/Libs/Sources/Service.cpp`)
and `hybridx-race/Software/Libs/Sources/Service.cpp:49`. Files land at
`/Apps/<App>/Activity/YYYYMM/activity_YYYYMMDDTHHMMSS.fit`, named with the
**local** start time (E.3). These examples came from UNA's own `una-apps`
repository, "Apps release" tags and all (E.7), so they are very likely the
apps the watch ships with. The S0 probe confirms it by listing `/Apps/`.

**Every one of them writes a FIT `session` with a sport.** All eight
`ActivityWriter.cpp`s set `Session::Sport`. What they write:

| App | sport / sub_sport |
|---|---|
| Running, RunLVGL | Running / Generic |
| Treadmill | Running / Treadmill |
| Cycling | Cycling / Generic |
| Hiking | Hiking / Generic (its code also references Running, Walking, Cycling and Training) |
| Workout, HRMonitor | Generic / Generic |
| HybridX Race | Running / Generic (D2) |

The fields a reader needs are all declared in the SDK's own profile, so no
number is invented: `session` StartTime = 2, Sport = 5, SubSport = 6,
TotalElapsedTime = 7, TotalTimerTime = 8 (scale 1000, seconds)
(`Libs/Header/SDK/Fit/FitProfile.hpp:132-137`). The `Sport` enum values come
from the same file. A third-party app's sport outside that enum maps to
"Other" unless we source its number from the public profile, as with
`wkt_step_name` in Race NOTES 5.11.

**Generic needs a second signal.** Workout and HRMonitor both write Generic.
The app folder name tells them apart (Workout → a workout; HybridXRace →
HYROX), so the classifier uses the sport first and the folder second.

**In-progress recordings are marked.** Each activity folder holds a fixed
`.recording` file naming the `.fit` currently being written
(`Libs/Header/SDK/Fit/RecordingMarker.hpp`, `kFileName = ".recording"`). The
scanner skips that exact file. An incomplete file has no `session` message and
its header size isn't back-patched, so the reader would reject it anyway.

**There is no FIT reader in the SDK (E.3), so we write a small one.** It is a
bounded, streaming parse: keep the definition table for the 16 local message
types, decode only `session`, check the header and CRC, and use a fixed
buffer with no heap. Race's host rig (`docs/experiments/fit_race_sample.cpp`)
already produces byte-faithful FIT from the real writer, so the reader is
tested against real output from the start, and `fitdecode` cross-checks it.

**Cross-app reads: the evidence, and its limits.**

| Evidence | Points to |
|---|---|
| The SDK's own shared file lives at `../SharedData/`, outside every app's folder, written by one app and read by another (E.3) | `..` traversal works on the device, at least into `SharedData` |
| The kernel's file-access flow is: path valid? → select volume → lock → FatFs call. It has **no per-app permission step**, and the apps volume is labelled "Media/Apps" (`Docs/architecture-deep-dive.md:1104-1218`) | No enforcement, but the diagram is descriptive, not a contract |
| The simulator's file system just prefixes the app's folder onto the path, so `..` reaches sibling apps (`Libs/Source/Simulator/Kernel/Mock/FileSystem.cpp:45-48`) | Development and simulator testing will work. **Says nothing about the watch** |
| No SDK code reads another app's own folder | Unproven either way |

The evidence points towards "allowed", but only the S0 probe on Jon's watch
settles it. It stays a go/no-go gate.

**Retention is a second unknown.** The phone's file-transfer protocol has a
DELETE command (`Docs/BLE-File-Transfer-Service.md:67`, 181-183). Nothing says
whether UNA's phone app deletes activities from the watch after syncing them.
If it does, an activity recorded and synced between two looks by the streak
would be missed. Mitigations are in PLAN §5.6; the probe and UNA both need
asking.

**Crash-safe saves have a reference implementation to copy.**
`RecordingMarker::write()` stages to `.tmp`, flushes, rotates the current file
to `.bak`, then renames the temp file into place, "a good copy present at
every crash point" (`Libs/Source/Fit/RecordingMarker.cpp:70-110`). It mirrors a
`Settings::ManagerBase` that the SDK does not ship. The streak's state file uses
the same sequence.

---

## Phase S0: scaffold, probe, watch-safe builds, first look (24 September 2026)

### S0.1 Jon's decisions

| | Decision |
|---|---|
| Look | **Summit climb**: every achieved week is a step up a mountain; badges are summits |
| Colours | Teal and lime |
| Voice | Encouraging coach |
| Rules | Every PLAN §12 recommendation, S1-S15, as written |

The design that follows from them is in `DESIGN.md`.

### S0.2 Watch-safe builds come from CI

The container's only ARM compiler is Ubuntu's, which the SDK calls
incompatible. Container builds link only with injected syscall stubs, so they
are **compile checks only** and must not go on a watch (hybridx-race NOTES 0.4
and 5.20).

`.github/workflows/watch-builds.yml` builds every target the way UNA's own
`apps-ci.yml` does: the same `xanderhendriks/stm32cubeide:16.0` image, ST's
toolchain, no stubs. It uploads them as one artifact, **watch-apps**, holding:

- HybridX Race;
- HybridX Streak (the demo build);
- the Streak glance;
- the Streak Probe.

A second job runs both apps' host tests.

A fresh clone has no `Output/` (it is git-ignored), and the linker writes its
`.map` there. Every CMake project therefore now creates it; the first CI run
failed on exactly that.

### S0.3 Versioning

`una-version.sh` strips only `apps-v`, `sdk-v` and `v`, so a bare
`streak-v1.2.0` tag would reach `app_merging.py` unparsed.
`Software/cmake/streak-version.cmake` handles this without an SDK edit:

1. It runs the script with the `streak-` prefix.
2. It strips `streak-v`.
3. It falls back to `0.0.0-dev` when the result is not X.Y.Z.
4. It sets `BUILD_VERSION` before `una_app_setup_version()`, which honours it.

The probe has a fixed `0.1.0`.

### S0.4 Structure

**Libs split.** It is Core (pure, header-only), App (the service) and Glance
(the glance's service). Two reasons:

- The SDK's service entry point includes exactly one `Service.hpp`, so the
  app and the glance each need their own.
- The GUI ELF links no Libs sources, so anything the GUI shares with the
  service must be header-only.

**The glance** is its own CMake project:
- `APP_TYPE Glance`, no GUI and no icons;
- output to `Output/Glance/`.

The simulator cannot run glances. The S0 glance therefore reports the real
glance area on the watch, in a small line of its own.

**Haptics.** No SDK GUI drives the motor; only services do. The GUI sends
`Celebrate{moment}` and the service plays it (DESIGN §6).

### S0.5 The probe

- **What it is.** `Tools/Probe/` is a Utility app, "Streak Probe"
  (`HXStreakProbe`, `APP_ID DE9CF1F8FFF3776D`).
- **Code.** The checks are pure C++ (`Probe::Runner`) over `IFileSystem`.
- **Output.** It writes `probe.txt` and appends to `probe-history.txt`.
- **Screen.** One screen shows the verdict.
- **Instructions.** Jon's steps are in `PROBE.md`.

**Tests.** The host tests run the probe against a new `TreeFileSystem` fake,
in `Tests/Host/support/`. The SDK's two fakes cannot list directories: the
directories of `InMemoryFileSystem` are always empty, and `FakeFileSystem` has
none. The new fake:
- resolves `..`, `.`, absolute paths and `2:` drive prefixes;
- models FatFs's refusal to rename onto an existing file;
- can block access outside the sandbox, to model a firmware that forbids it.

The same fake will serve the S1 scanner tests.

**Simulator run** (seeded scratch tree with two fake apps): GO. That covers:
- 3 activity files found;
- the newest opened, with its `.FIT` signature confirmed;
- a 300 KB read;
- the SharedData write, read and remove;
- history appended on the second run;
- the probe closing on R2.

The simulator reports rename as "replaces" because it uses POSIX `rename`.
The watch uses FatFs and is expected to refuse. The probe will say which.

**Two simulator facts, for S2's fixtures:**
- Its reported glance area is 240×60 with 32 controls. That is the
  simulator's value; the watch's is still to be measured.
- Its file system root is `../../../../../Output/` from the working directory.

### S0.6 First look: evidence and measurements

- **Screens.** 24 simulator captures, round-masked, are in `screens/`, and
  `streak-demo.mp4` (38 s) shows the animations. All 8 demo scenarios and
  every moment are covered.
- **LVGL pool.** The peak is **47%** of 37.5 KB across every screen and
  moment, against Race's 91%.
- **Design fixes made from the captures:**
  - The headline clipped at the bezel. It was redesigned as a number plus
    words on one baseline.
  - Coach lines longer than 21 characters clipped. They were rewritten.
  - The shield body ran under the R2 cross. It was narrowed.
  - STEEL_DARK read as purple. The far range is now GRAY_DARK.
  - "Summit!" sat too near the edge. It was moved down.
- **Host tests.** 39 pass:
  - `WeekMath`: every week-start day, across 29 February and two year-ends;
  - the ladder;
  - the summit geometry: every step inside the mountain and the safe circle,
    each step higher than the last, and a visible move per step;
  - the coach's line lengths;
  - `TreeFileSystem`;
  - `ProbeRunner`.
- **Local builds**, compile checks only:
  - app `.uapp`: 230 KB;
  - glance: 21 KB;
  - probe: 176 KB.

### S0.7 Open, for Gate S0

- **Gate 0 itself.** Jon runs the probe (`PROBE.md`).
- **Retention.** Does the phone's sync delete activities from the watch? The
  probe's two runs, before and after a sync, answer it.
- **The glance area on the watch.** The probe line and the glance itself will
  both report it.

---

## Phases S1–S4: counting, the service, the screens, the glance (24–25 September 2026)

Built in one run to Gate 3/4, with Gates 1 and 2 checked automatically along
the way (plan approved by Jon, 24 September). **Gate 0 has still to report:**
everything below works in the simulator and host tests; whether the watch lets
one app read another's `Activity/` files is the probe's question
(`PROBE.md`). Manual logging, the rules, the screens and the glance work
either way.

### S1.1 The FIT reader

- **A small parser of our own, since the SDK has none.** `Core/FitSessionReader`
  is a bounded streaming parser with one 512-byte buffer and no heap.
- **What it checks:**
  - the header;
  - a table of the 16 local definitions, honouring byte order, developer
    fields and compressed-timestamp headers;
  - the whole-file CRC, using the SDK's own `fitCrcUpdate`.
- **What it reads:** only the `session` message (mesg 18), fields 2, 5, 6, 7
  and 8, all numbered as in FitProfile.hpp.
- **Evidence:**
  - on HybridX Race's three real ActivityWriter files it reads exactly what
    `fitdecode` reads, for start, sport, sub-sport and timer;
  - a fixture writer drives the **SDK's real `FitWriter`** with
    ActivityWriter's message layout (developer fields and field descriptions
    included), and `fitdecode` reads its output with CRC checking on;
  - hand-built files cover big-endian definitions, compressed timestamps, a
    missing session and undefined local types.

### S1.2 The scan window: a change from PLAN 5.1

PLAN 5.1 had a single high-water mark. It is replaced by **a window**: the
previous week and the current week, stretched back to the week of the last
open (at most 8 weeks). A **dedup ring** decides what is new.

The reason: a file skipped while it was still recording (named in
`.recording`), or recovered after a crash, can carry an older start than one
already counted, and a high-water mark would pass it by for ever.

The ring holds 64 keys, each an FNV-1a hash of the app folder plus the local
start second. When it evicts a key, everything at least that old counts as
seen, so nothing inside the window is counted twice.

### S1.3 The rules: two choices made while building

1. **A week is achieved the moment it meets its target.** The climber steps
   up then and there, as DESIGN 4 and 6 show. Streak, weeks and lifetime are
   the committed figures plus the live week; closing the week commits them.
   An undo below the target takes the step back down.
2. **A week-start change applies at once** (S8). The current week closes:
   achieved if it met the target, otherwise void. Its sessions on days the new
   week covers move over. Target, scope, minimum and one-per-day wait for next
   week, and a new target or scope makes that week a trial.

**Order of news.** A shield offer or reset is played before this week's
sessions: the past first.

### S1.4 The state file

- **Encoding:** JSON through the SDK's `JsonStreamWriter`/`JsonStreamReader`,
  about 3 KB when full.
- **Saving:** the SDK's tmp → flush → .bak → rename sequence; loading falls
  back to `.bak`.
- **Loading:** every value is clamped.
- **Day numbers are stored as unsigned bit patterns.** The SDK writer formats
  `int32_t` with `%ld`, which is right on the watch (32-bit `long`) and wrong
  on a 64-bit host, and "no week yet" is `INT32_MIN`. **Worth telling UNA:**
  `JsonStreamWriter` has the same issue for any negative `int32_t` on 64-bit
  hosts, such as the simulator.

### S2.1 The service

- **On open:**
  1. clock (floor 1 Jan 2026);
  2. load;
  3. AppConfig goal;
  4. scan (at most 32 files);
  5. credit and judge;
  6. save `state.json` and the public `../SharedData/HybridX/streak.json`;
  7. views to the GUI on `GUI_RUN`.
- **Memory:** the model, scanner and buffers (about 15 KB) are statics placed
  at start-up, off the 10 KB service stack.
- **Messages:** there are six service → GUI views, all under 256 B. Views are
  payload structs inside the messages, because `MessageBase` cannot be copied.
- **Phone settings:** `Resources/app-manifest.json` declares 5 `configFields`.
  `validate_app_config.py` passes locally and in CI.
- **Gate 2:** `docs/experiments/sim_fixtures.sh` builds a pretend watch of real
  FIT files around today.
  - The first open counted Run (Running), Hybrid (a HybridX Race file) and Ride
    (Cycling).
  - It listed a 6-minute walk as too short, and skipped the file being
    recorded and a broken one.
  - A second open changed nothing.
  - The public copy was byte-identical.
  - The scan took under 1 ms in the simulator.

### S3.1 The screens

Home plays the service's moments, starting from the view as it was before
them.

New screens use the SDK's `WheelMenu`, as the UNA apps' menus do:
- Menu;
- This week (with "6 min · under 10" and similar reasons);
- leave out / count again / undo;
- Log a session;
- Trophy case;
- Settings and a value picker;
- Clock unset.

**LVGL pool.** Two menu screens alive at once during a switch peaked the pool
at **89%** (Race: 91%). The ScreenManager now deletes the old screen before
building the new one, via a placeholder. Peak **62%**, fragmentation 2%, across
every screen and moment.

**Evidence:**
- `screens/real/`: 20 round screenshots from the real app on a pretend watch
  (`capture_real.sh`).
- `screens/streak-real-walkthrough.mp4` (52 s, captioned; `walkthrough_real.sh`).
  It opens onto 11 weeks with last week missed:
  1. the shield offer;
  2. three sessions from three apps;
  3. "Week complete!", which is the top of Snowdon.
- `make_state` writes such histories.

Video capture now stamps frames with wall-clock time. x11grab had been
compressing time under load, so captions slid off their scenes.

### S4.1 The glance

- **Projection:** it reads the public copy, scans at most 4 new files, and
  projects the week to now **without saving** (the single-writer rule).
- **Layout:** `Glance/GlanceLayout` is pure and host-tested. The layout is
  chosen from the area and control budget the watch reports:
  - **full:** a line-drawn mountain with a flag, plus three lines;
  - **compact:** two lines;
  - **tiny:** one line.
  Every control is inside the area and within the budget, and every text is
  at most 32 bytes, for 8 areas × 6 budgets × 7 states.
- **Colour:** the glance has 16 colours and no lime, so GREEN stands in.
- **Preview:** the simulator cannot run glances, so `glance_preview` +
  `docs/experiments/glance_preview.py` draw the layout's real output
  (`screens/glance-preview.png`, with a stand-in font).

### Open, for Gates 3 and 4

- **Gate 0:** the probe on the watch, as before.
- **Gate 3:** Jon's review of the real screens and the walkthrough.
- **Gate 4:** the app and glance on the watch, from CI.
  - The glance logs its area: `Glance area WxH, N controls`.
  - Does the full layout appear?
- **Not built yet:** the reminder/background scan (P1, and only if the probe
  shows the phone deletes synced files). Also not built: a reset-streak option
  and Race's streak line (PLAN 9).

## S5: packaging (25 September 2026)

### S5.1 "Two manifests, two zips" is not a documented SDK pattern

PLAN §11's original S5 row assumed the glance gets its own store listing,
same as the main app. Two things found while building it say that assumption
doesn't rest on anything documented:

- `Docs/app-config-json.md:92-95` shows one manifest's `"type"` as an array
  (e.g. `["activity","glance"]`) for a **single** `.uapp` serving both roles.
  Separately, `una-app.cmake:389-414` / `app_merging.py:128-129` expose a
  binary-level `-glance_capable` flag, letting a non-Glance app *also* serve
  the glances screen from the *same* binary. Neither matches Streak's shape:
  two independent `.uapp`s, two separate `APP_ID`s.
- **None of the SDK's own six Glance examples** (GlanceHR, GlanceARHR,
  GlanceSteps, GlanceActivity, GlanceBattery, GlanceFloors) ship an
  `app-manifest.json` at all — their `Resources/` folders hold only an icon.
  `deploy.md`'s whole upload flow assumes one `.uapp` → one manifest → one
  zip → one store listing.

This is exactly PLAN §12 Q4 ("Can one store listing install two `.uapp`s, a
main app plus its glance?"), asked of UNA and still unanswered. Rather than
build a store listing against an unconfirmed mechanism, **the glance stays
side-load/CI-only** — installed the same way it already is today (copied
from the CI `watch-apps` artifact), with no manifest and no zip of its own.
S5 ships one manifest and one zip, for the main app, matching Race's already
working pattern exactly. Revisit if/when UNA answers Q4.

### S5.2 Docs and packaging built

- `hybridx-streak/Utilities/pack-store-zip.sh`, adapted from Race's script:
  same staging shape (`.uapp` + `icon.png` + `previews/` + manifest), same
  version-from-binary + validator gate before zipping.
- `hybridx-streak/README.md`, `docs/ARCHITECTURE.md`, `CHANGELOG.md` — new,
  following Race's shape (the root `CHANGELOG.md` is explicitly scoped to
  "HybridX Race" only, and the root `README.md` is Race's own; Streak has no
  such shared role to inherit, so both are package-level here).

## Gate 0: first run on Jon's watch (28 September 2026)

Jon installed the CI build, recorded a walk, and ran the probe five times
(13:01-13:05, no phone sync between). Every run: **NO FILES**. The Streak app
counted nothing either. Evidence: `probe.txt`, `probe-history.txt` and two
photos, in the conversation.

What the report shows:

| Check | Result |
|---|---|
| `list ..` | ok, 4 entries. The scan found two folders: `System Volume Information` and `gps`. The other two entries were presumably `SharedData` (skipped by name) and a file or dot-entry. **No app folders**: not `HXStreakProbe` itself, not HybridX Race, not a built-in app |
| `list /Apps`, `list 2:/Apps` | FAIL (cannot open) |
| `list /` | ok, 3 entries (the probe's own folder, E.3) |
| `list .` | FAIL |
| SharedData | mkdir, write, read back, remove: all ok |
| Rename onto an existing file | refused, destination kept (FatFs): §6.5's save sequence is right |
| Clock | local 13:05, offset +60 min (BST): correct |
| Glance config | 240 x 60, 32 controls |

Reading:

- `..` does **not** reach `/Apps` on the watch. It reaches a folder holding
  `System Volume Information` (which Windows puts at a volume's root), `gps`
  and, it seems, `SharedData`, but no app folders. The simulator's `..` is plain
  host traversal (E.9), which is why S0-S2 could not see this.
- Absolute paths out of the sandbox are refused.
- So, **on the routes the probe tries, one app cannot see another's folder.**
  The verdict should have been BLOCKED, not NO FILES: the probe says NO FILES
  whenever listing `..` succeeds, without checking that what it listed looks
  like apps. To fix in the probe if it is run again.
- **A second probe fault:** `scanApp` reports "0 months" for
  `System Volume Information` and `gps`, so opening
  `../System Volume Information/Activity` returned success on the watch,
  although that folder almost certainly doesn't exist. Either
  `IDirectory::open()` on a missing path succeeds on the watch, or it created
  the folder. Future code must check `exist()` before trusting `open()`. Jon to
  check over USB whether stray `Activity` folders appeared.

Not yet known, and needed before Gate 0 can be closed:

1. The top level of the watch's USB drive (hidden files shown): is it the
   same volume `..` listed?
2. Where the walk's `.fit` actually is on the drive, and which app recorded it.
3. Whether the phone sync step (PROBE.md step 4) changes anything.

Provisional Gate 0 result: **not GO.** PLAN §3's three options apply; the choice
waits for the answers above.

### Gate 0, second look: the USB drive (28 September 2026)

Jon's screenshots of the watch's USB drive, and the walk's `summary.json`:

- **Drive root:** `Apps/` (30 items), `DailyHealth/`, `GPS_EPO/`,
  `System Volume Information/`, `Update/`, `settings.json`,
  `settings.json.bak`.
- **So the probe's `..` is not this volume.** It listed `System Volume
  Information` and `gps`, while this root has `GPS_EPO` and `Apps`. From inside
  an app, `..` reaches some other volume (presumably the kernel's own), where
  `SharedData` lives. Built-in apps (`Apps/Walking`, `Apps/Running`, …) are on
  the USB volume, beside ours, but out of reach. **Cross-app reading is
  blocked: confirmed.**
- **Activity files are deleted after a phone sync.** `Apps/Walking/Activity/`
  holds an empty `202609/` and a `summary.json`. The walk's `.fit` is gone;
  Jon reports the same for Running and the others. This answers PLAN §3
  question 4: even with read access, "scan when opened" would miss every
  activity synced before Streak was opened (§5.6's risk, now real).
- **`summary.json` survives, but holds one activity**: the latest, overwritten
  each time (RunLVGL's `ActivitySummarySerializer` shape). Walk: `utc`
  1790596689 (12:58 local), 346 s, 461 m, 622 steps, HR avg 82 / max 93, one
  lap, plus a hex track map. Not a history, and unreachable anyway.

**Gate 0: NOT GO, closed.** Two independent blockers: the sandbox, and the
deletion on sync. Automatic counting of other apps' activities needs UNA:
either an activity-list API or an "activity saved" event that a background
service can receive. PLAN §3's options stand; Jon to choose.

### Gate 0, third look: `..` works for files (28 September 2026)

Jon found `streak.json` in `Apps/SharedData/` on the USB drive. The Streak app
writes it as `../SharedData/HybridX/streak.json` (`Libs/App/Sources/Service.cpp:32`).
So **for opening files, `..` from an app's folder is `/Apps`**, as the SDK's own
`../SharedData/stride.json` assumes (E.3). Only *listing* `..` landed somewhere
else (`gps`, `System Volume Information`). The listing result was therefore
misleading, and cross-app reading is **not** yet shown to be blocked for a file
opened by its exact path, e.g. `../Walking/Activity/summary.json`.

Its content also shows the app ran on the watch and published a week with no
sessions: `"d":[20724,…]` (day 20724 = 28 September 2026), all counts 0.

Gate 0 reopens on one question: can an app open `../<OtherApp>/Activity/summary.json`
by name? Next: a probe run that tries exact paths, without listing.

### Probe 0.2.0: [7] exact-path reads (28 September 2026)

Built for the reopened question above. **[7]** opens
`<route>/<App>/Activity/summary.json` for each app name, through `..`, `/Apps`
and `2:/Apps` in turn, **without listing a folder**; parses `utc`, `time` and
`distance`; and, where one opens, looks for a `.fit` in that month's folder
(`monthOf(utc)`) and checks its header. Names: a built-in list (Walking,
Running, Cycling, Hiking, Treadmill, Workout, HybridXRace) plus an optional
`apps.txt` in the probe's folder, which accepts folder names only (no `/`,
`\`, `:` or `..`).

[7] now decides the verdict: GO if any summary was read; a listed `.fit` that
opened or refused still stands; otherwise NO READ if a summary was found but
not read, NO FILES if app folders were seen, BLOCKED if nothing was reachable.
This fixes the first run's misleading NO FILES.

- `TreeFileSystem` gained `blockListingOutside()`: listing outside the sandbox
  fails, files still open by name, as on Jon's watch.
- 7 new host tests, including Jon's real walk `summary.json` as a fixture. All
  Streak host tests pass; watch target and simulator build.
- Simulator, with Jon's `summary.json` and `streak.json` in a fake tree: GO,
  "Walking 346 s, 461 m".
- Jon's steps: PROBE.md, "Second run".

### Gate 0, final: probe 0.2.0 on the watch (28 September 2026)

Four runs of 0.2.0 (runs 7-10, 13:48-13:52), before and after a phone sync:
**BLOCKED** every time. History line: `by name 7/0/0/0`: 7 names tried, 0 app
folders seen by `exist("../<App>")`, 0 `summary.json` found or read by any of
`..`, `/Apps` or `2:/Apps`. Shared folder ok, as before.

So the firmware lets `../SharedData/...` through (write, read, remove, and the
Streak app's own `streak.json` landing in `Apps/SharedData/`) but nothing else
outside an app's own folder: not a sibling app's folder, not a file in it by
exact name, not an absolute path. SharedData looks deliberately allowed, not
reached by general `..` traversal.

**Gate 0: NOT GO, closed.** Automatic counting of other apps' activities is
impossible without UNA. PLAN §3's options; Jon to choose.

---

## The UNA Android app (v2.2.8): what sync does (30 September 2026)

UNA supplied Jon with the APK (`com.unawatch`, version 2.2.8). Jon sent the
binary `AndroidManifest.xml`, `assets/index.android.bundle` and screenshots of
the `lib/` and `assets/` folders. Method: `strings` over the bundle. It is
**Hermes bytecode (version 96)**, so only string literals are readable, not the
code. Everything below is inferred from log and identifier strings; none of it
is documented by UNA or was run.

- **Framework:** React Native (`libhermes.so`, `libreactnative.so`,
  `index.android.bundle` 7.47 MB), with Sentry, Firebase, Notifee, Klaviyo and
  AppsFlyer.
- **Manifest:** no Health Connect or Google Fit permissions. It has BLE,
  companion-device, notification-listener and foreground-service entries, and
  a `strava` package query. Whether any component is exported for other apps
  is not readable from the raw binary; needs `aapt dump xmltree`.
- **Sync flow** (log strings, prefixes `[BLE]`, `[Activity Files]`,
  `[Activity Data]`): it connects over the BLE File Transfer Service, lists
  `/Apps`, and checks each app for new activities. It reads a pointer file,
  `latest_activity.txt` (its exact location and format are unknown; our probe
  saw none in `Walking/Activity/`). It lists the app's month folder and
  downloads files named `activity_YYYYMMDDTHHMMSS.fit`. It parses the FIT
  itself, saves the activity, then sends **"Deleting activity file from
  device"**. This confirms the deletion found in Gate 0.
- **History lives off the watch.** `autoActivityUploadEnabled`,
  "activityUpload success!" and `/sync/pull?lastPulledAt=` point to a local
  database in the app's private storage, synced with UNA's backend. A phone
  companion cannot read it.
- **Strava:** the app links to Strava by OAuth (`unawatch://strava/authorize`).
  That is the only export integration seen. `TrainingPeaks` appears only as a
  metric name (TSS). Not evidence of a TrainingPeaks link.
- **Other strings worth knowing:** `/Apps/app_list.json`,
  `/Update/firmware.ota`, `GPS_EPO`, `/settings.json`; FTS protocol version
  read and DIGEST check (agrees with Trail's NOTES).

**Consequences for Streak**
- No phone-side route exists without UNA: no Health Connect, nothing exported
  (unconfirmed), history in private storage.
- Racing the UNA app to the FIT files over BLE would be fragile, Android-only,
  and is not recommended.
- Using UNA's backend endpoints from our own code would rely on an
  undocumented, authenticated interface. Not to be done without UNA's say-so.
- The clean route: UNA writes a small recent-activities summary to
  `Apps/SharedData/` after each sync (the one path the firmware lets apps
  read). Drafted in `UNA_ACTIVITY_REQUEST.md`. Jon to send.
- Open: aapt dump of exported components; where `latest_activity.txt` lives.

## S4.2 The glance's third line (5 October 2026)

**Found on the watch:** the glance's small coach line showed as `?????3?????`.
Only the number in it survived.

**Cause:** the 10 point face holds digits only. The SDK's generated TouchGFX
tables (`Docs/Tutorials/Buttons/.../generated/fonts/src/Table_Poppins_*`, SDK
`a7a995a`) list the same Poppins faces as `GlanceFont_t`:
- `Poppins_Medium_10` has 11 glyphs, `0`-`9` and `?`, and `?` is its
  fallback. So every letter, space and colon drew as `?`.
- Every other face has the 95 printable ASCII characters; SemiBold 18 and
  Italic 18 also have the ellipsis.
- None of them has the middle dot `·` (U+00B7), so the tiny line's dot would
  have drawn as `?` too.
- None of the SDK's six glance examples uses Medium 10.

This is inferred from the tutorial's tables, not from the glance firmware's
own fonts, but it matches the symptom exactly.

The host tests checked only byte length and position, and the preview drew
with a stand-in font, so neither caught it.

**Why not just a bigger face?** The faces with letters are 23 px tall (18
point) or 25 px (SemiBold 20). Three lines need about 69 px; the glance is 60.

**Fix:** "2 of 3 this week" becomes a row of beads, which frees the bottom row
for one 18 point line (Regular 18).
- **Beads:** one per session of the target (1-7), 14×8 px. Done ones are
  green, the rest grey. Both are filled rects, the kind the flag already uses
  on the watch.
- **The bottom line**, in priority order:

  | Situation | Line | Colour |
  |---|---|---|
  | Shield decision waiting | `Shield? Open app` | amber |
  | Target met | `Banked. Rest up.` | green |
  | At risk, last day | `Last day: 1 more` | amber |
  | At risk | `2 more, 3 days left` | amber |
  | On track (Jon's choice) | `Ben Nevis: 14 wks` (the next summit) | grey |

- **Special states:** before the app is first opened, and with the clock
  unset, there are no beads; the line reads "Open it to start" or "in the UNA
  app".
- **Room:** the mountain is now 52 px with a 6 px gap, leaving 178 px of
  words. The longest line, "Arthur's Seat: 4 wks", is 173 px.
- **Budget:** the full layout needs 7 + target controls (at most 14; S4.3 later made it 10 + target, at most 17) and a
  240×60 area. Otherwise the compact layout is used, which still shows
  "2 of 3 this week".
- **Tiny line:** "7 wk streak, 2/3" (the dot is gone).

**Guards:**
- `Tests/Host/support/GlanceFontWidths.hpp` holds the real advance widths of
  Regular 18, Medium 18 and SemiBold 20, copied from those tables.
- `GlanceLayoutTest` now fails if:
  - any text uses Medium 10 or a non-ASCII byte;
  - any full-layout text is wider than its box. This is checked for every
    target, session count, day count and climb step, including two repeat
    Everests.
- `Layout::kMax` went from 12 to 16. A target of 7 needs 14 controls, and at 12
  the last bead would have been overwritten silently. The test now checks the
  full layout's count (7 + target) for every target.
- Trial runs with the coach line back in Medium 10, with the old long
  wording, and with `kMax` back at 12 each failed the tests.
- `glance_preview.py` now draws with the SDK's Poppins when `UNA_SDK` is set,
  and draws Medium 10 as the watch does.

**Verified:**
- 113 host tests pass.
- The glance compiles for the watch (a local compile check with stubs: 37 KB).
  The installable `.uapp` comes from CI.
- **Jon to check on the watch:** that the beads and the line show as in
  `screens/glance-preview.png`, and that 18 point text sits inside its 23 px
  box without clipping.

## S4.3 The mountain: lines do not draw on the watch (6 October 2026)

**Found on the watch** (Jon's photos of the glances list, S4.2's build): the
text, the green heading, the flag and the beads all drew as designed. The
mountain did not. Where its slopes should have been there was a faint short
horizontal stub and a short diagonal beside the "A" of the bottom line.

**Cause (not fully known):** the glance's line control (`GlanceLine_t`: a
start point and a stop point, as we gave it) does not draw a line between
those points on the watch. Nothing in the SDK says why:
- none of its six Glance examples uses a line control;
- `GlanceControlLine.hpp` documents only "start" and "stop";
- the layout test checked that every point was inside the area, which they
  were.

Text and filled rectangles are the only shapes the watch has now been seen to
draw correctly (the flag, the beads, every word).

**Fix:** the mountain is six stacked blocks, 52 px wide at the foot and
narrowing by 8 px a tier to a 12 px white cap, with a 2 px white pole and the
green flag on top. Same footprint as before, so the words still have 178 px.
- The mountain is 8 controls (it was 5), so the full layout needs
  10 + target controls (at most 17), before it was condensed (below). `Layout::kMax` is 20. The watch reports
  32.
- **Guards:** `GlanceLayoutTest` fails if the full layout contains a line
  control, and checks the mountain: each block above and narrower than the one
  below, all centred, the flag standing on the cap.
- The code still supports line specs; nothing uses them. If UNA says how the
  control is meant to be used, the old line-drawn mountain is in git (commit
  `e77060a`).
- **Question for UNA, worth adding to the next message to them:** how do the
  glance line controls work (coordinates relative to what, any minimum
  length, any clipping)?

**Verified:** 115 host tests pass; the glance compiles for the watch. Jon to
check the new mountain on the watch.

**Condensed (Jon's photo of the S4.3 build):** the mountain drew well, but
the glance filled the round screen's full width: the mountain's foot ran into
the list's curved scroll marker on the left, and the words sat close to the
right edge.
- The mountain is smaller: five 5 px blocks, 36 px wide at the foot (it was
  six 6 px blocks, 52 px), with a 9 px pole and an 8×5 px flag.
- Content is held in from both edges: 14 px on the left (it was 4) and the
  words keep 8 px clear on the right. The words have 176 px; the longest line,
  "Arthur's Seat: 4 wks", is 173 px, so no text changed.
- The full layout needs 9 + target controls (at most 16).
- `GlanceLayoutTest` checks the mountain starts at least 12 px in.
- The text sizes cannot shrink: only 18 and 20 point have letters (S4.2). If
  the glance still feels large, the next step is a shorter bottom line, such
  as the summit name alone.


## S4.4 Glances do not click through (7 October 2026)

- **The SDK:** a glance's service gets only `EVENT_GLANCE_START`, `_TICK` and
  `_STOP`. There is no select, tap or button event, and the glances screen
  starts the service and never the GUI (`service-lifecycle.md` 3.3).
- **On the watch (Jon):** SELECT on any glance, ours and UNA's (Steps
  included), does nothing.
- **The Notifications glance does click through,** but it is not an app: it is
  absent from `app_list.json` and from the watch's `Apps` folder, and no SDK
  example has it. It is firmware, so there is nothing to copy.
- **Decision:** the glance stays display-only; the full app holds the detail.
  If UNA ever opens select to third-party glances, ask them (with the line
  control question in S4.3) whether a glance can open its own app.
- **Not done:** rotating pages on the tick, to show more than one view. Jon
  did not want it for now.

---

## SDK v1.6.0 (14 October 2026)

The SDK released v1.6.0 (structured workouts, GPS speed filter, buzzer pitch, shared `:/Workouts`). Our pin is unchanged (`a7a995a1`); ABI 3 / kernel 1.4.0 at both. What it means for this app, and the plan to move the pin, are in `hybridx-race/docs/NOTES.md`, "Amendment — UNA SDK v1.6.0 reviewed" (5.21-5.24).

**Decision (14 October 2026):** no change for v1.6: it needs to see every activity and v1.6 does not widen that. See `hybridx-race/docs/NOTES.md` 5.25.

---

## Activity link: Streak reads the HybridX apps' summaries (27 October 2026)

Phase 3 of the HybridX activity link (`hybridx-link/docs/ACTIVITY_LINK.md`): the
route PLAN §3 called option B, after Gate 0 closed NOT GO. Each HybridX activity
app leaves its last 16 activities in its own `../SharedData/HybridX/act_<slot>.json`;
Streak reads them on open and credits them like any other activity. Run writes
its file already (Run NOTES, "Activity link"), and so does Race (27 October 2026,
Race NOTES, "Amendment — Race tells Streak"; its bytes are a host-test fixture
here), and so does Trail (27 October 2026, Trail NOTES, "Activity link"; also a
fixture, and a test with all three apps' real bytes on one watch). No Streak code
changed for Trail: its slot was read from the start. The bike and workout apps to
come are later phases. `hybridx-link/docs/experiments/family_check.sh` runs Run,
Race and Trail into one shared folder and then this app, and checks each is counted
once (ACTIVITY_LINK.md, "Family check").

- **`LinkReader`** (`Core`): opens each of the fixed names in
  `HybridXLink::kSlotPaths` (run, race, trail, ride, workout; the last two are
  reserved for apps that do not exist yet, so a Streak already released reads
  them), the primary if it decodes, else its `.bak`. It never lists the folder.
  It keeps entries whose local day is in the scan window and that the model has
  not counted, and merges them into the scanner's list oldest first (the model
  needs that order). A full list keeps the oldest and leaves the newest for the
  next open, which see them because they were never remembered.
- **Same activity, two routes.** The simulator can read the `.fit` as well as the
  summary, so a run can arrive twice with the same key (the app folder's hash and
  the local start). `StreakModel::update` credits the first and skips the second,
  because `credit` remembers the key. A host test pins this; no extra code.
- **Minutes** are whole minutes, rounded down, as the FIT reader does. **Kind**
  comes from the summary's sport code; the code table mirrors Streak's kinds.
- **No state change.** Streak keeps no distance or heart rate, so `StateCodec`
  is unchanged (still version 1) and nobody's saved streak is lost.
- **Where**: the app's `open()` between the scan and `update()`; the glance's
  projection too (`Glance/Sources/Service.cpp`), with room for 16 found where the
  scan alone reads 4 `.fit` files (the summaries are small files, so there is no
  reason to hold them to 4). `Software/Libs/Link` is a byte-identical copy of
  `hybridx-link`'s code (CI checks it); `libs.cmake` adds it to Core's lists.
- **Not on a watch yet.** The scan still finds nothing on a real watch (it cannot
  list `..`); the summaries are what Streak counts there, **for the HybridX apps
  only**. UNA's own Running, Walking and the rest stay uncounted until UNA
  answers `UNA_ACTIVITY_REQUEST.md` (still a draft, not sent).
- **Verified.** Host tests: 133 (18 new: `LinkReaderTest`, 13 on the reader and 5
  with the model), also under ASan + UBSan; they run over `TreeFileSystem` with
  listing and parent access blocked, as on the watch. Simulator, on a pretend
  watch of summaries only (`docs/experiments/sim_fixtures_link.sh`, files written
  by hand from the spec): the scan found 0 new, `Summaries: 4 files, 5 entries, 5
  new`; the week counted Run, Run, Hybrid and Ride (a made-up bike app in the
  reserved slot); the 5-minute Workout was listed but not counted; the broken
  `act_trail.json` was skipped; `streak.json` was rewritten. The screen showed
  "Your first week", three dots and "+1 Run · 30 min". Watch targets compile
  (app 280,200 B, glance 41,256 B; compile checks only, not for a watch).
- **Not checked.** Anything on a watch; the glance's projection of summaries
  (the simulator cannot run glances; its code is the app's two lines, compiled);
  a non-zero time zone end to end (the container is UTC; the reader's zone maths
  is covered by host tests).


## The Streak line (28 October 2026)

Streak now leaves `../SharedData/HybridX/streak_now.json` each time it saves. After a save, Run,
Race and Trail read it and every app's activity summary, and show "3 of 4 this week" and
"8-week streak" on their Saved screen, worked out by Streak's own rules. The design, its limits
(the streak is left out when only Streak can know it; no line on a treadmill run) and the checks
are in `hybridx-link/docs/ACTIVITY_LINK.md`, "The Streak line". The code is the link library's
`StreakNow` (copied into each app), and `Service::sendStreakLine()` in each app (in Streak,
`Service::saveStreakNow()`).

## Training load: fitness, fatigue and form (2 November 2026)

Jon's request: bring the family together with a training load like TrainingPeaks', shown in
Streak, with form on the glance. Each HybridX app scores its sessions (Edwards' TRIMP from the
watch's heart-rate zones; `hybridx-link/docs/ACTIVITY_LINK.md` "Training load") and leaves the
score as the 8th number of its summary. Streak builds the rest.

### The model (`Core/Header/LoadHistory.hpp`)

- **Banister's model**, as TrainingPeaks' Performance Manager uses it. Our own names, not
  their trademarks (TSS, CTL, ATL, TSB).
  - **Fitness:** each day, fitness moves 1/42 of the way to that day's load.
  - **Fatigue:** each day, fatigue moves 1/7 of the way to it.
  - **Form** = fitness - fatigue.
  - Integers in hundredths, rounded at each step. A host test holds them within 0.1 of a
    floating-point model over a year.
- **Late sessions.** Streak finds a session when it next opens, which can be days later. So the
  last 14 days stay open as daily loads, and the averages are settled up to the day before
  them. A session older than that goes on the oldest open day.
- **The start.** Fitness takes about six weeks to build and fatigue one, so starting both at 0
  shows a steady athlete as overreaching for weeks. A host test found exactly that. So when the
  first 14 days are all in the window, both averages are **seeded with those days' mean daily
  load**, and form starts near zero.
- **The words** use form as a share of fitness. They are **provisional, for Jon to tune**:

  | Form | Word |
  |---|---|
  | above +10 % | Fresh |
  | −10 % to +10 % | Steady |
  | −30 % to −10 % | Building |
  | below −30 % | Overreaching (on the glance, "ease off") |

  Before 14 days of history the word is "Learning: day N of 14". TrainingPeaks' fixed TSB
  bands are for its own load scale, so they do not carry over.
- **Every automatic session's load counts**, whatever the streak's rules (kind, minimum
  minutes, a session left out), because the body did the work. It enters once, through the
  model's existing dedupe. Manual logs carry none.

### Where it shows

- **Menu:** a "Training load" row, before Settings, with form as its hint ("Form +14
  fresh").
- **Training load screen** (`LoadScreen`, a wheel like the trophy case): Form ("+14, Fresh";
  amber when overreaching), Fitness ("21 · 6 weeks"), Fatigue ("7 · 7 days"), This week's load,
  and Today's.
- **Glance:** when on track and past the warm-up, the bottom line is "Form +14 fresh" (or
  "Form -30 ease off" in amber) in place of the next summit. Shield, banked and at-risk lines
  keep their place. The number is held to three digits there, and there is no comma: "Form
  +9999, building" measured 189 px against the 176 px limit.

### The file

The state keeps the load under a new optional key `"L"`, `[1, firstDay, openStart,
fitnessX100, fatigueX100, seeded, [14 daily loads]]`, with **no version change**. Raising
`kVersion` discards the whole state, so it was not raised. An older file reads with no load
history, and a broken `"L"` is dropped while the streak is kept. The new message `LOAD_VIEW`
makes an update seven messages, still inside the 10-deep queue.

### Verified (2 November 2026)

- **Host tests: 159**, 22 of them new:
  - `LoadHistoryTest` (11): the model against a floating-point reference; late, old and future
    sessions; a long gap; the warm-up; the words; a hard block then a taper reading as Steady,
    then Building or Overreaching, then Fresh.
  - `StreakLoadTest` (8): loads enter once each and despite the streak's rules; a judged week's
    late session; the state file with and without `"L"`, and a broken one.
  - `GlanceLayoutTest` (2): the form line, and its widest case on the full layout.
  - `LinkReaderTest` (1): a summary with a load.
- **The simulator** (`capture_real.sh`, which now also captures the Load screen):
  - a first week shows "Learning: day 1 of 14";
  - eleven weeks of loads from `make_state` (60, 45 and 90 a week) show the menu's "Form +14
    fresh" and the screen's "+14, Fresh" and "21 · 6 weeks";
  - the LVGL pool peak is 62 %, as before.
- **The family check** (all five apps, 23 of 23): every summary now carries a load (Run 4, Race
  4, Trail 4, Ride 3, Workout 0, as the simulator's heart rate gives). These match a hand
  calculation from each session's average heart rate and length.
- **Not checked:** anything on a watch, and the glance's form line drawn on a watch (its widths
  are host-tested against the real font widths).

## App IDs from the portal (9 November 2026)

Jon created the store listings (`hybridx-link/docs/STORE_RELEASE.md`). This app's portal App ID
is now in every place that names it: the watch build's `CMakeLists.txt`, `Resources/app-manifest.json`
and the simulator's `CMakeLists.txt`; the glance's in its CMake file and `Resources/Glance/app-manifest.json`. X Streak `BF63291BD26E20AC` (was `A75434BA114DFEBE`); X Streak Glance `BC8BFA0808314DB6` (was `D9923B245EF1C6FE`). The packager still names the development ID,
only to refuse it on a release. The CHANGELOG says 1.0.0. What is left: the tag `streak-v1.0.0`
(Jon, on GitHub), then the upload.

## X Training joins (15 November 2026; X Training's TP4)

X Training (`hybridx-training/`, training plans) writes its runs to a sixth slot of the activity
link: `Slot::Training`, `../SharedData/HybridX/act_training.json`, sport 1 (run), with its load.
- **Code:** `LinkReader` already opens every slot in `kSlotPaths`, so the code change is the
  library copy (`kSlotCount` 6) and one name.
- **The name:** This week's list showed "41 min · HybridXTraining" into the arcs. `Coach::appName`
  now shows it as "X Training", its name on the watch, as it shows Race as "HybridX". The other
  apps still show their folder names, which fit. Moving them all to their watch names (X Run,
  X Ride, ...) is Jon's call.
- **Tests:**
  - `LinkReaderTest`: every app's file read, in order, X Training's included; X Training's real
    row read as a Run with its load;
  - `CoachTest`: the name;
  - 160 pass.
- **The pretend watch:** `sim_fixtures_link.sh` leaves an X Training run. The simulator read
  "5 files, 6 entries, 6 new" and listed it in full.
- **A release is needed:** a Streak without the sixth slot never counts X Training's runs.
  Release it first, or with X Training: the other apps' Streak line already counts them once
  they are rebuilt.

## S7: the family dashboard (22 November 2026)

Jon's request, for the UNA app competition in November ("showcase the connected ecosystem"): make
Streak more of a central fitness dashboard, bringing together what the X family already leaves in
the shared folder. Jon asked for it to be built straight through, so this plan is recorded here
rather than agreed in plan mode first; the gate is at the end (below, "S7 gate").

### What it adds

1. **Week by sport** (menu row "By sport"): this week's time, sessions and distance per kind, and
   last week's totals beside them. From the activity link's summaries (`act_*.json`), which already
   carry distance (`a[i][3]`) and timer time; Streak ignored distance until now.
   - **Everything the X apps recorded counts here**, as for the training load: the streak's rules
     (minimum minutes, scope, a session left out) do not apply, because the body did the work.
     Manual logs show as sessions with no time or distance.
   - **No state change.** The totals are worked out from the summaries on each open; nothing new is
     saved. Each summary holds an app's last 16 activities, more than a week's worth.
   - On a watch the summaries are the only automatic source. In the simulator the folder scan also
     finds `.fit` files; those without a summary are not in the totals.
   - Distance is in kilometres (no app in the family has a units setting Streak can read).
2. **Your numbers** (menu row): the family's other figures, each shown only when its app has left it:
   - **Today's plan** from X Training's `training_today.json` (its glance's file and reader:
     `TodayFile::pick`), worded as X Training's glance words it: "Tempo 30 min", "Done", "Rest day".
   - **VO2max** from X Run's `vo2max.json` (X Training's range-checked reader, `RunVo2Max`). Never
     beside X Training's paces (X Training's rule).
   - **Weight** from X Weigh-in's shared `weighins.csv` and `weighin_units.txt`, through X Weigh-in's
     own `Trend`, so Streak shows the same trend and weekly change as X Weigh-in's Home does.
3. **Home's coach line** names today's planned session where it would otherwise contradict it.

### How

- `Libs/Family/`: **byte-identical copies** of the readers, kept so by
  `Utilities/check-family-copies.sh` (`--sync` copies the originals over; CI runs it), as the link
  library and the Run link are. X Training's: `Date.hpp`, `PlanTypes.hpp`, `PlanText.hpp`,
  `TodayFile`, `RunVo2Max`. X Weigh-in's: `Calendar.hpp`, `WeighLog`, `WeighProfile`, `WeightUnit.hpp`,
  `WeighText.hpp`, `Trend`, `LogStore`. Change the originals, never the copies.
- `Libs/Dash/` (pure, host-tested; the app only, not the glance): `WeekTotals` (fed by `LinkReader`
  as it reads each summary), `FamilyReader` (the three files into one trivially copyable view) and
  `DashText.hpp` (header-only words for the GUI).
- One new message, `DASH_VIEW` (under the 256-byte pool block): an update becomes eight messages,
  inside the GUI's 10-deep queue.
- The glance is unchanged.

### As built

- **The menu** is now This week, **By sport**, **Your numbers**, Log a session, Trophy case,
  Training load, Settings. By sport's tip is the week's time and distance ("4 h 03 · 44.2 km", or
  "2 by hand" with only manual logs); Your numbers' is today's plan, else the VO2max, else the
  weight, else "From the X apps".
- **By sport**: This week ("5 sessions · 4 h 03"), Distance, a row per kind done, By hand, Last
  week. Kind rows read "2 · 1 h 06 · 11.2 km" with a distance ("2 runs · ..." is 213 px, too
  wide) and "1 workout · 45 min" without.
- **Your numbers** (titled **Numbers** on screen: the SDK's title is a 120 px label and "Your
  numbers" is 127): Today's plan, then Pace (a session today) or Next (after a session done, a
  skip or a rest day), VO2max, Weight, and Trend ("-0.4 kg this week"), or "Last weigh-in · 12
  days ago" when the last is over a week old. With no file from an app, its row says "Needs X
  Training", "Needs X Run" or "Needs X Weigh-in": so the screen always shows where each figure
  comes from, and the wheel (which wraps a short list round) never shows one row twice. X
  Training's states without a session read "Plan paused", "Starts 7 Dec", "Plan complete",
  "No plan yet", and "Open X Training" (amber) when its file is over a fortnight old.
- **Text that fits.** The wheel wraps a long line rather than cutting it, so every wording is
  measured in pixels with the fonts' own advances (`gui/copy/WheelFontWidths.hpp`, generated by
  `Tools/Fonts/wheel_font_widths.py` from the committed fonts; no kerning, so a sum is exact).
  Tips are held to **172 px**: a selected row has about 190 px between the bezel arcs, but the
  row below sits lower on the disc, where Training load's "Learning: day 1 of 14" (189 px,
  shipped since 2 November) loses its first letter. That last is an existing clip, not S7's, and
  is left for Jon to decide. Labels are held to 182 px and titles to 120.
- **Home**: on a week banked at exactly its target the coach said "Week banked. Rest up."; on a
  day X Training still has a session to do it now says "Banked. Tempo today" (or "Intervals
  today", when the longer form passes the coach's 21 characters). A bonus week, any other week,
  and a plan day done, skipped or at rest are unchanged (`DashWords::bankedPlanLine`).
- **Sizes.** `DASH_VIEW` is under the 256-byte block (a static assert). Statics: `WeekTotals` 128
  entries of 16 bytes, `FamilyReader` about 3.6 KB (a 1.5 KB buffer, one plan window, a 90-day
  trend). The service reads the three files in 2 ms in the simulator.
- **The copies** (`Libs/Family`, 18 files) are byte-identical to X Training's and X Weigh-in's;
  CI runs `check-family-copies.sh`. A local arm build warns once inside X Weigh-in's
  `WeighText.hpp` (`%d` given a `long` on the watch's 32-bit `int32_t`); it is X Weigh-in's own
  code, so it is fixed there or not at all.

### Verified (22 November 2026)

- **Host tests: 190**, 30 new:
  - `WeekTotalsTest` (7): sums by kind and in all; seconds summed before minutes; only the days
    asked for; a full list; a kind out of range; sums at their limits; and through `LinkReader`,
    every entry heard, counted or not, in the window or not.
  - `FamilyReaderTest` (9): local days as X Training dates; today's session, done, rest, stale
    and no clock, as X Training's glance picks them; nothing there; all three files; a week of
    weigh-ins equal to X Weigh-in's own trend; a broken file falling back to its `.bak`; figures
    out of range left out.
  - `DashTextTest` (14): every wording, and every label, tip and title within its width, with
    the longest names, the largest sums and all three weight units; Home's banked line for every
    kind of session, and only on a day with one to do.
- **The simulator** (`docs/experiments/capture_dash.sh`, into `docs/screens/real/s7-*.png`; its
  pretend watches from `sim_fixtures_dash.sh`, every file written by hand from its documented
  format): the whole family, a day done in stones and pounds, the link alone, and a banked week
  with a session to do (Home: "Banked. Tempo today", `s7-40`). The service's
  log reads `Summaries: 5 files, 8 entries, 5 new` and `Family: plan yes (state 7), VO2max 523
  (x10), weight 7986 (x100)`. Every figure on By sport was checked by hand against the files
  (two runs, 35 + 31 min, 6.0 + 5.2 km: "2 · 1 h 06 · 11.2 km"). The LVGL pool peak is still
  62 %.
- `capture_real.sh` and `walkthrough_real.sh` walk the longer menu; `capture_real.sh` was re-run
  and every screen checked by eye. Only `04-menu.png` changed (it shows By sport under This
  week). The store previews gain `7-by-sport` and `8-your-numbers`; the packager accepts them.
- **Watch builds compile** (Trail Quest's recipe; compile checks only): the app 299,712 bytes,
  the glance 43,140.

### Not checked, and open

- **Anything on a watch.** On a watch the summaries are the only automatic source; in the
  simulator the folder scan also finds `.fit` files, and those without a summary are not in By
  sport (`capture_real.sh`'s pretend watch has only `.fit` files, so its By sport says "Nothing
  yet" beside "3 of 3 counted"; a watch never shows that).
- **Distance is in kilometres.** No app in the family has a units setting Streak can read. Miles
  would need one (Jon's call).
- The **family check** (`hybridx-link/docs/experiments/family_check.sh`) was not re-run: the link
  and its reader's counting are unchanged; the sink only listens.

### S7 gate (Jon)

1. Look at `docs/screens/real/s7-*.png` and the two new store previews.
2. On a watch, with X Run, X Training and X Weigh-in installed and used at least once: open X
   Streak, then the menu's By sport and Your numbers. Each figure should match what its own app
   shows (X Run's VO2max glance, X Training's Today, X Weigh-in's Home).
3. Decide: the names ("By sport", "Your numbers"), kilometres, and Home's banked line ("Banked.
   Tempo today").

## S8: training load graphs (22 November 2026)

Jon's request: from the Training load screen, press through to graphs of how the figures change,
with **fitness and fatigue on one graph**, drawn as X Weigh-in draws its trend, and the two lines
clearly different on the watch. Agreed in chat; the plan is recorded here and the gate is below.

### What it adds

- **R1 on the Training load screen** opens a graph of the selected row (it did nothing before):
  Fitness or Fatigue opens **Fitness and fatigue**, Form opens **Form**, This week or Today opens
  **Daily load**. L1 and L2 step between the three, R2 goes back.
- **Fitness and fatigue**: the last 12 weeks, a point a day, both lines on one scale from 0.
  - **Fitness: cyan, 3 px, drawn on top. Fatigue: orange, 2 px, underneath.** Blue against
    orange is the pair that stays apart in the common kinds of colour blindness; the two also
    differ in brightness (cyan is about twice as bright on the display) and in thickness, so they
    can be told apart even without colour. The legend under the graph is set in the same colours.
  - Orange is the SDK's `ORANGE_DARK` (C0 40 00), not Streak's amber `YELLOW_DARK`, which means
    "at risk" (DESIGN 2).
- **Form**: the same 12 weeks, one white line about a zero line.
- **Daily load**: the last 4 weeks, a bar a day.
- During the first two weeks the line graphs say "Learning", as the Form row does; the bars show.

### How

- **A daily history** (`LoadTrend`, in the state under a new optional key `"H"`, no version change,
  as `"L"` was): 84 days of fitness and fatigue (tenths) and the day's load, recorded as each day
  leaves the 14-day open window. The open days are replayed when asked. An older file reads with an
  empty history, so for anyone updating the graphs start with the two weeks in the window and fill
  from there.
- **On demand**: the GUI asks when a graph opens (`LOAD_SERIES_REQUEST`) and the service answers
  with three `LOAD_SERIES` messages (fitness, fatigue, load; 84 values each, under the 256-byte
  block). The update on open stays at eight messages.
- **Drawn as X Weigh-in's graph**: LVGL lines (`LV_USE_LINE` is on in the SDK's build; the chart
  widget is off), its plot geometry (x 80 to 209, y 60 to 159, clear of the bezel arcs), grey guides
  at the top and bottom of the scale, the scale's ends on the left, the dates underneath. The
  glance's line control, which does not draw on a watch (S4.3), is not involved.
- `GraphMath.hpp` (Core, header-only): the scales and the pixel mapping, host-tested.

### As built

- **The screens.** The Training load screen shows R1 (amber) now; R1 on a row opens its graph
  (`GraphScreen`). Titles: **Training load** (fitness and fatigue: "Fitness, fatigue" is 134 px
  and the title box 120), **Form**, **Daily load**. Under the plot: the dates ("31 Aug - 22
  Nov"), then the legend in the lines' own colours, its numbers the Training load screen's own
  (so the two screens agree): "Fitness 30" in cyan over "Fatigue 34" in orange; "Form -4,
  Building" (amber when overreaching; the number alone if the word does not fit); "This week 244"
  over "Today 95". Three dots at the foot mark the page, as X Weigh-in's do.
- **The plot** is X Weigh-in's (x 80 to 209), 2 px higher and 8 px shorter (y 58 to 149) to leave
  room for two legend lines low on the disc. Its scale labels are in a box 4 px wider than X
  Weigh-in's (46 px: the daily load's largest possible scale, "80000", is 44 px).
- **The scales** (`GraphMath`): fitness and fatigue share one from 0 to a round number above the
  higher of the two (at least 10); form is the same distance each way from 0 (at least 5), with a
  grey zero line a shade lighter than the guides; the daily load runs from 0 (at least 50). The
  round numbers are 1, 1.5, 2, 3, 4, 5, 6 and 8 times a power of ten, so a peak of 61 tops out
  at 80, not 100.
- **The history**: 84 days of three numbers, 504 bytes in the state (and so in the glance's copy
  of it, which ignores it). The worst-case state file, a year of everything with a full history
  of the largest numbers, is **3,819 bytes**: inside PLAN 6.5's 4 KB, and kMaxBytes is 6,144.
- **An updating athlete** gets the two open weeks at once, plus each day that leaves the window
  from then on; the months before cannot be rebuilt (the averages were kept, not the days).
- `make_state ... block` writes a training block (three weeks building, an easy week, four
  harder with a fourth session, an easy week, a taper) for the captures.
- Not in the glance: it has no room, and the glance's line control does not draw (S4.3).

### Verified (22 November 2026)

- **Host tests: 207**, 17 new:
  - `LoadTrendTest` (16): nothing recorded until a day leaves the window, and never a filler
    day before the first; each settled day equal to the model replayed from scratch to it; 12
    weeks kept; a gap starting again and a years-long one clearing; tenths rounding and holding;
    the series ending today with today's figures and each day's load; a new athlete's days only;
    an updated athlete's two weeks; the model recording and the file keeping it and carrying on;
    an older file, a broken history and one ending on the wrong day all read with the streak
    kept; the worst-case file in bounds; and `GraphMath`'s round numbers, scales and pixels.
  - `DashTextTest` (1 more): every title, legend, date span and scale label within its width,
    measured with the fonts' own advances (`WheelFontWidths.hpp` now also has Regular 16 and 14).
- **The simulator** (`docs/experiments/capture_graph.sh`, into `docs/screens/real/s8-*.png`; all
  three checks pass): sixteen weeks of a block (the service sends "84 days", the history keeps
  84), a first week ("Learning", one day of bars), and a file from before S8 (the lines start at
  the open weeks plus the days since the save). Every page by key presses, L2 round all three and
  back to the first, R2 back to Training load. The LVGL pool peak is still 62 %.
- `capture_real.sh` re-run; `11b` and `27` show the R1 mark. The store previews gain
  `9-load-graph`; `4-training-load` is refreshed.
- **Watch builds compile** (compile checks only): the app 306,304 bytes, the glance 43,772.

### Not checked, and open

- **Anything on a watch**, the lines above all. X Weigh-in's graph, drawn the same way, has been
  on Jon's watch since October, but no note records that its Graph page was looked at there.
- Whether the colours read apart on the watch's own display in daylight (the simulator shows the
  same 64 colours, but not the panel).

### S8 gate (Jon)

1. Look at `docs/screens/real/s8-*.png`.
2. On the watch: open X Streak, Menu, Training load, then R1 on Fitness. Check that the cyan and
   orange lines are easy to tell apart, outdoors as well as in. L2 to Form and Daily load, R2 back.
3. Say whether the 12 weeks, the titles and the legends are right.

## S9: form and fitness up front (23 November 2026)

Jon's request: make the training load data front and centre, easier to see and reach. Of the six
options offered he chose all six; this phase does the five inside Streak, and S10 (below) the one
across the family. Agreed in chat; the gate is at the end.

### What it does

1. **A Training load page on Home.** L1 and L2 on Home (which opened the menu, as R1 does) now
   turn between two pages: the mountain, and **Form**: form large with its word, fitness and
   fatigue in the graphs' cyan and orange, and a 4-week graph of the two lines. R1 on it opens the
   full graphs; R1 on the mountain still opens the menu; R2 on either leaves the app. During the
   first two weeks it says "Learning: day N of 14".
2. **Opens on.** A setting, on the watch (Settings) and in the UNA app ("Open on Training load",
   off by default): open on the Form page instead of the mountain. Anything Home has to play first
   (a session found, a week banked, a summit, a shield offer) still plays on the mountain, and the
   app stays there.
3. **Training load first in the menu**: Training load, This week, By sport, Your numbers, Log a
   session, Trophy case, Settings.
4. **Form on the glance every day it means something.** The bottom line's order becomes: a shield
   waiting; the days left when the week is at risk (the streak's call to act); then form whenever
   it is known, banked weeks included ("Banked. Form +14"); else the next summit. Until two weeks
   of history, the summit, as now.
5. **Home's arrival messages say what the session did to form**: after the "+1 Run · 42 min"
   toasts, one more, "Form now +12, fresh" (shorter forms when it would not fit), when a session
   found on this open carried a load and the warm-up is over.

### How

- The page is its own screen (`FormHomeScreen`), so the mountain's LVGL memory is freed before it
  is built. It asks for the series as the graphs do.
- The setting is AppConfig's sixth field, `opensOnLoad` (bool), in the manifest and the C++ table
  (CI checks they agree). The service sends it in `HomeView`'s flags with a second flag, "moments
  are coming", so the GUI opens on the Form page only when Home has nothing to play.
- The form toast is a new event, `FormNow` (a: the word, b: form), added by the model after the
  open's sessions; it is the first to give way when the eight-event list is full.

### As built

- **The Form page** (`FormHomeScreen`; `s9-04`): "Form" small at the top, the number in the
  60-point face (it has the signs and the digits), the word under it, fitness and fatigue as
  cyan and orange numbers with grey names, and the last four weeks of the two lines. Amber when
  overreaching, as everywhere else. The figures are the Training load screen's (`LOAD_VIEW`), so
  the screens agree. Learning: "Learning" and "Day 1 of 14" in the number's place (`s9-30`).
- **The small graph** is scaled to its own low and high (`GraphMath::ownRange`), not from zero:
  from zero, a month between 30 and 40 drew as a flat line. Both lines are 2 px (at 1 px the
  orange all but vanished, `s9` first capture); the colours carry the difference.
- **Home's marks** now show L1 and L2 (white), which turn to the Form page; R1 still opens the
  menu. The graphs' R2 goes back to where they were opened from (`Model::graphReturn`).
- **Opens on**: Settings' last row, R1 switches "The mountain" and "Form"; the phone's field is
  "Open on Training load". `HomeView`'s flags carry it (`kOpensOnLoad`) and "moments are coming"
  (`kMomentsComing`): the GUI goes to the Form page on the first view only when the setting is on
  and there is nothing to play, no clock problem and no shield decision.
- **The toast**: "Form now -34, ease off" in amber after the run's "+1 Run · 45 min" (`s9-01`,
  `s9-02`); "Form now +12, fresh" in white otherwise; shorter forms within the coach's 21
  characters. No buzz.
- **The glance**: "Banked. Form +14" in green, amber when overreaching
  (`docs/screens/glance-preview.png`, and a fourth glance store preview, `4-form.png`).
- **The store previews**: the menu's (6) gives way to the Form page.

### Verified (23 November 2026)

- **Host tests: 214**, 7 new: the glance's banked form line, its colours and its widest case; the
  form event after an open's sessions (its place after the last toast, its figure equal to the
  view's, none while learning or without a load, the first to give way when the list is full);
  the toast within the coach's line; the small graph's own range.
- **The simulator** (`docs/experiments/capture_form.sh`, all four checks pass; `docs/screens/real/
  s9-*.png`): a second open finding one run plays its toast and then the form toast; L1 to the
  Form page, R1 to the graph, R2 back, L2 to the mountain, R1 to the menu with Training load first;
  "Opens on" from the phone: the first open has news and stays on the mountain, the second opens on
  the Form page; Settings switches it and `app_config.json` is written with `"opensOnLoad":true`;
  a first week's Form page says "Learning".
- **Every other capture re-run** for the new menu (`capture_real.sh`, `capture_dash.sh`,
  `capture_graph.sh`): every walk lands on its screen; all their checks pass; the LVGL pool peak is
  still 62 %. One existing fragility, not S9's: `capture_real.sh`'s scenario B needs three sessions
  in the week of the run, so on a Monday its summit never comes and its last three screens land
  one press early (23 November was a Monday). Its committed screens 20 to 27 are from an earlier
  day and were kept.
- The SDK's `validate_app_config.py` passes with six fields; both packagers pass.
- **Watch builds compile** (compile checks only): the app 309,552 bytes, the glance 44,156.

### Not checked

- Anything on a watch: the Form page, the toast, Opens on from the UNA app, the glance's new line.

### S9 gate (Jon)

1. Look at `docs/screens/real/s9-*.png` and the glance mock-up.
2. On the watch: from the mountain press L1 (the Form page), R1 (the graphs), R2, L2 (back).
3. In the UNA app, turn on "Open on Training load"; open X Streak twice (the second time with no
   new sessions) and check it opens on the Form page.
4. Save a session in any X app, then open Streak: after its toast, "Form now ...".
5. Look at the glance on a banked week.

## S10: form on the family's Saved screens (23 November 2026)

The sixth of Jon's six options (S9): form and fitness in every activity, not only in Streak. After
a save, each X app's Saved screen says where form now stands, under the session's load.

### As built

- **Streak's load model moved into the link library**: `LoadHistory.hpp` and `FormText.hpp` (now
  `LoadFormText.hpp`, because X Run and X Training each have a `FormText.hpp` about running form)
  are `hybridx-link`'s originals, copied into every app with the rest of the link
  (`check-link-copies.sh`, CI). Streak reads them from its own `Libs/Link` copy; their namespace
  stays `Streak`.
- **Streak leaves its load window** in `streak_now.json` (`saveStreakNow`): the optional `at` (the
  local second of the save) and `lw` (its `LoadState`). Version 1 still: older readers ignore them.
  The widest file is 626 bytes; the limit is now 896 (within the apps' 1 KB read buffers).
- **Each app projects form** (`StreakProjector`): Streak's window, plus the loads Streak has not
  counted, replayed to today by `LoadHistory`; `formatStreakForm` words it ("Form now -18,
  building", within 21 characters, as Streak's own toast: `Coach::formToast` now calls the same
  `formatFormNow`). Nothing while Streak is learning, or with a Streak from before S10.
- **The six apps**: `StreakLineUpd.form[24]`, filled in `sendStreakLine()` and logged as `Streak
  form: "..."`; the model keeps it (`streakForm()`); the Saved screen shows it with the load: a
  Regular 16 line at y 134 between the load and the Streak line (Run, Race, Trail, X Training; in X
  Training after its plan lines, with the load), or the load's own small label's second line
  (Workout, Ride). Only with a load, as the load itself.
- **Workout and Ride word it within 18 characters** ("Form +12, fresh"), not 21: their load label
  sits at y 174, and two lines of "Form now +100, steady" (163 px in Regular 14) reached 116 px from
  the centre, past the arcs' clear radius of 109 (their audits caught it). With the form line the
  Streak line moves up to y 112 and the load to y 164; the widest at 18, "Form +1000, steady", is
  139 px and its corners about 105 px out.
- The details, the limits and the dedupe rule are in `hybridx-link/docs/ACTIVITY_LINK.md`, "Form
  after a save".

### Verified (23 November 2026)

- **Host tests**: `hybridx-link` 58, with the new "StreakNowForm" suite (the window's round trip;
  an older file; a broken window dropped and the rest kept; the widest file 626 bytes of 896;
  form equal to Streak's model plus the new sessions; what Streak counted not counted again; a
  session after `at` counted, one before it not; nothing while learning; the words' length at 21
  and 18). Streak 214 (its toast now from `formatFormNow`, same words). Run, Race, Trail, Workout
  (125), Ride (112) and X Training pass.
- **The family check** (`family_check.sh`, quick mode): Streak's first summary given a mature
  window, then each of the eight saves' "Streak form" equal to a separate copy of the model plus
  the sessions saved so far; every earlier check still passes (counts, kinds, loads, fitness,
  fatigue and form, X Run's VO2max). Passed with every app at 21 characters, and again (46 of 46)
  with Workout and Ride at 18: X Training "Form now +3, steady", Run and Race "+2", Trail "+1",
  Workout's three and Ride "Form +1, steady", each equal to the reference.
- **The screens**: X Run's Saved screen (`docs/screens/real/s10-run-saved.png`) and X Ride's
  (`s10-ride-saved.png`, from Ride's capture, all its checks passing; `r1-25` and `r1-54` refreshed).
  X Training's, Workout's and Ride's text-fit audits: 0 problems, the widest form included.
- **Watch builds compile** (compile checks only): X Run 523,084 bytes, Race 385,788, Trail 461,112,
  Workout 333,084, Ride 338,036, X Training 572,136; Streak's app and glance.
- `check-link-copies.sh`: the seven apps' copies match, `LoadHistory.hpp` and `LoadFormText.hpp`
  included.

### Not checked

- Anything on a watch. In particular the order on a real save: the line is worked out after the
  `.fit` closes and the summary is written, as the Streak line always was.
- Race, Run and Trail have no text-fit audit: their form line is a Regular 16 label across the
  disc at y 134, near the centre, and the widest (21 characters, about 170 px) was checked by eye.

### S10 gate (Jon)

1. Look at `docs/screens/real/s10-*.png`.
2. On the watch, after Streak has two weeks of history: open X Streak once (it leaves its window),
   then save a session in any X app. The Saved screen shows the load and "Form now ..." under it.
3. Open X Streak: its form toast should give the same figure (it may differ by a point if the save
   was very short, as Streak rounds the day's load once).
4. With a Streak from before this update, the apps show the Streak line and the load, and no form,
   until Streak has been opened once.

## Release 1.2.0 (23 November 2026)

X Streak and X Streak Glance 1.2.0 carry S7 to S10 (tag `streak-v1.2.0`; the steps, the checks and
the portal's release notes: `hybridx-link/docs/STORE_RELEASE.md`, "The Streak 1.2.0 update"). The
apps on the store need no update for it: their released decoders read the new `streak_now.json`,
and the released glance the new state, as before (checked by building each from its tag). Form on
their Saved screens comes with each app's next update.
