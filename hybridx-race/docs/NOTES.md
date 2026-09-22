# NOTES — HybridX Race

Running log of findings, decisions, versions and discrepancies.
Newest phase at the bottom. Every claim here cites a file path or pasted
command output; anything unverified is marked **UNVERIFIED**.

---

## Phase 0 — Orientation and environment (21 September 2026)

### 0.1 Status

Phase 0 complete except for **Gate 0**, which cannot be run: Jon has no watch.
Jon's decision: on-watch gates are **deferred, not failed**. Verification moves
to host tests, the PC simulator and FIT decoding; every hardware check is
recorded in `ON_WATCH_TESTS.md` instead of being skipped.

Jon has confirmed the §7.1 station table as correct, so Gate 1 is unblocked.

### 0.2 SDK commit and version

| Item | Value | Evidence |
|---|---|---|
| Commit | `b0f8955e41505486be3b0a8c81af3628584cc4f0` | `git log -1` |
| Date | 2026-09-17 16:30:37 +0100 | `git log -1` |
| `git describe` | `apps-v1.5.0-rc4-3-gb0f8955e` | `git describe --tags` |
| Branch | `main` | — |
| LVGL submodule | `v9.5.0` (`85aa60d1`) | `git submodule status` |
| Kernel ABI | `KERNEL_INTERFACE_VERSION 3` | `Libs/Header/SDK/Interfaces/IKernel.hpp:37` |
| `minKernelVersion` floor | **1.4.0** | `min_kernel_version.py --print` |

**We cannot pick an SDK tag.** `Examples/Apps/RunLVGL` is absent from
`sdk-v1.2.0`, `sdk-v1.3.0` and `sdk-v1.4.0`; LVGL landed on `main` on
2026-09-16, five days after the newest tag:

```
$ for t in sdk-v1.2.0 sdk-v1.3.0 sdk-v1.4.0; do
    git ls-tree -d --name-only $t Examples/Apps/RunLVGL; done   # all empty
$ git log --diff-filter=A --format="%ci %h %s" -1 -- "Examples/Apps/RunLVGL/*"
2026-09-16 10:16:31 +0100 30c535bf feat(runlvgl): PC simulator, ...
```

So the brief's Phase 0 instruction to "list SDK tags and pick the commit
compatible with Jon's kernel" is not achievable while we use LVGL. **We pin the
commit above.** When Jon gets a watch, its kernel version must be checked
against the 1.4.0 floor before anything is installed.

Two commits on `main` after `sdk-v1.4.0` matter to us directly:
- `b3eec0d6 fix(apps): number laps with their own message_index` — before this,
  every lap in a multi-lap file claimed `message_index` 0. We depend on correct
  lap indices, so this fix is required, and it exists only on `main`.
- `35b9a43c fix(apps): end the activity at the pause, not at the save` — trims
  the trailing UI pause out of elapsed time. Relevant to our pause accounting.

### 0.3 Environment verified in this container

| Tool | Version | Note |
|---|---|---|
| CMake | 3.28.3 | SDK needs ≥ 3.21 |
| Ninja | 1.11.1 | simulator uses `-G Ninja` |
| Python | 3.11.15 | + `pyelftools`, `Pillow` (packaging), `fitdecode` (validation) |
| `arm-none-eabi-gcc` | 13.2.1 (Ubuntu `15:13.2.rel1-2`) | **not the supported toolchain — see 0.4** |
| SDL2 | 2.30.0 | simulator |
| Xvfb / ImageMagick / xdotool | — | automated screenshots, see 0.8 |

`UNA_SDK` must be exported; it is not persisted anywhere, so every build command
sets it explicitly.

### 0.4 Toolchain: the distro ARM compiler does NOT work unmodified

`Docs/sdk-setup.md:16` warns that the system `gcc-arm-none-eabi` is "often
incompatible (newlib syscall stubs such as `_write` can be missing)".
**Confirmed exactly, with the predicted symptom:**

```
ld: .../libc.a(libc_a-writer.o): in function `_write_r':
    undefined reference to `_write'
... same for _close, _read, _lseek
collect2: error: ld returned 1 exit status
```

Root cause, traced with `ld -t`: `Libs/Source/AppSystem/linker/Main/Sections.ld`
resolves libc calls to addresses the kernel exports
(`linker/LibC/libc_exports_0.0.3.ld`, `PROVIDE(printf = 0x0802A371)` etc.) and
then `/DISCARD/`s `libc.a`, `libm.a` and `libgcc.a` (`Sections.ld:140-145`).
The kernel exports **no** `_write` / `_close` / `_read` / `_lseek`
(`grep -c` over the exports file returns 0). ST's toolchain does not pull those
newlib reentrant wrappers into the link; Ubuntu's does, despite `-nostdlib`.

**Workaround used here (development only):** four `ENOSYS` stubs compiled
separately and injected via `-DCMAKE_EXE_LINKER_FLAGS=<stubs.o>` at configure
time. No SDK file was touched. The code that would call them is discarded by the
linker script, so they are link-time filler only.

> ⚠️ **This build must not be treated as installable.** It was produced with the
> toolchain the SDK documents as incompatible. Before anything goes on a watch,
> rebuild with **STM32CubeIDE or STM32CubeCLT's `arm-none-eabi-gcc`** and confirm
> the stubs are no longer needed. Recorded as **W1** in `ON_WATCH_TESTS.md`.

Note this is why brief §3.3 lists a `syscalls.cpp` in the app's CMake directory
— see the conflict in 0.6, item 3.

### 0.5 Builds and tests run

**SDK host tests — pass.**
```
$ cmake -S Tests/Host -B build-host -DCMAKE_BUILD_TYPE=Debug && cmake --build build-host
$ ctest --test-dir build-host --output-on-failure
100% tests passed, 0 tests failed out of 1
$ ./build-host/una-sdk-host-tests
[==========] 408 tests from 33 test suites ran. (36 ms total)
[  PASSED  ] 408 tests.
```
GoogleTest is fetched by `FetchContent` from GitHub at pinned commit
`52eb8108` (`Tests/Host/cmake/FetchGoogleTest.cmake`) — the host test build
needs network on first configure. No ARM toolchain required.

**RunLVGL for the watch — builds, with the 0.4 caveat.**
```
$ cmake -G "Unix Makefiles" -S Examples/Apps/RunLVGL/Software/Apps/RunLVGL-CMake \
    -B build-arm3 -DCMAKE_EXE_LINKER_FLAGS=<stubs.o>
$ cmake --build build-arm3 -j4
-> Examples/Apps/RunLVGL/Output/RunLVGL_1.5.0-rc4-3-b0f8955.uapp   (419 740 bytes)
```
Configure log confirms: `APP_ID: A1B8E3F04C7D925E`, `APP_NAME: RunLVGL`,
`DEV_ID: UNA`, `UNA_APP_GUI_RAM_LENGTH: 900K`, `UNA_APP_GUI_STACK_SIZE: 24*1024`,
`UNA_APP_SERVICE_RAM_LENGTH: 500K`, autostart OFF, icons ON.

**RunLVGL simulator — builds and runs.**
```
$ cmake -S .../LVGL-GUI/simulator -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
$ cmake --build build -j4        # [589/589] Linking CXX executable bin/RunLVGLSimulator
$ SDL_VIDEODRIVER=dummy timeout 15 ./RunLVGLSimulator
  LvglPort::init  : LVGL 9.5.0 ready, 240x240, stripe 30 rows
  ScreenManager   : LVGL pool: 18216/35936 B used, peak 50%, frag 1%
  Service::run    : GUI is now running
```
`"GUI is now running"` is the same pass condition the SDK's own
`linux-simulator.yml` uses. Measured LVGL pool peak on the main screen: **50 %
of 35 936 B**. Screenshot of the unmodified app: `docs/screens/phase0-runlvgl-unmodified.png`.

### 0.6 Conflicts between the brief and the SDK

Per brief §0 and CLAUDE.md, the SDK wins. Jon should know about these.

1. **Activity file path.** Brief §10.1 says `activity_YYYYMMDDTHHMMSS.fit` under
   `YYYY/MM/`. The code writes a **single** `YYYYMM/` directory
   (`RunLVGL/.../ActivityWriter.cpp:445-474`), corroborated by
   `Docs/BLE-File-Transfer-Service.md:21`
   (`/Apps/Workout/Activity/202607/activity_...fit`).
   `Docs/FitFiles-Structure.md:686` repeats the brief's wrong form — **the doc is
   wrong, the code is right.** We follow the code.
2. **Linker script.** Brief §11.1 requires renaming it to `${APP_NAME}Service.ld`.
   There is one shared script, `Libs/Source/AppSystem/linker/Main/Sections.ld`,
   applied by `cmake/una-app.cmake` for both processes. **Nothing to rename**;
   drop this checklist item.
3. **`syscalls.cpp`.** Brief §3.3 puts one in `HybridXRace-CMake/`. No
   `syscalls.*` exists anywhere in the SDK and `RunLVGL-CMake/` contains only
   `CMakeLists.txt`. With ST's toolchain it is unnecessary. If we ever have to
   ship with a non-ST toolchain, 0.4's stubs are the shape it would take.
