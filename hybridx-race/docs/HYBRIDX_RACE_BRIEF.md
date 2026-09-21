# HybridX Race for UNA Watch: MVP Build Brief

**Owner:** Jon (HybridX)
**Audience:** Claude Code, working in this repository, plus Jon as tester and decision-maker
**Status:** v1.0 of brief, written 21 September 2026
**Working app name:** HybridX Race (placeholder, see Decision D1)

---

## 0. How to use this brief

This document is the single source of intent for the MVP. The UNA SDK repository is the single source of truth for how the platform works. Where the two disagree, the SDK wins, and Claude Code records the discrepancy in `docs/NOTES.md` and tells Jon.

Work proceeds in gated phases (Section 13). At the end of each phase Claude Code stops, summarises what was done, lists anything Jon needs to test or decide, and waits for approval before starting the next phase.

Claude Code must not invent Hyrox pacing data, FIT profile numbers, SDK APIs or manifest keys. If something is not in the SDK source, the SDK docs, the public FIT profile or this brief, it asks.

---

## 1. Product summary

A native activity app for the UNA Watch that times a HYROX-format race or race simulation. The athlete presses one button at the end of every run and every station. The watch shows which segment they are in, how long it has taken, total race time and heart rate, and records every segment as a lap in a FIT file that syncs to Strava and Garmin Connect.

**Primary user:** a hybrid athlete doing a race simulation in a gym, or racing an actual event, wearing the watch on the wrist, often with sweaty hands, high heart rate and no attention to spare.

**Design principles**
1. One button does the job during a race: R2 always means "next segment".
2. Mistakes are recoverable: double presses are blocked, the last split can be undone, a finish can be undone.
3. Glanceable: segment name and segment time readable at arm's length mid-sled-push.
4. Indoor first: no reliance on GPS. HYROX races are held indoors.
5. Standard output: every segment is a FIT lap, so the data works everywhere, including later in HybridX.

---

## 2. Platform facts (verified September 2026)

### 2.1 Hardware and runtime
- Display 240 x 240 px, input from **four buttons only (L1, L2, R1, R2). There is no touchscreen.** Confirm physical positions from `Docs/Tutorials/Buttons` in the SDK (its button naming was corrected in September 2026). R1 is top right.
- ARM Cortex-M33 MCU, FreeRTOS kernel. Apps are position-independent ELF binaries running directly in MCU memory with a shared libc. **There is no MMU**: a bad pointer can crash the watch, not just the app.
- Every app is two processes: a **Service** (background logic, sensors, files) and a **GUI** (screens and buttons), talking through typed kernel messages allocated from kernel pools.
- The GUI is ticked by the kernel at **10 Hz**. Button events arrive per tick.
- Sensors available include `HEART_RATE`, `HEART_RATE_EX` (distinguishes optical vs external strap; the activity apps subscribe to this one), accelerometer, gyroscope, `FUSION_RAW` (IMU at up to 100 Hz), `WRIST_MOTION`, `STEP_COUNTER`, `PRESSURE`, GPS types and battery types.
- Storage: internal flash file system, exposed over USB as mass storage for installing apps and retrieving files.

### 2.2 Important SDK changes since the public v1.2.0 docs
The hosted docs at `developers.unawatch.com/sdk-v1.2.0/` lag behind the repo `main` branch. Three merged changes matter here:

