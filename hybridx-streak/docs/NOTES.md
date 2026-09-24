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
| §2.2 Tier B "only works if app storage isn't sandboxed" | It is sandboxed, but there is a sanctioned shared folder (`../SharedData/`), and reading another app's folder is unverified | Tier B is re-planned as three routes (PLAN §6); the dependable one needs no unverified behaviour |
| §4 Q1 "complication, widget, or glance" as one idea | Three different things. Only the glance fits, and it must be its own `.uapp` | Two packages: main app plus glance |
| §2.2 Tier A "optionally time it with a simple start/stop" | Possible without a resident service: persist the start UTC and derive elapsed time (lifecycle §9) | Timing is deferred to P1, **with** the duration threshold it exists to serve; alone it has no purpose |
| §4 Q2 hopes for a scheduled wake | There is none | Lazy evaluation throughout; any reminder is P1, opt-in, and costs a resident service |
| `CLAUDE.md` "starting from `RunLVGL`" | `RunLVGL` is an `Activity` app; the streak app is a `Utility` | Start from `hybridx-race`'s own LVGL scaffolding (itself derived from RunLVGL and already fixed on our toolchain), stripped of race screens. Needs `CLAUDE.md` updated when the project starts |