4. **Version tagging.** CLAUDE.md says tag `vX.Y.Z` "so the SDK's version script
   picks them up". `cmake/una-app.cmake:149` hardcodes the tag prefix `apps-`:
   `una-version.sh ${WORKING_DIR} apps-`, which greps `git describe --match
   "apps-v*"`. A `v0.1.0` tag on our repo **will be ignored** and the build falls
   back to `1.0.0`/`1.0.0-dirty`. Two options, both fine:
   - tag `apps-v0.1.0` in our repo, or
   - pass `-DBUILD_VERSION=0.1.0`, which `una-app.cmake:141` honours first.
   **Decision needed from Jon in Phase 5**; defaulting to `-DBUILD_VERSION=`.
5. **Adding FIT enums.** Brief §10.1 says to add missing values to
   `FitProfile.hpp`. That is an SDK file and off-limits. Not a problem:
   `FitWriter::defineMessage` takes raw `{fieldNum, baseType, count}`, so
   `lap_trigger` (field 24, **absent repo-wide**), `wkt_step_name` (field 0,
   absent) and any sport byte go in **our own** profile header.
6. **Companion CBOR activity report.** Brief §10.4 asks us to find where
   Running/RunLVGL produce it. **They don't.** There is no CBOR writer in `Libs`
   at all (`find Libs -iname "*Cbor*"` → nothing; `tinycbor` is vendored but
   unused by any SDK header or app). The app emits only the `.fit`, a
   differently-shaped `.json` sidecar, and a payload-free
   `CommandAppNewActivity`. The report is assembled off-watch from the fetched
   FIT file. **Nothing for us to mirror** — set the manifest `supports*` flags
   honestly and write good laps. Anything more must be checked against the phone
   app, not this SDK. **UNVERIFIED** against the phone app itself.
