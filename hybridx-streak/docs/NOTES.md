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