| Change | Merged | Impact on this project |
|---|---|---|
| **LVGL v9.5 is a supported GUI toolkit** with equal standing to TouchGFX (PR #315). Includes `Examples/Apps/RunLVGL` (full Run app incl. intervals, laps, FIT, summary), shared SDK LVGL widgets, and a CMake PC simulator (SDL2) that builds on **Windows and Linux**. | 16 Sep 2026 | Removes the need for the Windows-only TouchGFX Designer. The whole GUI can be written in code and built from the terminal, which suits Claude Code. **This project uses LVGL** (see 5.1). |
| **App configuration fields collected by the companion app** (`SDK::AppConfig`, PR #286). An app declares fields in its manifest; the UNA phone app collects values and writes a JSON file next to the `.uapp`; the app reads it at launch and may write it back. Types: string, bool, int, float. | 19 Aug 2026 | Gives us phone-side settings (e.g. target finish time, Roxzone splits) with no custom Bluetooth work. |
| **Package metadata renamed to `app-manifest.json`**, which must declare `"manifest_version": 1`. `config.json` is no longer the package manifest. | Aug 2026 | Use `app-manifest.json` everywhere the old docs say `config.json`. Validate with the SDK's validator script. |

Also note:
- The kernel **does not stop a service when its GUI closes**. A service that should end must end itself (see `Docs/service-lifecycle.md`, and how Stopwatch and Waypoint handle `COMMAND_APP_NOTIF_GUI_STOP`).
- The kernel interface version (ABI) changed between app releases 1.3 and 1.4. An app built against a newer SDK can refuse to launch on older firmware. **Match the SDK commit to the watch's firmware** (Phase 0).

### 2.3 Distribution
- Direct install: USB mass storage, copy `.uapp` into `Apps/<AppName>/`, eject safely, power-cycle.
- Closed-source publishing: `https://apps.unawatch.com` portal (issues the APP_ID, accepts a zip, publishes versions).
- Open-source publishing: pull request to `github.com/UNAWatch/una-sdk` under `Examples/Apps/`.
- Licensing: UNA's SDK code is MIT. TouchGFX and LVGL are separately licensed third-party components. The MIT licence grants **no rights to the UNA name or logo**.

---

## 3. Development environment

### 3.1 Required software (Jon installs, Claude Code verifies)
- Git (clone **with submodules**: LVGL is a git submodule at `ThirdParty/lvgl`; GitHub release zips do not include submodules).
- **ST ARM GCC toolchain** from STM32CubeIDE or STM32CubeCLT. Distro `gcc-arm-none-eabi` is often incompatible (missing newlib syscall stubs). On Windows, CubeIDE is preferred because CubeCLT lacks `make`.
- CMake 3.21 or newer, `make` (or Ninja where the SDK uses it).
- Python 3 + pip, then `pip install -r $UNA_SDK/Utilities/Scripts/app_packer/requirements.txt`.
- For the LVGL simulator: SDL2 (Linux: system package; Windows: the SDK uses the copy TouchGFX ships, 32-bit, via Visual Studio). Read `Docs/Simulator.md` and `Docs/Tutorials/RunLVGL/ARCHITECTURE.md` for exact steps.
- For font/image conversion (only if new fonts or icons are added): `lv_font_conv` pinned to the version the SDK scripts pin (1.5.3 at time of writing), via `Utilities/Scripts/lvgl_assets/lvgl_assets.py`.
- Garmin FIT SDK tools (FitCSVTool or a Python FIT decoder) for validating output files.

### 3.2 Supported host OS
- **Windows or Linux** for everything including the LVGL simulator.
- macOS: ARM builds are listed as supported by the SDK, but the LVGL simulator is only documented for Windows and Linux. Treat macOS as untested (Decision D7).

### 3.3 Workspace layout

```
workspace/                      <- launch Claude Code here
├── CLAUDE.md                   <- short standing rules (supplied with this brief)
├── una-sdk/                    <- git clone --recurse-submodules; UNA_SDK points here; DO NOT EDIT
└── hybridx-race/               <- this app's own git repo
    ├── docs/
    │   ├── HYBRIDX_RACE_BRIEF.md   <- this file
    │   ├── NOTES.md                <- Claude Code's running log of findings and decisions
    │   ├── ARCHITECTURE.md         <- written in Phase 5, SDK example style
    │   └── screens/                <- simulator screenshots for review
    ├── Software/
    │   ├── Apps/
    │   │   ├── HybridXRace-CMake/  <- CMakeLists.txt, HybridXRaceService.ld, syscalls.cpp
    │   │   └── LVGL-GUI/           <- gui/, assets/, simulator/, lvgl-gui.cmake
    │   └── Libs/
    │       ├── Header/  Sources/   <- Service, RaceModel, ActivityWriter, serialisers
    │       └── libs.cmake
    ├── Tests/Host/                 <- GoogleTest suites, wired like the SDK's Tests/Host
    ├── Resources/
    │   ├── icon_30x30.png          <- REQUIRED by the app merger
    │   └── icon_60x60.png          <- REQUIRED by the app merger
    └── Output/                     <- built .uapp, app-manifest.json, store zip
```

`UNA_SDK` must be set in the environment (see `Docs/sdk-setup.md`; Windows has `Utilities/Scripts/export-stm32-tools.ps1`).

---

## 4. Required reading for Claude Code (in this order, from the SDK repo)

Read these before writing any code, and re-read the relevant ones at the start of each phase.

1. `README.md`, `Docs/sdk-setup.md`, `Docs/platform-overview.md`
2. `Docs/service-lifecycle.md` (what a service receives, how it must end, GUI suspend/resume, memory limits)
3. `Docs/SensorsLayer.md` (and check `HEART_RATE_EX` in `Libs/Header/SDK/SensorLayer`)
4. `Docs/Tutorials/RunLVGL/ARCHITECTURE.md` (LVGL port, ScreenManager, frame loop, pitfalls, TouchGFX vs LVGL)
5. `Docs/Examples/Running-Architecture.md` (the service that RunLVGL copies: Track state, intervals, laps, FIT, WristTiltDetector)
6. `Docs/FitFiles-Structure.md` and `Libs/Header/SDK/Fit/` (`FitWriter.hpp`, `FitProfile.hpp`)
7. `Docs/app-config-json.md`, `Docs/app-config-fields.md`, `Docs/Tutorials/Waypoint/ARCHITECTURE.md` (manifest, config fields, AppConfig read/write)
8. `Docs/deploy.md`, `Docs/Simulator.md`, `Docs/unit-testing.md`
9. Source of `Examples/Apps/RunLVGL`, `Examples/Apps/Running`, and, if present, `Examples/Apps/Workout`, `Examples/Apps/Treadmill` and the Stopwatch app (indoor, non-GPS activity recording and lap timing).

If any file named above does not exist at the chosen SDK commit, find its equivalent and note it in `NOTES.md`.

---

## 5. Template strategy

### 5.1 GUI toolkit decision: LVGL
- Reason: code-only GUI, CMake simulator on Windows and Linux, no Windows-only Designer step, smaller `.uapp`, shared SDK widgets (`SDK::LVGL`: Buttons hints, Title, ScrollIndicator, SensorStatusRow, Battery, TimerRing, Toggle, WheelMenu).
- Risk: LVGL support merged on 16 Sep 2026 and is the newest path. RunLVGL was verified on a watch through the full Run flow including intervals, laps, save, summary and FIT decode.
- Gate: Phase 0 installs the unmodified RunLVGL build on Jon's watch. If it will not run on his firmware and a firmware update is not available, fall back to TouchGFX by copying `Examples/Apps/Running` (Jon will then need TouchGFX Designer on Windows, or Claude Code uses the `tgfx.exe generate` CLI on Windows).

### 5.2 What to copy
- **Primary template:** `Examples/Apps/RunLVGL` (service + LVGL GUI + simulator + CMake). Its service already implements start/pause/resume/stop, manual laps on R2, an interval phase machine with "open" phases advanced by R2, FIT writing, activity summary, settings persistence and wrist-tilt backlight.
- **Reference for indoor recording:** `Examples/Apps/Workout` / `Treadmill` if present (how a non-GPS activity records and writes FIT).
- **Reference for config fields:** `Docs/Tutorials/Waypoint` (AppConfig field table, manifest `configFields`, validator `--check-bounds`, write-back).
- **Do not edit the SDK.** Copy into `hybridx-race/`, reference the SDK through `$UNA_SDK`.

---

## 6. MVP scope

### 6.1 In scope (P0 = must ship in MVP)
| ID | Feature | Priority |
|---|---|---|
| F1 | Race formats: **Full** (8 rounds), **Half A** (rounds 1 to 4), **Half B** (rounds 5 to 8) | P0 |
| F2 | Segment timing driven by R2 ("split"), with segment labels from the station table | P0 |
| F3 | Optional Roxzone splitting (setting, default off) | P0 |
| F4 | Split lockout (ignore R2 for N seconds after a split, default 3 s) | P0 |
| F5 | Undo last split; undo finish | P0 |
| F6 | Pause/resume (for training), end race early, discard | P0 |
| F7 | Race screen: segment name and number, segment time, total time, heart rate with zone, "next up" line | P0 |
| F8 | Split confirmation: haptic pattern plus brief on-screen toast with the finished segment's time | P0 |
| F9 | Heart rate from optical sensor or paired external strap (`HEART_RATE_EX`) | P0 |
| F10 | FIT file with one lap per completed segment, HR records at 1 Hz, developer fields identifying each segment | P0 |
| F11 | On-watch summary: total, runs total, stations total, Roxzone total, avg/max HR, full paged split list; "Last race" available from the main menu | P0 |
| F12 | Settings on watch and via the phone (AppConfig): Roxzone splits, split lockout, vibrate on split | P0 |
| F13 | Valid `app-manifest.json`, icons, store zip, direct USB install | P0 |
| F14 | Target finish time with per-segment targets, ahead/behind delta and projected finish | P1 (needs Jon's data, D6) |
| F15 | Splits face showing the last three segments | P1 |
| F16 | FIT workout/workout_step messages so Garmin Connect shows segment names | P1 (investigate in Phase 0) |

### 6.2 Out of scope for MVP
Custom simulations (station picker, custom run distance), Relay mode, GPS outdoor sims with auto-advance at 1 km, rep counting via IMU, wireless workout push from HybridX, glance, watch face, localisation. See Section 16.

### 6.3 User stories and acceptance criteria
- **US1** As an athlete I start a Full race from the main menu with one press, and the timer starts immediately on RUN 1.
  - AC: race clock is running within one GUI tick (100 ms) of the R1 press on "Start race".
- **US2** At the end of each run and station I press R2 and the watch moves to the next segment, buzzes, and briefly shows the time of the segment I just finished.
  - AC: 16 segments in Full (31 with Roxzone on), labels correct, haptic fires, toast shows for about 2 s.
- **US3** If I double-press R2 by accident, only one split is recorded.
  - AC: a second R2 within the lockout window is ignored and is not recorded anywhere.
- **US4** If I split too early I can undo it.
  - AC: undo merges the last two segments exactly; total time unchanged.
- **US5** When I press R2 at the end of Wall Balls the race finishes and the timer stops, and I can undo if that was a mistake.
- **US6** After saving I can review every split on the watch, and the activity appears on Strava/Garmin Connect with one lap per segment.

---

## 7. Race model specification

### 7.1 Reference data (HYROX 26/27 format)
Station order, distances and reps are fixed and unchanged for the 26/27 season. Every athlete runs 1 km before each station (8 km total). **Jon's HybridX data pack v2.2 (verified against the 26/27 Singles, Doubles and Relay rulebooks) is the source of truth; Jon confirms this table before Phase 1 is signed off.** Station weights are not needed by the app and must not be displayed.

| Round | Station | Work shown on watch |
|---|---|---|
| 1 | SkiErg | 1000 m |
| 2 | Sled Push | 50 m |
| 3 | Sled Pull | 50 m |
| 4 | Burpee Broad Jumps | 80 m |
| 5 | Row | 1000 m |
| 6 | Farmers Carry | 200 m |
| 7 | Sandbag Lunges | 100 m |
| 8 | Wall Balls | 100 reps |

Doubles: for the wearer the segment structure is identical (both partners run every run together and split the station work), so Full covers Doubles. Relay is out of scope.

Implement the table as a `constexpr` array in one header (`RaceData.hpp`) so it can be updated in one place. UI labels use British English.

### 7.2 Segment templates
A segment is `{ type, round (1..8), stationId (1..8 or 0), label }` where `type` is one of `RUN`, `ROX_IN`, `STATION`, `ROX_OUT`.

- **Roxzone off (default):** for each round r in the format: `RUN(r)`, `STATION(r)`.
  - Full = 16 segments; Half A or Half B = 8 segments.
- **Roxzone on:** for each round: `RUN(r)`, `ROX_IN(r)`, `STATION(r)`, `ROX_OUT(r)`, **except that no `ROX_OUT` follows the final station** (the race ends when the last station ends).
  - Full = 31 segments; Half A or Half B = 15 segments.
- Labels: `RUN 3/8 · 1 km`, `ROXZONE IN`, `SLED PULL · 50 m`, `ROXZONE OUT`. For halves, the round counter still uses the real round numbers (e.g. Half B shows `RUN 5/8`).

### 7.3 State machine
States: `IDLE` (menus), `RUNNING`, `PAUSED`, `FINISHED` (timer stopped, not yet saved), `SAVED`, `DISCARDED`.

| Event | From | Effect |
|---|---|---|
| `START(format, roxzone)` | IDLE | Build segment list, open segment 0 at `now`, go RUNNING |
| `SPLIT(t)` | RUNNING | If `t - lastSplitTime < lockout`: ignore (no state change, no record). Else close current segment at `t`, record it, open next at `t`. If the closed segment was the last: go FINISHED |
| `UNDO_SPLIT` | RUNNING, current index > 0 | Remove the current open segment, reopen the previous one (its start time unchanged). Total time unchanged. Remove the corresponding lap from pending output (see 10.1 on how laps are committed) |
| `PAUSE(t)` / `RESUME(t)` | RUNNING / PAUSED | Paused time is excluded from segment active time and total timer time, but included in elapsed time |
| `FINISH_EARLY(t)` | RUNNING, PAUSED | Close current segment at `t`, go FINISHED, mark race incomplete |
| `UNDO_FINISH` | FINISHED (reached via final SPLIT only) | Reopen the last segment as if the finishing press never happened; go RUNNING |
| `SAVE` | FINISHED | Write session, finalise FIT, write summary, go SAVED |
| `DISCARD` | RUNNING, PAUSED, FINISHED | Abort FIT file, go DISCARDED |

### 7.4 Timing rules
- `RaceModel` is pure C++ with **no SDK dependencies**. Time is injected through a small clock interface so host tests can simulate a 90-minute race instantly.
- On the watch, monotonic time comes from the kernel's millisecond clock. **Handle wrap-around** with unsigned subtraction (RunLVGL notes that the kernel clock returns to zero short of 2^32). UTC wall time is used only for FIT timestamps.
- Split timestamps are taken **when the button is pressed in the GUI** and sent to the service with the message, so the up-to-100 ms tick latency does not bias splits.
- Internal resolution: milliseconds. Display resolution: whole seconds. Segment time format `m:ss` (or `h:mm:ss` if over an hour); total time `h:mm:ss`.
- Do not format floats with `snprintf` on the watch (the kernel's exported `snprintf` float support is not guaranteed). Use integer formatting as in RunLVGL's `Format.hpp`.

### 7.5 Invariants (host tests must assert these)
1. Sum of recorded segment active durations equals total active time (tolerance 1 ms per segment).
2. Number of recorded laps equals number of completed segments.
3. A split inside the lockout window changes nothing.
4. `UNDO_SPLIT` followed by the same `SPLIT` again produces an identical state to never having undone.
5. `UNDO_FINISH` then a `SPLIT` produces the same final result as a single finishing split at that time.
6. Pausing for P ms during segment k adds exactly P ms to elapsed time and 0 ms to segment k's active time.
7. Segment counts: Full 16/31, Half 8/15 for Roxzone off/on.

### 7.6 Target pacing (P1, only once Jon supplies data)
- Input: `targetFinishMin` (AppConfig, 0 = off).
- Per-segment target = target total × share[segment]. **The share table comes from Jon's HybridX race-results dataset (Decision D6). Do not invent it.** Until supplied, the feature stays hidden.
- Display: cumulative delta vs target (`+0:23` behind in amber, `-0:15` ahead in green) and projected finish = elapsed + remaining targets.

---

## 8. User experience specification

### 8.1 Button map
| Context | L1 | L2 | R1 | R2 |
|---|---|---|---|---|
| Menus | Follow RunLVGL wheel conventions exactly (do not invent new ones) | | Select | Back / exit |
| Race screen | Previous face | Next face | Open action menu (race clock keeps running) | **Split** |
| Action menu | Up | Down | Select | Close menu |
| Finished screen | | Undo finish | Save | |
| Summary | Page up | Page down | | Exit |

Confirm against RunLVGL's actual mapping in Phase 0 and adjust the menu rows to match it; the race-screen mapping above is fixed.

### 8.2 Screens (LVGL)
1. **Main** (wheel menu): `Start race` (hint shows format and Roxzone state), `Format` (Full / Half: 1 to 4 / Half: 5 to 8), `Last race` (only if a summary exists), `Settings`. Sensor status row shows HR status. No GPS gating.
2. **Settings**: Roxzone splits (toggle), Split lockout (1 to 10 s picker), Vibrate on split (toggle), Target finish (read-only value set from the phone; hidden until F14). Changes are saved through AppConfig (see 10.3).
3. **Race** (faces cycled with L1/L2):
   - **Main face:** top: segment label (e.g. `SLED PUSH · 50 m`) and segment number; centre: segment time, largest font available; below: total time; bottom: HR with zone arc; small "Next: Sled Pull" line. Accent colour by segment type (run, station, roxzone), chosen for contrast on the watch's 2-bit-per-channel colour display.
   - **Splits face (P1):** last three completed segments with times, plus target delta and projected finish when F14 is active.
   - **Status face:** time of day and battery (reuse RunLVGL's status face).
4. **Split toast:** overlay for about 2 s after each split: finished segment name and time (`SkiErg 4:12`). Must not block the next split beyond the lockout.
5. **Action menu** (R1 during race): `Resume` (default), `Undo last split`, `Pause` / `Resume timer`, `End race` (hold R1 1.5 s, reuse RunLVGL's hold-confirm), `Discard` (hold R1 1.5 s). Auto-closes back to the race screen after 10 s without input. **Never exits the app on idle during a race.**
6. **Finished:** total time large; `R1 Save`, `L2 Undo`. Auto-save after 60 s without input (Decision D4).
7. **Summary:** overview (total, runs total, stations total, Roxzone total if used, avg and max HR) then paged split list (5 rows, L1/L2 to page).
8. **Saved / Discarded** result screens: reuse RunLVGL's.

### 8.3 Feedback
- Haptic patterns (use the vibration and buzzer requests the Running app uses): entering a RUN = one strong pulse; entering a STATION = two pulses; entering Roxzone = one short pulse; finish = one long pulse. Split rejected by lockout = no feedback.
- Backlight on at every split. During a race use `WristTiltDetector` (as Running does, fed by `FUSION_RAW` at 100 Hz) because simple wrist-motion events fire constantly during sleds, rowing and wall balls. Outside a race use `WRIST_MOTION`.

### 8.4 Idle and suspend behaviour
- Every **menu** screen implements the idle timeout and exits the app (the RunLVGL review found 10 menu screens missing this; do not copy that gap).
- Race, action menu, finished and summary screens never exit on idle.
- If the GUI is suspended while a hold-to-confirm is in progress, cancel the hold and return to the previous screen on resume (known RunLVGL edge case where a suspend during the hold completed the action).

---

## 9. Service architecture

### 9.1 Structure
- Start from RunLVGL's service (copied from Running). Remove: GPS connections and map building, distance/time auto-laps, the interval phase machine and interval settings. Add: `RaceModel`, segment-aware haptics, race messages, AppConfig.
- Keep: track lifecycle patterns, `ActivityWriter` (adapted), activity summary serialiser pattern, settings serialiser pattern, battery sampling, `WristTiltDetector`, backlight helper.

### 9.2 Sensors
| Sensor | When connected | Use |
|---|---|---|
| `HEART_RATE_EX` (1000 ms) | Race start to race end | Live HR (displayed unconditionally) and FIT records (recorded only when bpm > 20 and trust level 1 to 3, as Running does) |
| `BATTERY_LEVEL`, `BATTERY_METRICS` | Race start to race end | Status face, FIT battery developer fields |
| `FUSION_RAW` (100 Hz) | Race start to race end | `WristTiltDetector` backlight |
| `WRIST_MOTION` | While GUI visible | Backlight outside a race |
| GPS types | **Never** in MVP | |

Always check `isDataValid()` on every parser. (For GPS in future work, the fix flag is `isCoordinatesValid()`, not `isDataValid()`.)

### 9.3 Messages (Commands.hpp)
Follow whichever pattern RunLVGL uses (CustomMessage structs with fixed hex IDs, or GSModel/GSBridge). Proposed set:

- Service to GUI: `SETTINGS_UPDATE`, `LOCAL_TIME`, `BATTERY`, `HR_UPDATE`, `RACE_STATE_UPDATE`, `RACE_DATA_UPDATE` (1 Hz and on every split: current index, segment elapsed, total elapsed, HR, flags), `SPLIT_EVENT` (closed segment index and duration, drives the toast and haptics), `RACE_FINISHED`, `SUMMARY`.
- GUI to service: `SETTINGS_SAVE`, `RACE_START(format)`, `RACE_SPLIT(pressTimeMs)`, `RACE_UNDO_SPLIT`, `RACE_PAUSE`, `RACE_RESUME`, `RACE_FINISH_EARLY`, `RACE_UNDO_FINISH`, `RACE_SAVE`, `RACE_DISCARD`.

Message structs must stay small and fixed-size (no `std::vector` inside messages). The summary follows the Running app's pattern for passing `ActivitySummary`.

### 9.4 Lifecycle
- Read `Docs/service-lifecycle.md` and record in `NOTES.md` exactly how RunLVGL behaves if the GUI is closed mid-activity. Replicate that behaviour for a race in progress.
- When no race is in progress and the GUI stops, the service must end itself and disconnect all sensors (the Waypoint tutorial's bug was a leaked service still holding sensors after exit).
- Read AppConfig in `run()`, not in the Service constructor (in the simulator the logger does not exist yet at construction time).

---

## 10. Data and persistence

### 10.1 FIT activity file
Adapt the RunLVGL/Running `ActivityWriter` (`SDK::Fit::FitWriter` + `FitProfile.hpp`). Keep file naming and folder layout (`activity_YYYYMMDDTHHMMSS.fit` under `YYYY/MM/`).

- **File ID, Developer Data ID, Field Descriptions:** as the template.
- **Records:** 1 Hz, timestamp + heart rate (+ battery developer fields, as the template). No position, speed or altitude.
- **Events:** timer start/stop for pause and resume, as the template.
- **Laps: one per completed segment.**
  - Because a split can be undone, laps are **not** streamed at split time. Keep the segment list in RAM (at most 31 small structs, with per-segment HR sum, count and max so merges on undo are exact) and write all Lap messages in chronological order at `SAVE`, before the Session message. Claude Code must confirm this ordering decodes cleanly with the FIT SDK and imports correctly to Strava and Garmin Connect; if not, switch to writing each lap when it can no longer be undone and restrict undo accordingly.
  - Fields: timestamp (segment end), start_time, total_elapsed_time, total_timer_time, avg_heart_rate, max_heart_rate, message_index (0-based, sequential), lap trigger = manual if the profile field is added.
  - Distance: **not written** in MVP (nominal distances would give misleading pace data; Decision D9).
  - Developer fields on Lap: `segment_type` (uint8: 0 run, 1 roxzone in, 2 station, 3 roxzone out), `round` (uint8, 1 to 8), `station_id` (uint8, 1 to 8, 0 if not a station).
- **Session:** sport and sub-sport per **Decision D2**. Only use values from the public Garmin FIT profile; add any missing enum values to `FitProfile.hpp` with their published numbers. Developer fields on Session: `race_format` (uint8: 0 full, 1 half A, 2 half B), `roxzone_mode` (uint8), `completed` (uint8: 1 if finished normally, 0 if ended early).
- **Activity:** as the template.
- **P1 (F16):** investigate how the template uses the `L_WORKOUT` / `L_WORKOUT_STEP` local types; if the activity file can carry a workout with named steps and each lap's `wkt_step_index`, Garmin Connect may show segment names. Report findings before implementing.

### 10.2 On-watch summary
JSON via the template's `ActivitySummarySerializer` pattern: format, Roxzone flag, start UTC, completed flag, total active time, runs total, stations total, Roxzone total, HR avg/max, and a per-segment list `{type, round, stationId, durationMs}`. Loaded on app launch for `Last race`. Use coreJSON-based parsing that checks types (note: `JsonStreamReader` getters do not type-check; `get(query, int32_t&)` on a string returns true with 0).

### 10.3 Settings and phone configuration (AppConfig)
Use `SDK::AppConfig` for all user settings so the phone and the watch edit the same file. Values are read once at launch; last writer wins. Watch-side edits call `save()` (crash-safe, preserves unknown keys).

| id | type | default | min | max | unit | label |
|---|---|---|---|---|---|---|
| `roxzoneSplits` | bool | false | | | | Roxzone splits |
| `splitLockoutSec` | int | 3 | 1 | 10 | s | Split lock |
| `vibrateOnSplit` | bool | true | | | | Vibrate on split |
| `targetFinishMin` | int | 0 | 0 | 240 | min | Target finish (0 = off) |

The C++ `constexpr` field table and the manifest's `configFields` must match; CI-style check with the SDK validator's `--check-bounds` option (see `Docs/app-config-fields.md`). The last-used race format may be stored in the app's own settings JSON (template `SettingsSerializer`), not AppConfig.

### 10.4 Companion app activity report
`Docs/app-config-json.md` describes how manifest flags (`supportsLaps`, `supportsHeartbeat`, etc.) map to a CBOR activity report the UNA phone app displays. In Phase 0, find where Running/RunLVGL produce this report (or confirm it is derived by the system) and mirror it: lap start times per segment and average heart rate per lap.

---

## 11. SDK compliance checklist (documentation and packaging requirements)

Claude Code ticks these off in `NOTES.md` with evidence (file path, command output).

### 11.1 Project and build
- [ ] App lives outside the SDK; builds from `Software/Apps/HybridXRace-CMake` with `cmake -G "Unix Makefiles" -S . -B build` then `cmake --build build` (or the generator RunLVGL uses).
- [ ] `CMakeLists.txt` sets: `APP_NAME` (`HybridXRace`, no spaces), `APP_ID` (16 uppercase hex characters), `DEV_ID`, `APP_TYPE` (`Activity`, as in the Running example), `APP_AUTOSTART` Off, and the path variables (`LIBS_PATH`, `OUTPUT_PATH`, `RESOURCES_PATH`, GUI path).
- [ ] Linker script renamed to `${APP_NAME}Service.ld` to match `APP_NAME`.
- [ ] `APP_ID`: during development, generate locally with the SDK's documented md5 method and **never reuse** RunLVGL's ID. Before publishing, create the app on `apps.unawatch.com`, paste the portal's App ID into `CMakeLists.txt`, re-run CMake and confirm the ID in the `app_merging.py` log.
- [ ] Memory: start from RunLVGL's values (its GUI RAM was raised to 900K; LVGL's GUI RAM is about 139 KB for frame, stripe and a 40 KB pool). Log pool peak use and record measured figures.
- [ ] Versioning: the SDK derives `BUILD_VERSION` from git tags. Tag the app repo (`v0.1.0` etc.) and keep `appVersion` in the manifest in step.
- [ ] `Resources/icon_30x30.png` and `Resources/icon_60x60.png` present (the merge step fails without them). Placeholder icons are fine until Jon supplies HybridX artwork.
- [ ] Output: `.uapp` in `Output/`.

### 11.2 Manifest (`Output/app-manifest.json`)
Draft to adapt; validate against the SDK's `app-config.schema.json` and `validate_app_config.py`. Keys follow the current SDK docs; if the docs at the chosen SDK commit differ, the docs win.

```json
{
  "manifest_version": 1,
  "type": ["activity"],
  "name": "HybridX Race",
  "icon": "icon.png",
  "binary": "<built .uapp file name>",
  "previews": "previews/",
  "appVersion": "0.1.0",
  "minKernelVersion": "<lowest kernel this build supports; confirm in Phase 0>",
  "requiredHardware": ["HR", "ACCELEROMETER"],
  "description": "Race and simulation timer for HYROX-format events. One button per split, every run and station recorded as a lap.",
  "supportsLaps": true,
  "supportsDistance": false,
  "supportsTrack": false,
  "supportsHeartbeat": true,
  "supportsElevation": false,
  "supportsStep": false,
  "supportsSpeed": "none",
  "customMeasures": [],
  "stravaExport": true,
  "id": "<APP_ID>",
  "configFile": "app_config.json",
  "configFields": [
    { "id": "roxzoneSplits", "type": "bool", "label": "Roxzone splits",
      "description": "Record Roxzone in and out as separate laps.", "default": false },
    { "id": "splitLockoutSec", "type": "int", "label": "Split lock",
      "description": "Ignore the split button for this many seconds after a split.",
      "default": 3, "min": 1, "max": 10, "unit": "s",
      "validationMessage": "Between 1 and 10 seconds." },
    { "id": "vibrateOnSplit", "type": "bool", "label": "Vibrate on split",
      "description": "Vibrate when a new segment starts.", "default": true },
    { "id": "targetFinishMin", "type": "int", "label": "Target finish",
      "description": "Target race time in minutes. 0 turns targets off.",
      "default": 0, "min": 0, "max": 240, "unit": "min",
      "validationMessage": "Between 0 and 240 minutes." }
  ]
}
```

- [ ] `minKernelVersion` set to the floor the SDK tooling derives (the SDK has a `min_kernel_version.py` helper; the floor was 1.4.0, ABI 3, in August 2026).
- [ ] Manifest validates; field table matches the C++ table (`--check-bounds`).

### 11.3 Store package (closed-source route)
- [ ] Zip containing: the `.uapp`, `app-manifest.json`, `icon.png`, `previews/` (PNG screenshots from the simulator), and `assets/icons` and `assets/previews` if used. Follow `Docs/deploy.md` at the chosen commit (its screenshots still show the old `config.json` name).
- [ ] Upload under the app's **Version** tab on the portal, then Release and Confirm and Publish. (Jon does this step.)

### 11.4 Direct install (development)
1. Connect the watch by USB and wait for mass storage (running apps may need to flush first).
2. Create `Apps/HybridXRace/` and copy the `.uapp` into it.
3. Eject safely, disconnect, power-cycle the watch, find the app from the top right button.
4. If the app is missing, compare file hashes; for deeper faults, logs come from the debug UART via UNA's Dev tool. Check `Utilities/Scripts` for helper scripts (e.g. `Update-Watch-Apps.ps1` on Windows).

### 11.5 Documentation the project must produce
- [ ] `docs/NOTES.md`: running log of findings, versions, decisions, measured memory and battery figures, discrepancies between docs and code.
- [ ] `docs/ARCHITECTURE.md` in the same structure as the SDK's example architecture docs (Overview, Architecture, Service, GUI, Data, Build, Simulator). Required if the app is ever contributed as open source, and useful for future Claude Code sessions either way.
- [ ] `README.md`: build, simulate, install, test steps for Jon.
- [ ] `CHANGELOG.md` from v0.1.0.

### 11.6 Licensing and naming
- [ ] Keep third-party licence notices for LVGL (and TouchGFX if used) as the SDK does.
- [ ] Do not use the UNA name or logo in the app name or icon (MIT grants no trademark rights).
- [ ] "HYROX" is a third-party trademark: do not put it in the app name or icon without Jon's decision (D1). Descriptive use in the description is Jon's call.

---

## 12. Testing

### 12.1 Host unit tests (GoogleTest, run on Jon's PC with no watch)
Wire `Tests/Host` like the SDK's harness (`Docs/unit-testing.md`). Minimum suites:
- `RaceTemplateTest`: segment counts and labels for Full/Half A/Half B with Roxzone off/on (16/8/8 and 31/15/15), round numbering in halves, no `ROX_OUT` after the final station.
- `RaceStateTest`: every transition in 7.3, including illegal events (e.g. `UNDO_SPLIT` at index 0 does nothing).
- `RaceTimingTest`: the invariants in 7.5, clock wrap-around, pause accounting, lockout boundary (exactly at N seconds is accepted).
- `LapAccumulatorTest`: HR sum/count/max merge on undo.
- `FitOutputTest` (host build of the writer against a mock file system, if the SDK's mocks allow): lap count equals segments; decode the file with a FIT decoder and check lap durations.
- `AppConfigFieldsTest`: defaults, clamping, and that the C++ table matches the manifest.

### 12.2 Simulator
- The LVGL simulator maps keyboard keys 1 to 4 to L1, L2, R1, R2 (confirm in `Docs/Simulator.md`). The simulated heart-rate sensor serves `HEART_RATE_EX`.
- Claude Code captures a screenshot of every screen into `docs/screens/` at the end of Phase 4 for Jon to review.

### 12.3 On-watch test protocol (Jon runs, Claude Code prepares a checklist file)
| # | Test | Pass condition |
|---|---|---|
| T1 | Install and launch | App appears, main menu renders, exits cleanly |
| T2 | Short Full race (20 to 30 s per segment) | 16 segments, correct labels, haptics per type |
| T3 | Roxzone on | 31 segments |
| T4 | Double press R2 | One split only |
| T5 | Undo last split | Segments merged, total unchanged |
| T6 | Pause and resume | Pause excluded from active time |
| T7 | Finish then undo finish, then finish | Single correct finish |
| T8 | End early and Discard | Correct summary; discarded file removed |
| T9 | Leave app mid-race and return | Behaviour matches `NOTES.md` description |
| T10 | External HR strap | HR shown and recorded |
| T11 | FIT import | Strava and Garmin Connect show one lap per segment with correct times |
| T12 | Phone config change | New value applied on next app launch |
| T13 | Real gym simulation | Usable with sweaty hands; no missed or phantom splits |
| T14 | Battery over 90 minutes | Record drain in `NOTES.md` |
| T15 | Exit with no race | No leaked service (sensors disconnected) |

---

## 13. Build plan for Claude Code (gated phases)

Use plan mode at the start of each phase, commit at the end of each phase, and stop for Jon's review at every gate.

### Phase 0: Orientation and environment
- Jon: install the toolchain (3.1), clone the SDK with submodules, set `UNA_SDK`, find the watch's firmware/kernel version (watch settings or the UNA phone app).
- Claude Code: read Section 4 docs; list SDK tags and pick the commit compatible with Jon's kernel; build RunLVGL for the watch and for the simulator **unchanged**; run the SDK host tests.
- Write `NOTES.md` covering: SDK commit, kernel compatibility, RunLVGL button mapping, GUI-stop behaviour mid-activity, how the companion activity report is produced, how `L_WORKOUT`/`L_WORKOUT_STEP` are used, whether Workout/Treadmill/Stopwatch exist and what is reusable, any conflicts with this brief.
- **Gate 0:** Jon installs the unmodified RunLVGL `.uapp` on the watch and confirms it runs. If not, decide on firmware update or TouchGFX fallback.

### Phase 1: RaceModel core
- Pure C++ `RaceData.hpp`, `RaceModel.hpp/.cpp` with injected clock; host tests from 12.1 (template, state, timing, accumulator).
- **Gate 1:** all host tests pass; Jon confirms the station table (7.1).

### Phase 2: App scaffold
- Copy RunLVGL into `hybridx-race/`, rename (APP_NAME, new dev APP_ID, linker script, project names), placeholder icons, builds for the watch and simulator with RunLVGL behaviour unchanged under the new name.
- **Gate 2:** Jon installs it and sees "HybridX Race" on the watch.

### Phase 3: Service integration
- Replace intervals/GPS logic with RaceModel; messages (9.3); sensors (9.2); haptics and backlight (8.3); AppConfig (10.3); FIT changes (10.1); summary (10.2); lifecycle (9.4).
- **Gate 3:** simulator run of a full race produces a FIT file that decodes with correct laps; host tests still pass.

### Phase 4: GUI
- Screens in 8.2 built on SDK LVGL widgets; idle and suspend rules (8.4); screenshots to `docs/screens/`.
- **Gate 4:** Jon reviews screenshots and runs T1 to T8 on the watch.

### Phase 5: Packaging and documentation
- Manifest, validation, store zip, `README.md`, `ARCHITECTURE.md`, `CHANGELOG.md`, on-watch test checklist file.
- **Gate 5:** manifest validates; zip structure matches `Docs/deploy.md`.

### Phase 6: Field testing and release candidate
- Jon runs T9 to T15; Claude Code fixes issues; tag `v0.1.0`.
- **Gate 6:** Jon decides on publishing route (D5).

---

## 14. Known pitfalls (from the SDK's own history)

1. **Leaked services:** the kernel does not stop a service when its GUI closes. End the service and disconnect sensors when appropriate.
2. **GPS validity:** `GpsLocation::isDataValid()` only means a well-formed record; the fix is `isCoordinatesValid()`. (Not used in MVP, but relevant for any GPS work.)
3. **JSON types:** `JsonStreamReader` getters do not check types. Use AppConfig for settings and typed parsing elsewhere.
4. **Float formatting:** avoid `snprintf("%f")` on the watch; format with integers.
5. **Tick maths:** the GUI runs at 10 Hz; derive tick counts from milliseconds with the SDK's helper macros, never hard-code "60 ticks = 2 seconds".
6. **Clock wrap:** the kernel ms clock wraps; always subtract as unsigned deltas.
7. **LVGL teardown:** destroy SDK LVGL widgets (which own timers and animations keyed on the C++ object) before deleting their parent objects; switch screens through a deferred ScreenManager as RunLVGL does, never deleting a screen inside its own event.
8. **Button delivery:** one key event per frame is delivered to the active screen; a press that switches screens means the release goes to the new screen. Test hold-to-confirm flows carefully.
9. **Idle and suspend:** every menu must handle idle timeout; a suspend during a hold-to-confirm must cancel the hold.
10. **ABI and firmware:** build against the SDK commit that matches the watch kernel. A mismatch can stop the app launching or, in one past case, corrupt memory.
11. **Simulator vs watch:** the simulator's source lists are separate from the ARM build's globs; a file added to one may be missing from the other. Build both after every change.
12. **Heart-rate gating:** the trust-level gate is for what goes into the FIT file; live readouts are shown unconditionally.
13. **Messages:** allocate only from kernel pools, always release, keep structs fixed-size.
14. **No MMU:** bounds-check every array index (segment index, summary list paging, string buffers).

---

## 15. Open decisions for Jon

| ID | Decision | Default until decided |
|---|---|---|
| D1 | App name and icon (avoid UNA marks; decide on use of "HYROX") | "HybridX Race", placeholder icon |
| D2 | FIT sport and sub-sport for the session (affects how Strava and Garmin label it) | Claude Code produces test files with two or three candidate values from the public FIT profile; Jon uploads them and picks |
| D3 | Roxzone splits default | Off |
| D4 | Auto-save after finishing if no input | Yes, after 60 s |
| D5 | Publishing route: closed source via portal, or open source via PR (MIT) | Undecided; build either way |
| D6 | Pacing share table for target splits (from the HybridX results dataset) | F14 hidden until supplied |
| D7 | Development OS | Windows or Linux; macOS untested for the simulator |
| D8 | Split lockout default | 3 s |
| D9 | Write nominal distances (1 km per run, station distances) into FIT laps | No |

---

## 16. Post-MVP roadmap (not to be built now)

- **v1.1:** custom sims (choose stations, run distance 500 m or 1 km, number of rounds); Relay (two runs and two stations per athlete).
- **v1.2:** target pacing from the HybridX dataset (F14) and the splits face (F15); FIT workout-step names (F16).
- **v1.3:** outdoor sims with GPS and automatic run completion at the set distance.
- **v2:** wall ball and burpee rep counting from the IMU (research project; needs recorded training data).
- **HybridX integration:** read completed activities via Strava into HybridX, map laps to segments using the developer fields or lap order, and compare splits against the race-results dataset; deliver simple presets to the watch through AppConfig fields (the values file is capped at 8 KB).
- **Glance:** last race time and best splits.

---

## 17. References

- UNA SDK repository: https://github.com/UNAWatch/una-sdk (read `Docs/` at the chosen commit)
- Hosted SDK docs (older snapshot): https://www.developers.unawatch.com/sdk-v1.2.0/platform-overview.html
- App portal: https://apps.unawatch.com
- SDK pull requests referenced: #286 (AppConfig and `app-manifest.json`), #308 (service lifecycle doc), #315 (LVGL support and RunLVGL)
- UNA community: GitHub Discussions and Issues on the SDK repo
- HYROX rulebooks (26/27): https://hyrox.com/rulebook
- Garmin FIT SDK and profile: https://developer.garmin.com/fit/
