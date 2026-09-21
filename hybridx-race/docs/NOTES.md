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
| D2 | FIT sport / sub_sport | Phase 3 | `Training(10)` / `Generic(0)` — the best named pair in `FitProfile.hpp`; arbitrary bytes are writable, so candidates are cheap |
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
| LVGL pool peak | 50 % of 35 936 B |

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