7. **`CMakePresets.json`** does not exist in the SDK (only LVGL's own, vendored).
   All builds use explicit `-S/-B/-G`.
8. **`Docs/SensorsLayer.md` is stale** — it has no `HEART_RATE_EX` entry, though
   that is the sensor the activity apps use. `Libs/Header/SDK/SensorLayer/SensorTypes.hpp`
   is the truth. Likewise `Docs/Examples/Running-Architecture.md:102` says
   `HEART_RATE` where the code uses `HEART_RATE_EX`.
9. **Two source lists.** Brief §14.11 warns the simulator and ARM source lists
   diverge. **Not true for RunLVGL** — both include the same globbed
   `lvgl-gui.cmake` and `libs.cmake` with `CONFIGURE_DEPENDS`. That hazard is
   TouchGFX-only (`simulator/msvs` vs `simulator/gcc`, policed by
   `msvs-source-sync.yml`). Adding a `.cpp` under `gui/src/` is picked up by both.
   We still build both after every change, per CLAUDE.md.

### 0.7 What the GUI closing does to a running activity (brief §9.4)

**This is the most important finding of Phase 0.**

The kernel does not stop a service when its GUI closes; it sends
`COMMAND_APP_NOTIF_GUI_STOP` (`Docs/service-lifecycle.md` §1.1, §5.3).
RunLVGL handles it like this:

```cpp
// RunLVGL/Software/Libs/Sources/Service.cpp:481-487
void Service::onStopGUI()
{
    mGuiStarted = false;
    requestAccessoryRelease();
    mSensorWristMotion.disconnect();
}
```

It does **not** touch the track. The exit lives in the run loop:

```cpp
// Service.cpp:267-273
} else {
    if (guiInitTimeout.expired()) {      // SDK::Timer, 5 s from service start
        LOG_INFO("No activities, exiting service\n");
        return;                          // Exit app
    }
}
```

`SDK::Timer::expired()` is `const` and does not reset
(`Libs/Header/SDK/Timer/Timer.hpp:127-134`), so once 5 s have passed it is
permanently true. **The exit test has no `mTrackState` term.** Therefore:

| Situation | What actually happens |
|---|---|
| GUI closed while idle | Service exits within ~500 ms. Correct. |
| GUI closed **mid-activity** | Service **also exits within ~500 ms**. `stopTrack()` never runs, the FIT is never finalised, no summary is written, no `CommandAppNewActivity` is sent. The part-written `.fit` keeps its `RecordingMarker` and is repaired only on the **next** app launch by `recoverInterrupted()`. |
| `COMMAND_APP_STOP` (kernel teardown) | Handled properly — `stopTrack(false)` runs first (`Service.cpp:144-152`). |

The shipped app is safe only because its GUI has no route to `exitApp()` during
an activity: `Model::exitApp()` is reachable from `MainScreen`,
`TrackSummaryScreen` and `TrackResultScreen` only, and `TrackScreen` /
`TrackActionScreen` have no idle timeout at all. `Docs/service-lifecycle.md` §5.4
says so explicitly and instructs copiers to add the outstanding-work term
themselves.

**Consequences for HybridX Race (Phase 3):**
- Our exit test must be `!mGuiStarted && guiInitTimeout.expired() && raceNotInProgress()`.
- Brief §8.4's "race, action menu, finished and summary screens never exit on
  idle" must be honoured strictly, because it is load-bearing, not cosmetic.
- Stopwatch (`Examples/Apps/Stopwatch/.../Service.cpp:44-56`) is the example that
  gets this right — it gates its exit on `!mStopwatch.isRunning()`. Copy that
  shape, not Running's.
- **Open question for Jon:** if the GUI is closed mid-race, should the race be
  saved automatically, or should the service stay resident (Stopwatch's choice)
  and let the athlete come back? Defaulting to *stay resident and keep timing*,
  as that matches an athlete accidentally leaving the app mid-race.

### 0.8 Two Phase-3/4 risks retired early

**Batched laps decode correctly — the undo design is safe.**
Brief §10.1 requires laps to be buffered in RAM and written together at `SAVE`,
because a split can be undone. The SDK's `ActivityWriter::addLap()` instead
streams each lap immediately, so that ordering is not exercised anywhere in the
SDK. Proven by experiment: a file with all 16 laps written after every record
and before the session, using the SDK's own `FitWriter`, decoded with the
independent `fitdecode` library:

```
decoded cleanly: 4144 records, 16 laps
last record #4152 < first lap #4153 < session #4169: True
all 16 laps match expected duration/index/dev-fields: True
session num_laps=16 sport=training sub_sport=generic race_format=0 completed=1
```

The three Lap developer fields from brief §10.1 (`segment_type`, `round`,
`station_id`) and the three Session ones (`race_format`, `roxzone_mode`,
`completed`) round-tripped by name, resolved from the `FieldDescription`
messages. Source and decoder kept at `docs/experiments/`; they become `FitOutputTest` in Phase 3.

> Still **UNVERIFIED**: that Strava and Garmin Connect accept it. Only an upload
> proves that — recorded as **W11** in `ON_WATCH_TESTS.md`. `fitdecode` agreeing
> is necessary, not sufficient.

**Automated screenshots are possible.** The simulator has no capture feature and
`LV_USE_SNAPSHOT` is `0` in the SDK's `lv_conf.h` (which we may not edit). The
working route is external: run under `Xvfb`, drive with `xdotool key 1|2|3|4`,
capture with ImageMagick `import -window <id>`. Verified — produced a clean
480x480 PNG of the main wheel (`docs/screens/phase0-runlvgl-unmodified.png`) and
confirmed key injection changes the frame. Phase 4's `docs/screens/` deliverable
needs no human at a keyboard.

### 0.9 Platform facts for later phases

**Buttons** (`Libs/Header/SDK/GUI/Button.hpp`): click codes `L1='1' L2='2'
R1='3' R2='4'`, press `q/w/e/r`, release `a/s/d/f`, chord `L1R2='z'`.
Positions: **L1 top-left, L2 bottom-left, R1 top-right, R2 bottom-right**.
RunLVGL's mapping matches the brief's §8.1 race-screen requirement already:
menus are L1/L2 wheel + R1 select + R2 back; the track screen is L1/L2 face
cycling, R1 → action menu, R2 → lap. Long-press is not a code — screens derive
it from press/release, and RunLVGL's `TrackHoldConfirmScreen` (1500 ms,
`lv_anim`-driven) is the hold-to-confirm pattern to copy. There is **no shared
LVGL hold widget** in the SDK (`SDK/GUI/CountdownTimer.hpp` is TouchGFX-only).

**Simulator keys**: `1..4` = L1, L2, R1, R2; `q/w/e/r` press; `a/s/d/f` release;
`5` raises wrist motion; Esc quits. A key held < 500 ms counts as a click.
Simulated HR ramps 120–180 bpm in "Running" mode and serves `HEART_RATE_EX`.

**Message size cap — 256 bytes.** `Docs/writing-a-clockface.md:310-318`: the
kernel's message pools top out at 256 B; a larger allocation returns `nullptr`
and **the send is dropped silently**. The simulator uses `new[]` and will never
show this. Only Stopwatch guards it with a `static_assert`; Running/RunLVGL do
not. **Every message struct we define gets a `static_assert`.** Our `SUMMARY`
message carrying up to 31 segments cannot fit — RunLVGL passes a raw pointer
into service memory instead (`CustomMessage::Summary`), which works only because
the processes share an address space and is a real lifetime hazard. Phase 3 must
choose deliberately between paging the summary over several messages and
copying RunLVGL's pointer trick.

**GUI custom-message queue is 10 deep** (`SDK/Port/GuiCommandProcessor.hpp:108`)
and drains once per GUI tick. A suspended GUI drains nothing, and once full the
**newest** message is rejected. At 1 Hz that is ~10 s of suspension before loss.

**Heart rate.** `HEART_RATE_EX` (0x43) at 1000 ms. Parser
`SDK::SensorDataParser::HeartRateEx` gives arbitrated `BPM` + `TRUST_LEVEL`,
`SOURCE` (`UNKNOWN/OPTICAL/EXTERNAL`), and raw optical/external pairs. The
brief's FIT gate is confirmed verbatim (`Service.cpp:682-688`):
`bpm > 20 && trust >= 1 && trust <= 3`. Live display is ungated, as the brief says.
An external strap is opt-in via `Accessory::RequestPrepare` with
`kinds = Accessory::Kind::HRM`, sent at GUI start and released on GUI stop.

**Haptics and backlight** are messages, not helpers — `SDK::Kernel` exposes only
`sys, log, mem, comm, fs`. `RequestBuzzerPlay` allows **max 10 notes, so max 5
beeps**; `RequestVibroPlay` allows **max 8 notes, so max 4 effects**, from a named
`Effect` enum (`STRONG_CLICK_100=1`, `ALERT_750MS_100=15`, …). Brief §8.3's
patterns (1 / 2 / 1 short / 1 long pulse) all fit.

**Backlight during a race**: `WristTiltDetector` is **app-local**, copied into
the app (`RunLVGL/Software/Libs/{Header,Sources}/WristTiltDetector.*`), fed from
`FUSION_RAW` at 100 Hz in 10-sample batches. It holds a `float[1024]` window
(~4 KB) — real memory against the service's budget. Outside a race,
`WRIST_MOTION` is used, and its `isDataValid()` *is* the event.

**AppConfig**: RunLVGL does **not** use `SDK::AppConfig` at all — it keeps
settings in its own `settings.json` via `SettingsSerializer`. The only worked
example is `Docs/Tutorials/Waypoint`. Take the pattern from there: constexpr
`Field` table in one file, one entry per line (that is the form
`validate_app_config.py --check-bounds` parses), read in `run()` and not the
constructor, values file capped at **8192 bytes**, at most **32 fields**.

**Packaging**: icons are validated by `app_merging.py` — `icon_60x60.png` and
`icon_30x30.png`, exactly those sizes, square, converted to ABGR2222; required
for every non-Glance app. `APP_ID` is the first 16 hex chars of `md5(app_name)`,
uppercased (`Docs/sdk-setup.md:314-319`), and **must be mirrored into the
simulator's `target_compile_definitions`** or the simulator writes a different
`developer_data_id`. Manifest validation covers only `manifest_version`,
`configFile`, `configFields` and `minKernelVersion`; the `supports*` flags are
not machine-checked by anything in this repo.

### 0.10 Template strategy — refined

The brief assumes a single template. The evidence says split it:

| Part | Source | Why |
|---|---|---|
| GUI, scaffold, CMake, simulator | `Examples/Apps/RunLVGL` | the only LVGL app |
| `ActivityWriter` | `Examples/Apps/Workout` | already non-GPS, distance-free, manual-lap, `Sport::Generic`; its `RecordData` has only HR + battery flags and its `LapData` has no distance or speed |
| Workout-step tagging (F16) | RunLVGL's `addWorkout()` + `wktStepIndex` | `Lap::WktStepIndex` (field 71) exists and is written today |

Starting the writer from Running's copy would mean deleting GPS, speed, altitude
and cadence that Workout never had. Note `RunLVGL/Software/Libs` and
`Running/Software/Libs` are **byte-identical** (`diff -rq` is silent) — RunLVGL
is Running with an LVGL GUI.

**F16 (segment names in Garmin Connect) is more feasible than the brief assumed.**
The workout/workout_step plumbing and the `lap.wkt_step_index` linkage are all
present and exercised by RunLVGL's intervals mode. Only the per-step *name*
(`wkt_step_name`, field 0) is missing from the SDK's profile constants, and we
can define it ourselves. Still P1 — report before implementing.

### 0.11 §11 compliance checklist (started)

- [x] App builds outside the SDK — confirmed: `cmake/una-app.cmake` resolves
      everything through `$ENV{UNA_SDK}` and explicit path variables.
- [x] `minKernelVersion` floor determined: **1.4.0** (ABI 3).
- [x] Icons requirement understood (60x60 + 30x30, square, mandatory).
- [x] `APP_ID` generation method understood; must **not** reuse RunLVGL's
      `A1B8E3F04C7D925E`.
- [ ] `CMakeLists.txt` variables set for HybridXRace — Phase 2.
- [ ] ~~Linker script renamed~~ — **not applicable**, see 0.6 item 2.
- [ ] Memory: start from RunLVGL's 900K GUI RAM / 24 KB GUI stack; measured pool
      peak so far is 50 % of 35 936 B on the main screen. Re-measure in Phase 4.
- [ ] Versioning — see 0.6 item 4, decision pending.
- [ ] `.uapp` in `Output/` for our app — Phase 2.
- [ ] Manifest validates with `--check-bounds` — Phase 5.

### 0.12 Decisions outstanding

| ID | Decision | Needed by | Current default |
|---|---|---|---|
| D1 | App name; whether "HYROX" may appear | Phase 5 | "HybridX Race", placeholder icon |
| ~~D2~~ | ~~FIT sport / sub_sport~~ | ~~Phase 3~~ | **Decided 23 September 2026: `Running(1)` / `Generic(0)`** — see 5.12 |
| D5 | Publishing route | Phase 6 | build for either |
| D6 | Pacing share table | F14 | feature stays hidden |
| — | Version tagging mechanism (0.6 item 4) | Phase 5 | `-DBUILD_VERSION=` |
| — | Mid-race GUI close behaviour (0.7) | Phase 3 | stay resident, keep timing |

---

## Phase 1 — RaceModel core (21 September 2026)

### 1.1 Status

Complete. `RaceData.hpp`, `RaceModel.hpp/.cpp` and four host suites, **66 tests,
all passing**, no warnings under `-Wall -Wextra -Wpedantic`. Gate 1 met.

### 1.2 Decisions taken

**Time is passed in, not read from an injected clock.** CLAUDE.md says "pure C++
with an injected clock"; the brief's own §7.3 event table already passes `t` into
every event, and §7.4 requires the split to be stamped at the button press in the
GUI. A model that called its own clock would re-time every split by up to one GUI
tick (100 ms), which is exactly the bias §7.4 exists to prevent. The SDK's own
pure-logic example takes the same shape
(`Examples/Apps/Stopwatch/Software/Libs/Header/Stopwatch.hpp`: `elapsed(state, nowMs)`).
Jon chose timestamps-as-parameters. **CLAUDE.md's wording should be amended.**

**Mid-race GUI close: stay resident, then autosave after 5 minutes.** Jon's
instinct was straight autosave — an athlete leaving the app has probably
finished. The counter-argument that changed it: R2 is Back/exit almost everywhere
else in the UI and the launcher button is top right, so the person most likely to
leave the app mid-race is the one fumbling for the split button with sweaty hands
at 180 bpm. Straight autosave would end their race irrecoverably on one bad press,
against design principle 2 ("mistakes are recoverable") and F5. The costs are
lopsided: autosaving wrongly bins a 40-minute simulation; staying resident wrongly
costs battery and a leaked service, which is recoverable. So: keep timing, and
autosave + exit if the GUI does not return within **5 minutes**. Implemented in
Phase 3; the window is one constant, and T13 is the chance to tune it.

### 1.3 A bug the tests caught

`LapAccumulatorTest.UndoFinishMergesTheFinalSegmentToo` failed on first run with
a doubled heart-rate sum. Cause: `split()` banks the open segment and, on the
**finishing** split, never opens another — so the per-segment accumulators were
left populated. A later `undoFinish()` then merged that segment's heart rate and
paused time in a second time. Fixed by clearing the accumulators in
`closeCurrent()` (where the banking happens) rather than in `openSegment()`.

Worth recording because it is invisible on every path except undo-after-finish,
which is precisely test T7 on the watch.

### 1.4 Measured

| Item | Value |
|---|---|
| `sizeof(SegmentDesc)` | 3 bytes |
| `sizeof(SegmentResult)` | 24 bytes |
| `sizeof(RaceModel)` | **888 bytes** |
| 31 results | 744 bytes |
| ARM object (`cortex-m33`, `-Os`) | 1511 text, 64 data, 0 bss |

`RaceModel` is comfortable against the service's 500K budget. The 744-byte
segment list confirms 0.9's warning: **the summary cannot travel in one kernel
message** (256-byte pool cap). Phase 3 must page it or use RunLVGL's
pointer-passing trick deliberately.

### 1.5 Deviations from the brief, and why

- **`UNDO_SPLIT` is refused while paused.** Brief §7.3 lists it from `RUNNING`
  only, and the tests assert that. Our action menu does not auto-pause (§8.1 says
  the race clock keeps running when it opens), so this should never bite in
  practice. If T5 on the watch says otherwise it is a one-line change.
- **Heart-rate samples taken while paused are dropped.** Not specified. A pause is
  rest, and counting it would drag the segment average down and misreport the
  effort. `LapAccumulatorTest.SamplesTakenWhilePausedAreDropped` pins it.
- **The lockout also guards the race start.** `START` seeds the lockout reference,
  so a double press on "Start race" cannot split straight out of segment 0. Falls
  out of §7.3's "t − lastSplitTime" with `lastSplitTime` initialised to the start.

### 1.6 Test coverage

| Suite | Tests | Covers |
|---|---|---|
| `RaceTemplateTest` | 14 | §7.2 counts, ordering, half-race round numbering, every station label, buffer bounds |
| `RaceStateTest` | 22 | every §7.3 transition, and the illegal ones |
| `RaceTimingTest` | 18 | §7.5 invariants 1-7 by name, clock wrap, lockout boundary, pause accounting |
| `LapAccumulatorTest` | 12 | heart-rate merge on undo, average-of-averages trap, paused-sample handling |

All seven §7.5 invariants have a test named after them, so a failure says which
promise to the athlete broke. The 90-minute race in
`RaceTimingTest.AFullRaceOfNinetyMinutesAddsUp` runs in under a millisecond.

### 1.7 Not done in this phase

No `Software/Apps/` tree, no messages, sensors, FIT or GUI — those are Phases 2
and 3. CLAUDE.md's "build the watch target and the simulator after every change"
had nothing to build here, but `RaceModel.cpp` was cross-compiled for
`cortex-m33` with the app's real flags to prove it is embedded-clean.

---

## Phase 2 — App scaffold (21 September 2026)

### 2.1 Status

Complete. RunLVGL copied into `hybridx-race/`, renamed throughout, and building
for **both** targets with its behaviour unchanged under the new name. Gate 2's
watch install is deferred (`ON_WATCH_TESTS.md` W3); everything testable here is
verified.

### 2.2 What was copied and renamed

| From (SDK, read-only) | To | Note |
|---|---|---|
| `RunLVGL/Software/Apps/LVGL-GUI/` | `Software/Apps/LVGL-GUI/` | GUI, assets, simulator |
| `RunLVGL/Software/Libs/` | `Software/Libs/` | service, merged alongside our `RaceModel` |
| `RunLVGL/Software/Apps/RunLVGL-CMake/` | `Software/Apps/HybridXRace-CMake/` | renamed directory |
| `RunLVGL/Resources/*.png` | `Resources/` | **replaced**, see 2.4 |

Identity now:

```
APP_NAME       HybridXRace          (no spaces, drives file names)
APP_USER_NAME  HybridX Race         (what the watch shows)
APP_FILE_NAME  HybridXRace
APP_TYPE       Activity
DEV_ID         HybridX
APP_ID         8C345EF26E3350E7     (development only, see 2.3)
```

The simulator's own `target_compile_definitions` carry the same `APP_ID`, because
`Docs/writing-a-clockface.md:80` warns that a mismatch makes the simulator write
a different `developer_data_id` into the FIT file than the watch does.

`RUNLVGL_FRAME_STATS` became `HYBRIDXRACE_FRAME_STATS`; the underlying SDK define
`UNA_LVGL_FRAME_STATS` is the SDK's and was left alone.

### 2.3 APP_ID

`8C345EF26E3350E7` = first 16 hex of `md5("HybridXRace")`, uppercased, per
`Docs/sdk-setup.md:314-319`. **Deliberately not RunLVGL's `A1B8E3F04C7D925E`.**
Before publishing, this must be replaced by the ID the portal issues (brief
§11.1) in *both* `HybridXRace-CMake/CMakeLists.txt` and the simulator's
`CMakeLists.txt`.

### 2.4 Icons replaced rather than reused

RunLVGL's icons were copied in and then **replaced with plain "HX" placeholders**
(teal, white text). Shipping UNA's artwork under our own app would sit badly with
brief §11.6 ("do not use the UNA name or logo"), and the MIT licence grants no
trademark rights. They are placeholders until Jon supplies HybridX artwork (D1).

`app_merging.py` validates them: exactly 60x60 and 30x30, square, converted to
ABGR2222. Build log confirms both were accepted.

### 2.5 Version number confirms the Phase 0 finding

The configure log reads `Detected BUILD_VERSION: 0.0.0-dev`. That is 0.6 item 4
observed live: `cmake/una-app.cmake:149` greps `apps-v*` tags, our repo has none,
so the fallback applies. Harmless now; **must be settled before Phase 5** by
tagging `apps-v0.1.0` or passing `-DBUILD_VERSION=0.1.0`.

### 2.6 Verification

```
$ cmake -G "Unix Makefiles" -S hybridx-race/Software/Apps/HybridXRace-CMake \
    -B hybridx-race/build -DCMAKE_EXE_LINKER_FLAGS=<stubs.o>
$ cmake --build hybridx-race/build -j4
-> hybridx-race/Output/HybridXRace_0.0.0-dev.uapp   (419 732 bytes)

$ cmake -S .../LVGL-GUI/simulator -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
$ cmake --build build -j4
-> build/bin/HybridXRaceSimulator

$ SDL_VIDEODRIVER=dummy timeout 12 ./HybridXRaceSimulator
  LvglPort::init  : LVGL 9.5.0 ready, 240x240, stripe 30 rows
  ScreenManager   : LVGL pool: 18216/35936 B used, peak 50%, frag 1%
  Service::run    : GUI is now running

$ ./hybridx-race/build-tests/hybridx-race-host-tests
  [  PASSED  ] 66 tests.
```

Screenshot under the new name: `docs/screens/phase2-renamed-scaffold.png` —
the wheel reads **HYBRIDX RACE** with RunLVGL's Start/Intervals menu intact,
which is exactly what "behaviour unchanged under the new name" should look like.

The `.uapp` is 8 bytes smaller than RunLVGL's, consistent with a pure rename.

### 2.7 Worth knowing for Phase 3

`Software/Libs/libs.cmake` globs `Sources/*.cpp`, so **`RaceModel.cpp` is already
compiled into both the service and the simulator** — it just is not called yet.
That is free proof it builds in the real app context, ARM included, and it is why
Phase 3 can wire it in without touching the build.

Still inherited from RunLVGL and due for removal in Phase 3: GPS connections and
map building, distance and time auto-laps, the interval phase machine and its
settings and screens.

---

## Phase 3 — Service and GUI integration (21 September 2026)

### 3.1 Status

Complete. Both builds green, 66 host tests pass, and **Gate 3 is met**: a full
16-segment race driven in the simulator produces a FIT file that decodes with
the right laps. Gate 4's on-watch tests stay deferred (`ON_WATCH_TESTS.md`).

Done in two commits because the tree was red in between: the service changes
the message set out from under the GUI, so there is no ordering that keeps both
ends compiling. Part 1 was the service, part 2 the GUI.

### 3.2 Gate 3 evidence

A scripted race — Xvfb, `xdotool` for the buttons, 16 splits four seconds apart
— then decoded with `fitdecode`:

```
decoded cleanly: 67 records, 16 laps
ordering last-record 80 < first-lap 81 < session 97: True

  lap  0  RUN      r1                         6.0s
  lap  1  STATION  r1 SkiErg                  4.0s
  ...
  lap 15  STATION  r8 Wall Balls              4.0s

session: laps=16 sport=training sub=generic format=0 roxzone=0 completed=1 timer=67.0s
message_index sequential 0..15: True
```

Station names in that listing are resolved *from the developer fields*, not from
anything the decoder knows about HYROX — which is the point of §10.1. The
batched-lap ordering Phase 0 proved in isolation now holds in the real app.

Service log for the same run:
```
Service::startRace  : Race started: format 0, roxzone 0, 16 segments
Service::finishRace : Race finished: 16 of 16 segments, completed 1
Service::saveRace   : Race saved: 16 laps
```

### 3.3 The middle dot does not exist in the fonts

Brief §7.2 writes labels as `RUN 3/8 · 1 km`. The first simulator run rendered
that as **`RUN 1/8 □ 1 km`** — an empty box. The shipped Poppins subsets are
ASCII only (`ASCII = "0x20-0x7E"` in `LVGL-GUI/assets/gen_assets.py`), so
U+00B7 has no glyph.

Changed to a hyphen: `RUN 3/8 - 1 km`, `SLED PULL - 50 m`. One constant,
`Race::kLabelSep` in `RaceData.hpp`.

To restore the real middle dot, add `0xB7` to the font ranges and regenerate
with `lv_font_conv` (`Utilities/Scripts/lvgl_assets/lvgl_assets.py`, pinned
1.5.3). That is a Node toolchain and an asset job rather than a code change, so
it belongs with Phase 4's visual pass. **Open for Jon:** hyphen, restored middle
dot, or something else.

This is a good argument for the screenshot pipeline existing: the bug is
invisible in the source and obvious on screen.

### 3.4 Design decisions worth knowing

**The action menu does not pause the race.** RunLVGL's equivalent calls
`trackPause()` in `onShow()`. Brief §8.1 is explicit that the race clock keeps
running when the menu opens, and an athlete who opens it to look would otherwise
have their race silently paused. Ours leaves the clock alone; Pause is an item
in the menu.

**The summary is paged, and the GUI owns its copy.** `SUMMARY_META` resets the
accumulator, then `SUMMARY_PAGE` messages fill it eight segments at a time.
RunLVGL instead sends a raw pointer into service memory
(`CustomMessage::Summary`), which works only because the two processes share an
address space and leaves the GUI holding a lifetime it does not control.

**`label()` is inline in the header.** Both processes need it — the service for
the summary, the GUI for every screen — and the GUI ELF does not link
`RaceModel.cpp`. Found by a linker error, fixed in the right place rather than
by adding the source to the GUI build.

**Screens deleted rather than adapted**: the interval and alert screens
(`MenuIntervals*`, `MenuAlerts*`, `TrackIntervals*`, `IntervalsPicker`,
`AlertSaved`) and the `IntervalsTimer`, `TwoTonePicker` and `Map` widgets. Pace
and distance formatters went with them: a race has neither.

### 3.5 Measured

| Item | Value |
|---|---|
| `.uapp` | 365 428 bytes (RunLVGL was 419 740) |
| `RaceDataUpd` | 72 bytes |
| `SummaryPage` | 140 bytes, 8 segments |
| `SummaryMeta` | 64 bytes |
| `SettingsUpd` | 52 bytes |
| LVGL pool peak | 50 % of 35 936 B -- **superseded, see 5.6**: this was the first screen switch only, and the real peak across the flow is 91 % |

Every message is `static_assert`ed against the 256-byte pool at compile time.
The app is ~54 KB smaller than RunLVGL, consistent with dropping GPS, the map,
the interval machine and nine screens.

### 3.6 Layout issues for Phase 4's visual pass

Phase 3's screens are functional, not designed — that is Phase 4's job. Two
things the screenshots already show:

- On the race screen the "Next:" line runs underneath the heart-rate zone arc.
  Both are at the bottom of a 240 px circle and neither was placed with the
  other in mind.
- The status face is a bare clock and battery percentage; brief §8.2 says to
  reuse RunLVGL's status face, which has proper widgets.

### 3.7 Still to do

- **F14 target pacing and F15 splits face** — P1, and F14 needs Jon's data (D6).
- **F16 workout-step names** — P1; the plumbing exists (`NOTES.md` 0.10).
- **D2 sport/sub-sport**: currently `training`/`generic`. `ActivityWriter::TrackData`
  carries `sport` and `subSport` as parameters, so candidate files are a
  one-line change when Jon wants to compare them on Strava and Garmin.
- The **splits face** on the race screen: only Main and Status exist today.

---

## Phase 4 — Visual pass (21 September 2026)

### 4.1 Status

Done in this container: every screen in brief §8.2 laid out against the round
display's real geometry, the §8.4 idle and suspend rules audited screen by
screen, and the whole flow captured to `docs/screens/`. Not done: looking at any
of it on a watch. Gate 4 needs Jon's eye on the screenshots and T1-T8 on
hardware (`ON_WATCH_TESTS.md`).

Host tests 71 pass at the time of writing (73 after 5.9). Watch target and simulator both build clean.

### 4.2 The middle dot, properly fixed

`NOTES.md` 3.3 recorded that U+00B7 was missing from the fonts and that the
labels had fallen back to a hyphen. Node v22 turned out to be available in the
container, so the subsets were regenerated rather than worked around:

- `assets/gen_assets.py`: `ASCII = "0x20-0x7E,0xB7"`, and `SDK_ROOT` now comes
  from `$UNA_SDK` instead of counting seven directories up from the script —
  out of tree that arithmetic landed on `/home` and the script found nothing.
- Eleven text faces regenerated with `npx lv_font_conv@1.5.3`, 2 bpp,
  uncompressed, matching the flags the SDK's own script uses.
- `Race::kLabelSep` is back to `"\xC2\xB7"` and `RaceTemplateTest` asserts the
  labels byte for byte.

### 4.3 What a round 240 px display actually allows

This is the finding that drove most of Phase 4, and it is worth writing down
because nothing in the SDK docs says it: **the usable width is a chord, not
240 px**, and it collapses fast towards the bottom of the screen.

The display is a 240 px circle centred at (120, 120), so at height y the usable
width is `2 * sqrt(120^2 - (y - 120)^2)`:

| y | Usable width |
|---|---|
| 44 | 186 px |
| 60 | 208 px |
| 100 | 237 px |
| 120 | 240 px |
| 160 | 226 px |
| 180 | 208 px |
| 200 | 179 px |
| 210 | 159 px |

What binds a line of text is its **lowest** pixel, not its baseline: a label at
y = 180 in an 18 px face is still inside 208 px, but its descenders are down at
y = 198 where only 182 px are left. Laying out to the top of a line is how the
heart rate ended up under the arc.

An earlier version of this table was wrong at both ends -- 226 px at y = 60 and
222 px at y = 160, which are each other's values, near enough. The layout was
done against the screenshots rather than the table, so nothing shipped wrong,
but the table is design guidance for later work and is now computed rather than
eyeballed.

Three things were overrunning it, all invisible until the screenshots were
looked at properly:

- **The race screen's segment label.** `BURPEE BROAD JUMPS \xC2\xB7 80 m` is 25
  characters and ran off both sides. The race face now splits it: the name takes
  the accent colour on one line, the work joins the segment counter on the grey
  line below (`1000 m \xC2\xB7 2 of 16`). `RaceModel::label()` is unchanged and is
  still what the FIT lap names and the summary use.
- **The summary's HR row.** `HR 146 avg 156 max` was clipped at both ends. It is
  now two rows, `Avg HR` and `Max HR`, in a proper two-column table inset to
  x = 34..206 — the width the display still has at the bottom row.
- **The heart rate above the zone arc.** The arc is the bottom of a circle of
  radius 113 centred below the screen; its topmost pixel is at y = 185. The
  heart rate was at y = 166 and the arc painted over the bottom of the digits.

### 4.4 Station short names — Jon to sign off at Gate 4

The summary's split rows sit where the display is at its narrowest, and a full
station name plus a time does not fit in 178 px at any face we have. So
`Station` gained a third field, `brief`, used **only** on that row:

| Station | Split row |
|---|---|
| SKIERG | SKIERG |
| SLED PUSH | SLED PUSH |
| SLED PULL | SLED PULL |
| BURPEE BROAD JUMPS | **BURPEES** |
| ROW | ROW |
| FARMERS CARRY | **CARRY** |
| SANDBAG LUNGES | **LUNGES** |
| WALL BALLS | WALL BALLS |

These three abbreviations are display text I chose, not HYROX terminology.
**Jon: say if you would write any of them differently** — it is a one-line edit
in `RaceData.hpp`, and `RaceTemplateTest` pins the width budget
(`kMaxBriefLen`) so a longer replacement fails the build rather than clipping on
the watch.

### 4.5 Three bugs the visual pass found

- **The split list showed no names at all.** `TrackSummaryScreen::redraw()`
  formatted the label into a local and then printed only the index and the time.
  The compiler had nothing to warn about: the variable was written, just never
  read.
- **The start screen still asked "Start before signal acquired?"** — RunLVGL's
  GPS gate, inherited whole, on an app that brief §8.2 says has no GPS gating at
  all. It is now an "On your marks" screen that states the format, the segment
  count and whether Roxzone is split, which is what an athlete on the line wants
  to confirm.
- **The split toast showed the work as well as the name.** Brief §8.2 item 4
  writes the toast as `SkiErg 4:12`; it now uses the name alone.

### 4.6 Idle and suspend, screen by screen (§8.4)

| Screen | On idle | Correct because |
|---|---|---|
| Main menu | Exits the app | §8.4, every menu screen |
| Settings | Saves, then exits the app | §8.4; losing an edit to a timeout would be worse |
| On your marks | **Nothing** | Where an athlete waits for the gun |
| Race | Nothing (no timer running) | §8.4 |
| Split toast | Nothing; its own 2 s timer dismisses it | §8.2 item 4 |
| Action menu | Back to the race after 10 s | §8.2 item 5, never out of the app |
| Hold-confirm | Cancels the hold | §8.4 third bullet |
| Finished | Auto-saves after 60 s | Decision D4 |
| Saved / Discarded | Nothing | Nothing left to lose |
| Summary | Nothing | A race may still be unsaved behind it |

The base `onIdleTimeout()` is empty, so a screen that wants no timeout simply
does not override it — the safe default is the do-nothing one, which is the
opposite of RunLVGL, where ten menu screens got the do-nothing behaviour by
accident.

One change of mind since Phase 3: `MainScreen` used to exempt the `Start race`
row from the idle exit, so that a hesitating athlete was not thrown out. That
re-created exactly the gap §8.4 names, and it exempted the menu's *default*
selection, so in practice the main screen never timed out at all. The exemption
is gone; the "On your marks" screen is the place to wait instead.

Suspend during a hold-to-confirm was already handled —
`TrackHoldConfirmScreen::onSuspend()` calls `cancel()` — and is unchanged.

### 4.7 Screens captured

`docs/screens/phase4-*.png` are straight captures of the simulator driven by a
scripted key sequence (`Xvfb` + `xdotool` + ImageMagick `import`, the rig built
in Phase 0). They are 480x480 because the simulator draws at 2x; the watch is
240x240.

Twenty-one screens, the whole flow end to end: the main menu on each row
(01-04, 07), Settings (05-06), "On your marks" (08), the race face running and
on a station (09, 11), the split toast (10), every action-menu row (12-14, 20),
the finished screen after a complete race and after ending early (15, 21),
Saved (16), and the summary overview and both split pages (17-19).

Two of these are worth looking at side by side. `phase4-15-finished.png` offers
`R1 Save` and `L2 Undo`; `phase4-21-finished-early.png` offers only `R1 Save`,
because brief §7.3 refuses `UNDO_FINISH` on a race that was ended early rather
than finished by a split. Phase 3 advertised the Undo in both.

`phase4-03-main-lastrace.png` shows `Last race` greyed with no R1 hint. Brief
§8.2 item 1 says the row appears "only if a summary exists"; greying it rather
than hiding it keeps the menu the same length every launch and tells a new user
the feature is there. `MainScreen::confirm()` makes it genuinely inert, not just
grey.

The colours are the ones the watch will show: the simulator applies the same
2-bit-per-channel quantisation, which is why the accent colours were picked for
separation rather than subtlety.

The capture rig itself needed hardening. Two runs overlapped on display `:97`,
one killed the other's `Xvfb`, and the surviving simulator kept writing 166-byte
blank frames while its log filled with `Queue is full` — a lost display looks
exactly like a working one if you only check the exit code. `shot.sh` now aborts
when no window is found and warns on any capture under 500 bytes.

### 4.8 Still to do

- **Gate 4 is not met in this container.** Jon reviews the screenshots and runs
  T1-T8; the font glyph, the short names and the arm's-length readability of the
  race face all need a real screen.
- **Splits face (P1)** — the race screen still has Main and Status only.
- **F14 target pacing** still waits on Jon's data (D6).

---

## Phase 5 — Packaging and documentation (22 September 2026)

### 5.1 Status

Gate 5 is **met in full in this container**: the manifest validates, the C++
field table and the manifest agree under `--check-bounds`, and the zip matches
the layout `Docs/deploy.md` specifies. None of it needed a watch.

Host tests 71 pass at the time of writing (73 after 5.9); the watch target and the simulator both build clean.

### 5.2 §11 compliance checklist, completed

Replaces the half-ticked list in 0.11. Every line has its evidence.

**§11.1 Project and build**

- [x] **App lives outside the SDK.** `Software/Apps/HybridXRace-CMake/CMakeLists.txt`
      resolves everything through `$ENV{UNA_SDK}` and explicit path variables;
      no SDK file is modified.
- [x] **`CMakeLists.txt` variables.** `APP_NAME=HybridXRace`,
      `APP_ID=8C345EF26E3350E7`, `DEV_ID=HybridX`, `APP_TYPE=Activity`,
      `APP_AUTOSTART` off, and the four path variables.
- [x] ~~**Linker script renamed**~~ — not applicable; see 0.6 item 2. The SDK's
      `una-app.cmake` generates the service linker script from `APP_NAME`, so
      there is no file to rename.
- [x] **`APP_ID` is ours, not RunLVGL's.** RunLVGL is `A1B8E3F04C7D925E`; ours is
      `8C345EF26E3350E7`, generated locally by the SDK's documented md5 method.
      It is a **development** ID and must be replaced by the portal's before
      publishing — said again in `README.md` and printed by the packaging script
      on every run.
- [x] **Memory**, measured on this build (5.3).
- [x] **Versioning** resolved (5.4).
- [x] **Icons** present: `Resources/icon_30x30.png` and `icon_60x60.png`.
      Placeholders, decision D1 open.
- [x] **`.uapp` in `Output/`**: `HybridXRace_0.0.0-dev.uapp`, 367 476 bytes.

**§11.2 Manifest**

- [x] **Written** at `Resources/app-manifest.json`. Deviation: brief §11.2 puts
      it at `Output/app-manifest.json`, but the root `.gitignore` excludes
      `hybridx-race/Output/` wholesale, so nothing there can be reviewed in git.
      The tracked source of truth is in `Resources/`; `pack-store-zip.sh` emits
      the stamped copy into `Output/Release/`.
- [x] **`minKernelVersion` derived, not typed**: `min_kernel_version.py --stamp`
      then `--check` give `1.4.0` from ABI 3.
- [x] **Field table matches** (`--check-bounds`):
      `4 configuration field(s) OK, field table matches (4 entries)`.

**§11.3 Store package**

- [x] **Zip built and structure verified.** `Utilities/pack-store-zip.sh`
      produces `Output/HybridXRace-<version>.zip` containing the `.uapp`,
      `app-manifest.json`, `icon.png` and `previews/` with six screenshots --
      the content list `Docs/deploy.md` gives.
- [ ] **Upload** — Jon's step, needs the portal.

**§11.5 Documentation**

- [x] `docs/NOTES.md` (this file), `docs/ARCHITECTURE.md`, `README.md`,
      `CHANGELOG.md`.

**§11.6 Licensing and naming**

- [x] `LICENSE` (MIT) and `THIRD-PARTY-LICENSES.md`, with one open item for Jon
      (5.5).
- [x] The UNA name and logo appear nowhere in the app name or icon.
- [x] "HYROX" is not in the app name or icon. Its use in the store description
      is D1, still Jon's.

### 5.3 Measured on this build

| Item | GUI | Service |
|---|---|---|
| `.text` | 265 816 B | 87 920 B |
| `.data` + `.got` | 4 008 B | 1 440 B |
| `.bss` | 114 748 B | 6 864 B |
| `.stack` | 24 576 B | 10 240 B |
| **Image total** | **409 148 B** | **106 464 B** |

The app is position-independent and loaded whole into RAM, so the image total is
what the kernel must find. The GUI's linker region is `GUI_RAM_LENGTH = 900K`,
raised from the SDK default of 600K in Phase 2 because an LVGL image is larger
than a TouchGFX one — so the GUI occupies **44 %** of its ceiling, with room for
the splits face and target pacing later. `GUI_STACK_SIZE` is 24 KB, also raised
in Phase 2: LVGL's software renderer recurses deeper than TouchGFX's.

`.uapp`: 367 476 bytes, against RunLVGL's 419 740.

LVGL's own static pool needs its own section: see 5.6.

### 5.4 Version tagging — resolved

0.6 item 4 left this open. The answer: `una-app.cmake` calls
`Utilities/Scripts/build-cube/una-version.sh` with the tag-family prefix
`apps-`, and the script then matches only `apps-v*`. **A plain `v0.1.0` tag is
not seen** and the build stays `0.0.0-dev` — which is exactly what the current
binary is.

CLAUDE.md says to tag `vX.Y.Z`. The SDK is the source of truth where the two
conflict, but here they need not: tag **both** on the same commit. `v0.1.0` is
what CLAUDE.md asks for and what a GitHub release wants; `apps-v0.1.0` is what
stamps the binary. Phase 6 does this.

`pack-store-zip.sh` will not let the two drift: it reads the version out of the
`.uapp` filename rather than trusting the manifest, and `--expect-version 0.1.0`
makes it refuse to package a binary that is not actually 0.1.0 — which is what
catches a forgotten tag.

### 5.5 A licensing gap in the SDK, for Jon

We redistribute `assets/fonts/*.c`: LVGL bitmap fonts generated from the Poppins
TTFs the SDK ships. That is redistribution of a derivative of the font, not just
a build artifact, so the font's licence travels with it.

Poppins is published under the SIL Open Font License 1.1, which permits this and
requires the notice to accompany it. But:

- The SDK ships the Poppins TTFs (under `Docs/Tutorials/Buttons/.../assets/fonts/`)
  **with no licence file beside them**.
- The SDK's own `THIRD-PARTY-LICENSES.md` lists TouchGFX, coreJSON and tinycbor,
  and names **neither LVGL nor Poppins** — though LVGL is a submodule under
  `ThirdParty/lvgl` with its own MIT `LICENCE.txt`, and our GUI links it
  heavily.

So our `THIRD-PARTY-LICENSES.md` rests on Poppins' public licensing rather than
on anything in the checkout. **Jon: before publishing under either route,
confirm the licence covering the TTFs the SDK distributes**, and if OFL, add
`OFL.txt` beside our generated fonts. Worth reporting upstream as an SDK issue
either way.

Confirmed by inspection, not assumed: `HybridXRaceGUI.elf.elf.map` contains **no
TouchGFX symbols**. That matters, because SLA0048 forbids subjecting TouchGFX to
open-source terms, and it means the MIT licence on our own code is unobstructed.

### 5.6 The LVGL pool peaks at 91 % — the top risk going into Gate 6

§11.1 asks for the pool peak to be logged. Doing it properly, rather than
glancing at the first screen as Phase 3 did, turned up the most significant
finding of this phase.

`ScreenManager::switchNow()` logs `lv_mem_monitor()` after every screen switch.
Walking the entire app — every menu row, Settings, "on your marks", the race
face, sixteen splits with their toasts, the action menu, finished, saved and
both summary pages:

| | Value |
|---|---|
| Pool size (`LV_MEM_SIZE`) | 40 KB, of which ~36 KB is usable after LVGL's own bookkeeping |
| Steady-state use | 4.4 KB (a confirm screen) to 18 KB (the race face) |
| **Peak** | **91 %**, about 32.7 KB |
| Fragmentation at peak | up to 69 % |

The peak is **structural, not incidental**. `switchNow()` creates and loads the
new screen before deleting the old one, so both widget trees are alive for the
duration of the switch, and the pool must hold the largest such pair. The first
switch of the run logs 50 % — that is the one with no predecessor to hold on to.
The second logs 90 %, and it never comes down, because `max_used` is a
high-water mark.

**Why this matters more than the number suggests.** There is no MMU. An LVGL
allocation that fails does not raise anything; it returns null and a widget
quietly fails to be created, which on the watch is a null dereference at the
next render. Roughly 3 KB of headroom, against a pool that is 69 % fragmented
at the worst moment, is not much of a margin — and every screen we add later
(the splits face, target pacing) spends some of it.

**We cannot make the pool bigger.** `LV_MEM_SIZE` is set in
`una-sdk/Libs/Header/SDK/Port/LVGL/lv_conf.h`, which is SDK-owned and read-only
for us. It applies to every LVGL app on the platform.

**Not changed in this phase, deliberately.** Two fixes look plausible and both
are GUI-architecture changes that want a watch to verify:

1. **Free the old screen before building the new one.** Halves the peak
   directly. The risk is the window where the display's active screen has been
   deleted and its replacement not yet loaded; it is all inside one
   `lv_async_call` so nothing should render in between, but "should" is doing
   real work in that sentence and the current order is the one the SDK's own
   TouchGFX app uses.
2. **`lv_mem_add_pool()`** with a static buffer of our own. The symbol is
   already linked, and the GUI image uses only 44 % of its 900K region, so the
   memory exists. This adds to the heap rather than restructuring anything.

Changing how screens are torn down, in a packaging phase, against a device I
cannot test on, is how a well-behaved app starts crashing in a race. It goes to
Jon as the first thing to decide after Gate 4.

**On the watch this needs watching anyway.** The simulator's LVGL is the same
code with the same pool size, so the figure should carry over, but fragmentation
depends on allocation order and the watch's renderer may differ in detail.
`ON_WATCH_TESTS.md` T21 covers it.

### 5.7 D2 — FIT sport candidates, first round (superseded by 5.9)

`docs/experiments/build-fit-candidates.sh` writes the same simulated Full race
three times, differing only in the session's `sport` and `sub_sport`, and decodes
each with the independent `fitdecode` library:

| File | sport | sub_sport | Decode |
|---|---|---|---|
| `race-training-generic.fit` | training (10) | generic (0) | 4144 records, 16 laps, ordering and all developer fields correct |
| `race-running-generic.fit` | running (1) | generic (0) | same |
| `race-running-track.fit` | running (1) | track (4) | same |

**Jon did, and all three came back bare. What was wrong with them, and what
replaced them, is 5.9.** Whichever wins becomes the default in `ActivityWriter::TrackData`,
which already takes both as parameters — a one-line change.

**A constraint worth knowing.** These are the only combinations available to us.
`SDK/Fit/FitProfile.hpp` declares `Sport { Generic, Running, Cycling, Training,
Walking, Hiking }` and `SubSport { Generic, Treadmill, Street, Trail, Track,
IndoorCycling }`. The public FIT profile has values that would arguably suit a
HYROX race better — `fitness_equipment`, `hiit`, `cardio_training` — but their
numbers are not in this SDK, and CLAUDE.md forbids inventing FIT profile
numbers. If those are worth testing, Jon needs to confirm the numbers first.

The Phase 0 experiment `batched_laps.cpp` now takes the output name and the two
enums as arguments rather than being copied; with no arguments it behaves
exactly as it did in Phase 0, so the original evidence is unchanged.

### 5.9 The first candidate files came back empty-looking, and why

Jon uploaded the three D2 candidates to Garmin Connect and Strava on 22
September 2026. All three rendered almost identically, and almost bare: no
distance, no pace, Moving Time 0:00, no calories, and sixteen laps numbered 1 to
16 with nothing to say which was the SkiErg.

**The question was whether the data was missing or unreadable. It was missing.**
Decoding the file answers it exactly: Garmin displayed every single field we
wrote and nothing else.

| What we wrote | What Garmin showed |
|---|---|
| `heart_rate` on 4144 records | avg 158, max 184, zones, the chart |
| 16 laps with elapsed/timer time | 16 laps, times correct to the second |
| `segment_type`, `round`, `station_id` per lap | **nothing** -- see below |
| *no distance, anywhere* | Distance `--`, Avg Pace `--:--` |
| *no speed, anywhere* | Moving Time 0:00 |
| *no calories* (deliberate) | Total Calories `--` |

So three separate causes, not one:

1. **Distance and speed were never written.** The brief's decision D9 said not to
   write nominal distances, and `ActivityWriter` duly wrote none -- but nothing
   had connected that decision to the fact that distance is what Garmin and
   Strava hang Distance, Pace, and (through speed) Moving Time on. Without it an
   activity looks broken rather than minimal.
2. **The developer fields work perfectly and are invisible anyway.**
   `segment_type`, `round` and `station_id` decode correctly on every lap.
   Neither Garmin Connect nor Strava displays arbitrary developer fields in a
   lap list. They are worth keeping -- they are how HybridX will map a lap back
   to a segment -- but they will never label a lap for a human.
3. **The timer was never stopped.** We wrote a timer-start event and no
   timer-stop. Harmless in practice but not well-formed.

#### What changed

Jon's decisions, 22 September 2026:

- **Distance: every metre the format states.** Runs 1 km each, plus each
  station's own figure, the SkiErg's and the Row's machine metres included.
  Wall Balls are reps and carry none. A Full race totals **10 480 m**.
  `Station` gained a `distanceM` field and `RaceModel::distanceM()` reads it;
  `RaceTemplateTest` pins every value and all three format totals.
- **Lap names: not yet.** Naming a lap needs the FIT profile's
  `wkt_step_name`, which `SDK/Fit/FitProfile.hpp` does not declare, and
  CLAUDE.md forbids inventing FIT field numbers. Instead the app now writes a
  named **workout** ("HYROX Full Race") with one **workout_step** per segment
  and a `wkt_step_index` on every lap, which is as far as the declared fields
  go. Worth raising with UNA: their `WorkoutStep` namespace omits the one field
  that would let any app name a lap.

Also written now: `Lap::TotalDistance`, `Lap::AvgSpeed`,
`Session::TotalDistance`, `Session::AvgSpeed`, and the missing timer-stop event.
Calories stay absent -- there is still no honest source for them.

#### The cost of "every metre the format states"

Distance divided by time is pace, and a consumer app will show it per lap:

| Lap | Distance | Time | Pace shown |
|---|---|---|---|
| RUN 1 | 1000 m | 5:01 | 5:01 /km |
| SKIERG | 1000 m | 3:23 | 3:23 /km |
| SLED PUSH | 50 m | 3:26 | **68:35 /km** |
| SLED PULL | 50 m | 3:29 | **69:44 /km** |
| WALL BALLS | 0 m | 3:44 | -- |
| **Session** | 10 480 m | 1:09:04 | 6:35 /km |

The runs read perfectly. The sleds read as nonsense, because 50 m in three and a
half minutes IS nonsense as a pace even though it is exactly what happened.
Flagged to Jon with candidate C as the alternative (runs only, 8 km, stations
show no pace at all); it is a one-line change to the `kStations` table either
way.

#### The experiment that caused it

`batched_laps.cpp` was a hand-written stand-in for the writer, built in Phase 0
to answer one narrow question about lap ordering. It answered it. But the files
Jon uploaded came from the stand-in, not from the app, so nobody could tell
whether the app would behave the same -- and the stand-in had been copied from
`ActivityWriter` at a point where neither wrote distance.

It is superseded by `fit_race_sample.cpp`, which drives the **real**
`ActivityWriter` and the **real** `RaceModel` over the SDK's kernel test
doubles. What Jon uploads is now what the watch writes, and
`fit_decode_report.py` prints every field a consumer app reads, flagging the
absent ones, so "is it missing or unreadable?" is never an open question again.

### 5.10 Still to do

- **The LVGL pool (5.6) is the first thing to decide after Gate 4.** It is the
  only finding in this phase that could stop the app working rather than just
  look wrong.
- **D2, round three.** Distance is settled (every stated metre, 10 480 m) and
  every step is now named; what remains is whether a race should call itself
  Running, Cardio or HIIT. Candidates E, F and G (5.11).
- **Gate 4 is still open**: Jon's review of the Phase 4 screenshots, the three
  station abbreviations (4.4), and T1–T8 on a watch.
- **Gate 5's last line is Jon's**: uploading the package to the portal.
- **D1** icon artwork, **D2** the sport choice above, **D5** the publishing
  route. MIT is in place, so either route stays open.
- Phase 6: T9–T15 in the field, then tag `v0.1.0` **and** `apps-v0.1.0`.

### 5.11 Labelled laps, and two gaps in the SDK's FIT profile

Round 2 worked: Jon confirmed 10.48 km, 6:35/km avg pace and sixteen laps with
per-lap distance and pace in Garmin Connect. Two questions came back from it --
can the laps be labelled, and would "Cardio" suit the race better than "Running"
-- and both turn on numbers `SDK/Fit/FitProfile.hpp` does not declare.

**Where the numbers came from.** Not from memory, and not invented. `fitdecode`
ships profile tables generated from Garmin's published FIT SDK, so the data
dictionary is readable locally and, better, *checkable*: we write a field and
the same library reads it back under its proper name. That is the standard this
project needs for a FIT constant, and it is met here.

| What we needed | Where the SDK stops | The dictionary says |
|---|---|---|
| A name on a workout step | `field::WorkoutStep` declares message_index, duration, target and intensity -- every field **except** the name | `wkt_step_name` is field **0**, base type string |
| `sub_sport` for a cardio or HIIT session | `enum class SubSport` stops at `IndoorCycling = 6` | `cardio_training` = **26**, `hiit` = **70**, `strength_training` = 20, `indoor_running` = 45 |

**Lap labels: only one route exists.** The FIT `lap` message has no name field
at all -- it carries `sport`, `sub_sport` and `wkt_step_index`, and that is the
lot. So a lap can only be named by pointing it at a named workout step, which is
what the app now does: a workout of one step per planned segment, each step
named from `RaceModel::name()` ("RUN 1/8", "SKIERG", "SLED PUSH"), and
`wkt_step_index` on every lap. Whether Garmin Connect and Strava actually
*surface* that name is what candidates E, F and G are for.

**Worth taking to UNA**, alongside the other findings: `FitProfile.hpp` omits
`wkt_step_name`, which blocks any app on the platform from labelling a lap, and
its `SubSport` enum covers six of the profile's values, which rules out every
studio and gym sport an app might record. Both are one-line additions. The app
works around them by passing raw numbers to `FitWriter`, which is
profile-agnostic by design -- but an app should not have to.

**The default is now running/generic**, because that is the combination Jon
confirmed reads well. `TrackData` keeps sport and sub_sport as parameters, so
whichever of E/F/G wins is a one-line change.

### 5.12 D2 closed, and a flaw in how it was tested

**Jon's decision, 23 September 2026: `sport = Running(1)`, `sub_sport =
Generic(0)`.** That is already the default in `ActivityWriter::TrackData`, so
nothing needed to change, and it is the combination he had confirmed reads well
in Garmin Connect: 10.48 km, 6:35/km, sixteen laps each with their own distance
and pace.

What he reported of candidates E, F and G was that **none of them changed the
activity type Garmin displayed**, which is what made Running the pragmatic
choice: if the field makes no difference, take the one that already works.

#### The test could not have shown a difference

Every candidate file ever sent — three in round one, four in round two, three in
round three — carried the same hard-coded start:

```cpp
const std::time_t startUtc = 1790000000;   // 2026-09-21 14:13:20 UTC
```

Jon's screenshots say **"21 Sept @ 2:13 pm"** in every round. That is this
constant. Ten files, one timestamp, one duration, ten uploads of what Garmin
Connect had every reason to treat as the same activity.

Consumer platforms de-duplicate on start time, and an activity's type is
generally fixed when it is first filed. So the likely explanation for "none of
them take any activity type through" is not that `sport` is ignored — it is that
**Garmin never created a second activity to apply it to**. The first upload of
all, back in round one, was `race-running-generic.fit`; everything since has
been landing on the activity that created, which is filed as Running.

This is a flaw in the test rig, not in the file, and it is mine. Fixed:
`fit_race_sample.cpp` now ends the race at the moment it is written and takes a
"days ago" argument, so candidates are inherently distinct and can be spaced
apart.

#### Whether it changes the answer

Probably not, and Jon should not feel obliged to re-run it. Running is defensible
on its own terms — a HYROX race is eight kilometres of running plus stations, it
gives per-lap pace, and Garmin's running analysis is the richest it has. But the
evidence for it is currently "the alternative did not visibly fail", which is
weaker than it looked.

Two files exist if he wants ten minutes of certainty: `H-running-fresh.fit` and
`J-cardio-fresh.fit`, a day apart and both after the old fixed date. If J files
as anything other than Running, the sport field does reach Garmin and the choice
was a real one; if it still says Running, the field genuinely is ignored and the
decision stands on firmer ground either way.

#### Still unknown: whether the lap names surface

The names are in the file — `fit_decode_report.py` reads all sixteen back as
`wkt_step_name`, "RUN 1/8", "SKIERG", "SLED PUSH". Whether Garmin Connect *shows*
them in its Laps tab has not been confirmed, and the same collision would have
prevented it: a re-upload onto an existing activity need not rebuild its lap
table.

If they do not surface, there is nothing further to try. The FIT lap message has
no name of its own, and a named workout step is the only mechanism the format
offers. The fallbacks are already built: the developer fields, which HybridX
reads directly, and the watch's own summary, which names every split on the
wrist.

---

## Amendment — adjustable run distance (23 September 2026)

### 5.13 Why, and what it touches

Jon's request: a HYROX run is 1 km, but a test run-through is often done at 500 m
or 800 m, and the app had no way to say so. So the run distance is now a
setting, adjustable in **100 m steps from 100 m to 1 km**, defaulting to the
race distance.

The ceiling is deliberate. This exists to make a sim shorter, not to invent a
longer HYROX; anyone wanting more moves one constant in `RaceData.hpp`.

Nothing about the change was architectural, because the plan was always
generated from parameters rather than hard-coded. `buildTemplate()` is untouched.

| Where | What changed |
|---|---|
| `RaceData.hpp` | Bounds, `clampRunDistanceM()`, and `kRunWorkByHundreds` |
| `RaceModel.hpp` | `distanceM()`, `label()` and `work()` take the run distance, defaulted to 1 km so nothing else had to change at once |
| `Settings` | `runDistanceM`, persisted and clamped on load |
| AppConfig | A fifth field, so a coach can set it from the phone; the manifest and the C++ table still agree under `--check-bounds` |
| GUI | A "Run length" row in Settings, and the race face's work line |
| FIT | Run laps carry the shortened distance, and the session gains `run_distance_m` (developer field 13) |

**`kRunWorkByHundreds` is a table, not a formatter.** Ten strings indexed by
hundreds of metres: the watch formats with integers only (brief 14.4), every
caller wants a `const char*` it does not own, and `[10]` can then read "1 km"
rather than "1000 m", which is how the format writes the race distance.

### 5.14 Two things the change made visible

**A shortened run makes it a sim, and the app now says so.** "On your marks"
reads *"Full sim — 16 segments · 500 m runs"* instead of *"Full race"*, and the
FIT workout is named *"HYROX Full Race, 500 m runs"*. An athlete who left the
setting at 500 m by accident finds out before the gun rather than afterwards.
The session's `run_distance_m` means HybridX never has to infer it from the lap
distances either.

**A `Style::Tip` row draws its title over its value.** The first version of the
Settings row was titled "Run distance", which wrapped to two lines and landed on
top of the "500 m" underneath it — legible in neither direction. Caught in the
simulator, not by a build. It is now "Run length", exactly as long as "Split
lock", so if one fits the other does.

Screens: `screens/phase5-settings-run-length.png`,
`phase5-on-your-marks-sim.png`, `phase5-race-500m.png`.

### 5.15 Verified

- 78 host tests, five of them new: the work table, the clamp (swept over every
  value from 0 to 1200 m, asserting the result always indexes the table), the
  per-segment distance, sim totals, and labels.
- A 500 m sim written through the real `ActivityWriter`:
  `fit-candidates/K-sim-500m-runs.fit`. 6 480 m total, run laps at 500 m,
  stations untouched, `run_distance_m = 500`.
- Watch target and simulator build clean.

---

## Amendment — SDK pin bumped to a7a995a1 (23 September 2026)

### 5.16 Why

Jon asked whether development had tracked the latest SDK. It had not: the pin
set in Phase 0 (0.2) was `b0f8955e`, and by 23 September `main` had moved five
commits ahead. Checked properly rather than assumed stale-and-fine.

`git log --stat HEAD..origin/main` showed four fixes plus the docs commit that
describes them, all landed the same day (21 September, 18:51-21:57), all in the
message-lifetime area:

- **`fix(simulator): destroy messages the way the watch does, and stop leaking
  them`** -- the simulator called `delete msg` (a virtual dispatch); the watch
  destroys messages non-virtually, because a message is constructed inside the
  allocating app's own image and the kernel frees that image on unload. A
  message type with a declared destructor would previously run it in the
  simulator and silently not on the device -- exactly the divergence a
  simulator exists to catch, inverted.
- **`fix(simulator): zero message storage on allocation, as the watch does`**
  -- the kernel's allocator memsets a block; the simulator's did not, so an
  uninitialised field read as garbage in the simulator and zero on the watch.
- **`fix(port): return queued custom messages before the GUI exits`** -- the
  GUI process used to exit holding up to ten pool blocks in its own queue,
  which the kernel's drain cannot see and which are then lost for the life of
  the boot. Now each queued message is answered FAIL and released on exit.
- **`docs(sdk): message lifetime rules...`** -- writes the above down where an
  app author can find it (`sdk-overview.md`,
  `Docs/TouchGFX-Port-Architecture.md`), and fixes several of the SDK's own
  tutorial examples that violated the rules as written (IDs outside the
  application-specific range, a `std::string` member on a message type, a
  payload four times the pool ceiling).

### 5.17 Checked before bumping, not after

Every rule the docs commit states, this app already followed -- checked by
reading `Commands.hpp`, not assumed: no message type declares a destructor, and
every field of every message struct has an explicit in-class default (`= 0u`,
`{}`), so none of them was depending on the old simulator's un-zeroed memory to
happen to work. Nothing here was a latent bug the fixes exposed.

Then verified rather than inferred: fetched `origin/main`, checked it out in a
scratch worktree (`git worktree add`, not touching the working `una-sdk/`
until this was through), and built all three targets against it with zero
changes to our code --

```
host tests:  78/78 pass
simulator:   builds clean, linked
watch:       builds clean, .uapp produced
```

then ran a short race through the simulator against the new pin specifically
to exercise message traffic under the changed destroy/zero/drain behaviour --
split, undo menu, exit -- and watched the log: settings load warning (expected,
first run), then at exit the GUI's queue draining exactly as the new fix
describes (`clearGuiQueue`, `clearCommonQueue`, `Queues cleared`). No crash, no
assert, screens identical to before.

### 5.18 Bumped

`una-sdk/` now checked out at `a7a995a1` (`apps-v1.5.0-rc4-8-ga7a995a1`), five
commits past the Phase 0 pin. `README.md` and `THIRD-PARTY-LICENSES.md`
updated; §0.2's table is left as the historical record of what Phase 0
actually pinned, not rewritten.

Worth doing rather than leaving alone: the two simulator fixes make the
simulator match the watch *more* closely, which matters more here than on a
project that can just test on hardware -- our entire verification strategy
through Phase 5 has been "prove it in the simulator, because there is no
watch." A simulator that now destroys and zeroes messages the way the device
does is strictly less likely to pass something that then fails on hardware.

No code changes were needed anywhere in `hybridx-race/`. This is a pin bump,
not a phase.
