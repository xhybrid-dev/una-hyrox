# HybridX Run: notes

A test app: the SDK's RunLVGL copied, plus a VO2max estimate worked out live
during a run. It exists to check the method against a Garmin worn on the
other wrist, before deciding how real athletes (who use UNA's own Running
app) could get a VO2max.

## Why a separate run app (10 October 2026)

- An app can't read UNA Running's activities: storage is sandboxed to the
  app's own folder plus `Apps/SharedData/`, and the phone deletes each
  `.fit` after sync (`hybridx-streak/docs/NOTES.md`, Gate 0 and "The UNA
  Android app").
- So the only on-watch route is our own app computing during the run. Its FIT
  file is the same as RunLVGL's, so UNA's phone app still syncs it (and on to
  Strava): the test costs the tester nothing.
- For athletes on UNA Running, the routes are UNA (a per-split summary in
  `SharedData`, or VO2max itself) or server-side from Strava's streams. Both
  were put to Jon; this app tests the method either would use.

## Decisions (Jon, 10 October 2026)

- Base: the SDK's **RunLVGL** (LVGL, as Intervals and Trail).
- Max HR: the athlete's entered value; else **Tanaka, 208 − 0.7 × age**
  (Tanaka, Monahan and Seals 2001); age from birth year and month.
- **Auto-raise** max HR from a run only when none was entered.
- Name **HybridX Run**, tags `run-vX.Y.Z`.

## The method

Published methods only.
- **Oxygen cost of running**, the ACSM metabolic equation (ACSM's Guidelines
  for Exercise Testing and Prescription): VO2 = 3.5 + 0.2·v + 0.9·v·grade,
  ml/kg/min, v in m/min, grade as a fraction. ACSM gives it for level and
  uphill running; downhill grade is taken as 0.
- **Intensity**: %HRR ≈ %VO2R (Swain and Leutholtz 1997), so
  VO2max = 3.5 + (VO2 − 3.5) / %HRR, with %HRR = (HR − RHR) / (HRmax − RHR).
- Applied to steady one-minute windows; the run's figure is the median of the
  accepted windows; the shown figure is a weighted mean of recent runs.
- The acceptance thresholds are provisional, in one header, for Jon to tune
  against his Garmin (R1).

## R0: the app starts from RunLVGL (10 October 2026)

- **Verbatim copy first** (as Intervals P3a, Trail T2): `Software/` is the
  SDK's RunLVGL (`a7a995a1`), committed unchanged on its own. GUI
  `Apps/LVGL-GUI`, CMake `Apps/HybridXRun-CMake`, service `Libs/App`; the pure
  core will be `Libs/Core`; `Libs/libs.cmake` compiles both.
- **Identity.** `HybridXRun`, type `Activity`, development APP_ID
  `74CDEFFE3DBA395D` (first 16 hex of md5("HybridXRun")), "HybridX Run" on the
  watch (11 bytes, under app_merging.py's 15). Version from `run-v*` tags
  (`Software/cmake/run-version.cmake`). Start screen title "RUN".
- **Icon** (`Resources/make_icons.py`, Intervals' method): a grey dial whose
  top end is lime, needle in the lime.
- **CI** builds the app and adds its `.uapp` to `watch-apps`.

### R0 verified
- Watch target, local compile check (Ubuntu's `arm-none-eabi-gcc` 13.2 with
  Race's syscall stubs built `-mfloat-abi=hard -mfpu=fpv5-sp-d16`, so **not
  installable**; the watch copy comes from CI): `HybridXRun_0.0.0-dev.uapp`,
  419,996 bytes. No warnings outside SDK files.
- Simulator builds and runs (dummy video driver, 5 s, clean shutdown).
- Behaviour is still RunLVGL's.

## R1: the VO2max core (10 October 2026)

Pure C++ in `Software/Libs/Core`, no SDK headers, host-tested.
- `Vo2Profile`: age from birth year and month (month unset = January) at the
  watch's UTC; max HR entered > Tanaka (raised to the auto max if that is
  higher) ; resting HR entered > the watch's daily figure. Status says what
  is missing: birth year or max HR, resting HR, or a reserve under 40 bpm.
- `Vo2Run`: fed once a second. One-minute windows; a pause drops the part
  window. A window keeps its ACSM cost and mean HR only if it is past the
  warm-up, has 55+ good seconds (HR trust 1-3, valid GPS speed without dead
  reckoning, running pace), is steady (speed spread, HR range) and not steep.
  `estimate()` applies the profile (%HRR floor, plausibility) and takes the
  median; at least 5 windows. Also the auto max: the highest HR held for 5 s
  with good trust. About 1 KB, fixed.
- `Vo2History`: a ring of the last 10 runs plus the auto max; the shown value
  is the latest 5 weighted by window count (capped at 30). Its own JSON
  writer and bounded reader (integers only; anything malformed reads as
  empty), and the SharedData file.
- `Vo2Text`: "52.3", and the one-line reasons.

### Provisional limits (`Vo2Config.hpp`), for Jon to tune

| Limit | Value | Basis |
|---|---|---|
| Window | 60 s, 55 good seconds | chosen |
| Warm-up ignored | 300 s | chosen: HR lag |
| Slowest pace | 134 m/min (7:28 /km) | ACSM's running-equation range |
| Speed spread | 8% | chosen |
| HR range in a window | 10 bpm | chosen |
| Grade | -3% to +10%; gentle downhill as 0 | ACSM covers level and uphill; limits chosen |
| Lowest intensity | 50% HRR | chosen |
| Plausible VO2max | 15-95 | chosen |
| Windows per run | at least 5 | chosen |
| Auto max hold | 5 s, after the warm-up | chosen |
| Auto max confirmation | reached on 2 of the last 10 runs (review, 11 Oct) | chosen |
| Shown value | latest 5 runs, weight cap 30 | chosen |

### R1 verified
- Host tests: 33 pass (`hybridx-run-host-tests`), also under ASan + UBSan.
  The steady-run test checks the arithmetic by hand: 200 m/min level, HR 160,
  rest 50, max 190 gives 54.4.
- The core compiles for the watch (both processes) with no warnings.

## R2: the service works it out (10 October 2026)

- **The athlete's numbers** are config fields (`Resources/app-manifest.json`,
  `AppConfigFields.cpp`; CI checks they agree): `birthYear`, `birthMonth`,
  `maxHr`, `restingHr`, each 0 for "not set". Read once when the app opens
  (as the SDK intends). Until the app is on the store, the phone can't edit
  them: write `Apps/HybridXRun/app_config.json` over USB, e.g.
  `{"schema":1,"values":{"birthYear":1986,"birthMonth":5,"maxHr":0,"restingHr":0}}`.
- **Resting HR from the watch**: `HEART_RATE_METRICS_DAILY`, connected while
  the GUI runs, period 60 s (with the default 0 the simulator's sensor never
  reports). Values outside 30-100 are ignored.
- **Each second** (`processTrack`), `feedVo2()` passes RunLVGL's latched
  readings to `Vo2Run`: the arbitrated HR and its trust, GPS speed (with its
  valid and dead-reckoning flags), grade. A reading counts only if it arrived
  in the last 2.5 s (unsigned ms deltas), so a sensor that stops cannot make a
  falsely steady minute.
- **On save** (not discard), before the FIT is closed: the auto max HR is
  raised first (only if no max HR was entered), then the profile is resolved,
  the run estimated, the history updated and saved as `vo2.json` (Streak's
  crash-safe `SafeFile`, copied), and the public
  `../SharedData/HybridX/vo2max.json` written. A `VO2_UPDATE` message carries
  the figures to the GUI (also sent with the initial info). Window counts and
  the result go to the log.
- Nothing is added to the FIT file: no FIT field for it is confirmed.
- **Found in the simulator: RunLVGL's LVGL pool.** Starting a run crashed
  the GUI (out of memory building the track screen): RunLVGL builds the next
  screen before freeing the last. Intervals' fix (P3b.3, from Trail T3.2)
  ported: free first, then build. Peak since: 82%.
- `docs/experiments/sim_run.sh [seconds] [shots] [app_config.json]`: a
  pretend watch, start, run, save, the log's VO2 lines and both files.

### R2 verified
- Watch target compile check: 430,276 B `.uapp`, no warnings in our code.
- Simulator, 90 s run: the history and public files written; the
  simulator's "resting HR" is 120 (its running floor), so the profile says
  "Set resting HR", as it should; auto max HR recorded (158).
- Host tests unchanged (33), green.

## R3: on the screens (10 October 2026)

- **Start screen**: "VO2max 52.2" in small grey under the wheel, once there
  is a figure (nothing before, so a first run looks like RunLVGL).
- **Summary**: a VO2MAX face after HEART RATE (L2 from heart rate): this
  run's figure over "THIS RUN", or the reason there is none ("Need 5 steady
  minutes", "Set your birth year", "Set resting HR", "Check HR settings"; all
  host-tested to fit one line); then "Recent 52.2 (3 runs)". While paused it
  says "After saving": the estimate is made on save.
- **Found in the simulator: the summary did not fit with a fifth face.**
  RunLVGL builds all its summary faces up front (28.5 KB, 82% of LVGL's pool,
  the app's peak); the VO2 face took it over and drawing the map failed.
  Ported Trail's run-screen approach (its T3.2) to the summary: only the face
  on display exists, and paging deletes it and builds the next. The summary
  now takes about 9 KB; the app's peak is 76% (the run screen, unchanged).
  Face changes log `LVGL pool (summary face)`.
- `sim_run.sh` takes `SEED=1` (a three-run history, so the screens have a
  recent figure) and photographs the summary faces.

### R3 verified
- Host tests: 34 (the reason-length test added), green.
- Watch target compile check: 432,128 B `.uapp`, no warnings in our code.
- Simulator: start, run, pause, save, every summary face; no pool errors.
  The simulator's runner and heart rate are too noisy for a steady minute
  (and its "resting HR" is its running floor), so on the simulator the face
  shows a reason, not a figure. The arithmetic is the host tests' job.
- Simulator, 12 minutes (720 s): 12 windows: 5 warm-up, 5 unsteady, 2
  kept, so "Need 5 steady minutes", as expected of its noisy runner. The
  seeded history was kept, the auto max HR recorded (158), no pool errors
  (summary faces 74% peak).

## Review (11 October 2026)

A code review of the branch (the VO2max core, the service wiring, the GUI;
not the verbatim RunLVGL copy), each finding then checked against the code,
plus a review of the method itself.

### Code findings

| # | Finding | Verdict | Change |
|---|---|---|---|
| 1 | Auto max HR only ever rose, from any 5 s of trusted HR including the warm-up: one optical artefact (cadence lock) raised max HR, and so VO2max, on every later run, for good | Real, the most serious | Each saved run's sustained max goes into a 10-run ring; the auto max is the **second-highest** (reached on two runs), so one artefact cannot raise it and old values leave with their runs. Warm-up readings never count |
| 2 | "Need 5 steady minutes" was shown when the steady minutes were there but too easy (under 50% HRR) | Real | New statuses: "Too easy to estimate", "HR above your max" (HR over the max HR in use: a max HR set too low) |
| 3 | Grade had no freshness check: a stalled grade sensor fed its last value into every second | Real | Grade older than 2.5 s counts as unknown (level), as HR and speed already did |
| 4 | A daily resting-HR sample of 0 (after midnight, before today's figure) wiped the known value | Real | Only a plausible figure (30-100) replaces the last; it is also kept in `vo2.json`, for a run that ends before the sensor first reports |
| 5 | HR trust was rounded before the 1-3 test, so 0.6 counted and 3.6 did not match RunLVGL | Real | The core takes the raw float and applies RunLVGL's own test |
| 6 | A late GPS or HR sample can be reused for two ticks; a window counts ticks, not wall seconds | Minor | No change: dropping reused samples would turn ordinary jitter into lost windows, and the tick comes from RunLVGL's monotonic time. Accepted |
| 7 | The summary's page indicator was not updated when a new summary arrived | Real, cosmetic | Fixed |
| 8 | `SafeFile` is a copy of Streak's | Deliberate | Kept: the apps are independent by design (CLAUDE.md); its header names its source |
| 9 | A hand-rolled JSON reader where the SDK has one | Fair | The history now uses the SDK's `JsonStreamReader`/`Writer` (coreJSON), as Streak's state file does. File version 2; version 1 files still read; unknown keys are ignored, so a newer file no longer reads as empty (and then gets overwritten) |
| 10 | The GUI compiled the whole core for one function | Minor | The GUI compiles only `Vo2Text` |

The SDK's reader does no type or range check on integers (`strtoul` on
whatever is there), so every value read from `vo2.json` is read as int64 and
bounded by the core.

### The method

Sound, and as published:
- the ACSM running equation (3.5 + 0.2 v + 0.9 v G; running speeds from
  134 m/min);
- %HRR = %VO2R (Swain and Leutholtz 1997), rearranged as
  VO2max = 3.5 + (VO2 - 3.5) / %HRR;
- Tanaka's 208 - 0.7 x age.
The arithmetic is checked by hand in the host tests (54.4 for the reference
run).

Known biases, not changed (they need Jon's field data before any tuning):
- **Max HR dominates.** 10 bpm too high gives about +7% VO2max. Tanaka's
  formula can be 10 bpm or more out for an individual; the auto max can raise
  it but never lower it. A measured max HR is the single best improvement.
- **HR lag.** In the minute or two after a pace rise or a pause, HR is below
  its steady value for the pace, which overstates VO2max. The 10 bpm range
  test removes the steepest of these minutes and the median dilutes the rest.
  Possible refinement: only count a window whose previous window was steady at
  a similar pace.
- **Cardiac drift.** On long steady runs HR creeps up at the same pace, so
  late windows read low. The median of the whole run sits between.
  Possible refinement: weight or limit to the first part of the run.
- **Overground vs treadmill.** ACSM's equation comes from treadmill running.
  Outdoors, air resistance adds a little cost, more at faster paces, so the
  figure may read slightly low for fast runners.
- **Running economy.** The equation assumes a typical runner; an economical
  runner reads low, an uneconomical one high. No field estimate can see this.
- **Shown value has no age limit.** The latest five runs count however old.
  Possible refinement: ignore runs older than some weeks.
- **Settings changes are not re-scored.** Past runs keep the figure worked
  out with the max and resting HR of the day. After changing them, delete
  `Apps/HybridXRun/vo2.json` over USB to start afresh.

### Where the athlete sees VO2max

- **On the watch, in HybridX Run only**: the start screen ("VO2max 52.2")
  and the summary's VO2MAX page after each run.
- **Not on the phone.** The UNA app syncs the activity's `.fit` file, which
  does not carry VO2max (R2: no field added). `vo2.json` and
  `SharedData/HybridX/vo2max.json` stay on the watch; other watch apps can read
  the shared file, the phone cannot.
- Ways to the phone, for Jon to choose:
  1. a **FIT developer field** on the session (the SDK documents how:
     `Docs/FitFiles-Structure.md`, "Developer Fields on Lap / Session"). The
     file then carries it through the UNA app and on to Strava, but whether
     either shows a developer field is not documented (the manifest's
     `customMeasures` may be how UNA's app displays one; UNA to confirm);
  2. **ask UNA** to sync `SharedData/HybridX/` (or the app's folder) to the
     phone, alongside the activity request already drafted;
  3. a small **Android companion** that reads `vo2.json` over the BLE File
     Transfer Service, as Trail's Route Sender writes routes, and passes it
     to the HybridX platform. Android only.

### Review verified
- Host tests: 43 (9 new or changed), green, also under ASan + UBSan.
- Watch target compile check: 432,792 B `.uapp`, no warnings in our code.
- Simulator, 120 s run from a version 1 history: read and rewritten as
  version 2 with its three runs; the simulator's resting HR of 120 ignored;
  no pool errors.

## R4: VO2max in the FIT file (11 October 2026)

Asked by Jon, to see whether anything downstream shows it. Each saved run's
FIT file now carries VO2max in two ways:
- **Developer fields on the session** (as RunLVGL's battery and HR-source
  fields, SDK `Docs/FitFiles-Structure.md`, "Developer Fields on Lap /
  Session"): field 7 `vo2max` (this run) and field 8 `vo2max_recent` (the
  figure shown on the watch), float32, units "ml/kg/min", so any reader shows
  52.2 with no scale to know. No figure: FIT's invalid value.
- **The FIT profile's own `max_met_data` message** (229) with `vo2_max` (field
  2, uint16, scale 10, mL/kg/min), `update_time`, `sport` running,
  `sub_sport` generic: the recent figure, written before the session, only
  when there is one. Not in the SDK's `FitProfile.hpp`; the numbers are the
  FIT SDK 21.171 profile as generated into fitdecode's `profile.py`, cited in
  `ActivityWriter.cpp`. A tool that knows the profile (rather than developer
  fields) has a chance to label it VO2max.
- Not changed: the `.json` sidecar (its reader is unknown; an extra key is a
  risk to the sync for no known gain) and the manifest's `customMeasures` (it
  reaches the phone only with a store package, never a side-loaded build, and
  how a measure maps to a FIT field is not documented).
- Expectation, from Race (its NOTES 5.9: "The developer fields work perfectly
  and are invisible anyway"): Garmin Connect and Strava do not show arbitrary
  developer fields. The open question is UNA's app.
- `docs/experiments/fit_vo2_report.py <file.fit>`: decodes a file (fitdecode,
  CRC checked) and prints both fields and `max_met_data`.

### R4 verified
- Simulator, seeded history: the file decodes with its CRC; `vo2max` empty
  (no steady 5 minutes on the simulator), `vo2max_recent` 52.2,
  `max_met_data.vo2_max` 52.2. No history: both fields empty, no
  `max_met_data`. Both branches of the writer exercised.
- Watch target compile check: 433,440 B `.uapp`, no warnings in our code;
  host tests 43, green.

### Where to look (Jon)
The phone deletes the `.fit` from the watch when it syncs, so to inspect a
file yourself, copy it off first (USB, before the phone connects).
1. **UNA's app**, after sync: the activity's detail screen. This is the real
   question.
2. **Strava**, via UNA's upload: expected not to show (Race's finding).
3. **Garmin Connect**, uploading the copied `.fit` by hand (Import Data):
   worth one try for `max_met_data`; developer fields are not shown.
4. **The file itself**: `python3 hybridx-run/docs/experiments/fit_vo2_report.py
   <file.fit>` (needs `pip install fitdecode`), or any FIT viewer that lists
   developer fields.

## R5: the VO2max page, settings on the watch, the tuning log (14 October 2026)

Jon's choices (R5 plan): the page shows the overview first, then the runs;
on-watch settings and the tuning log in the same phase; a history reset and a
glance later.

- **Start wheel**: Start, Intervals, **VO2max**, Settings.
- **VO2max page** (`Vo2Screen`): the figure shown, "+0.4 since last run"
  (the shown figure before the latest run, `Vo2History::rollingX10Before`),
  then "Max HR 180 (age)" and "Rest HR 52 (watch)", or what the profile
  still needs. L2 pages to the last 10 runs, five a page, newest first, by
  local date. R1: the profile menu. One page built at a time.
- **VO2 profile menu** (`MenuVo2ProfileScreen`, as RunLVGL's lap alerts):
  birth date, max HR, resting HR (each with the value in use as its tip) and
  a tuning-log toggle. Pickers (`Vo2PickerScreen`): one centred number in
  RunLVGL's picker faces, as the two-column `TwoTonePicker` cannot hold a
  four-digit year; birth date is month then year; "---" is automatic; stepping
  up from automatic starts at the value in use (`Vo2Settings`, host-tested).
  A save goes to the service (`VO2_PROFILE_SAVE`), which writes
  `app_config.json` back with the same `AppConfig` it read, as Race does: no
  more hand-made JSON over USB.
- **Tuning log** (config field `tuningLog`, on by default):
  `Apps/HybridXRun/Logs/vo2_<start>.csv`, named as the run's activity file.
  A `#` context line (start, max and resting HR in use with their sources,
  age), the header, one row a minute, a `#` result line on save; deleted on
  discard. Columns: `window, active_s, good_s, outcome` (kept, warmup, gaps,
  unsteady, grade, overflow), `speed_m_min, spread_pct, grade_pct, hr_avg,
  hr_min, hr_max, cost` (ACSM, ml/kg/min). About 4 KB an hour; `Logs/` is not
  under `Activity/`, so the phone's sync leaves it. Nothing deletes old logs:
  clear the folder over USB now and then.
- `docs/experiments/vo2_log_report.py [--max-hr N] [--rest-hr N] [--rows]
  log.csv`: counts by outcome, each kept window's VO2max the app's way, the
  median; the overrides show what another max HR would have given.
- Found on the way: the speed spread lost a steady pace's variance to
  single-precision rounding (0.1% for a constant pace); now summed as
  deviations from the window's first sample. Found in the simulator: the
  scripts pressed Start before the simulated GPS fix on a slow start;
  `sim_run.sh` now waits for RunLVGL's first-fix buzz.

### R5 verified
- Host tests: 58 (15 new), also under ASan + UBSan; the 43 earlier ones
  unchanged.
- Watch target compile check: 446,672 B `.uapp`, no warnings in our code.
- Simulator: `capture_vo2.sh` walks every new screen (captures in
  `docs/screens/vo2-*.png`); the saves rewrote `app_config.json` (born June
  1985, max HR 182, tuning log off); LVGL pool peak 66%. `sim_run.sh`, 150 s:
  the log has its context, two warm-up rows and the result.
- `vo2_log_report.py` on a hand-made log reproduces the watch's 54.4, and 52.6
  with `--max-hr 185`.

## R6: the VO2max glance (17 October 2026)

Jon asked for a glance like X Weigh-in's, showing the VO2max trend and the
current figure. His choices: the bars are "the average over time" (the shown
figure as it stood after each of the last 10 runs); the glance is called
"VO2max".

- **A second `.uapp`**: `Software/Apps/HybridXRunGlance-CMake`, `APP_TYPE
  Glance`, no icon, development `APP_ID 1EC7F5A0424304CD` (first 16 hex of
  md5("HybridXRunGlance")), "VO2max" on the glances screen, built to
  `Output/Glance/`, versioned from `run-v*` tags. `Software/Libs/Glance`:
  `GlanceView` (reads the shared file, every value range-checked; anything
  odd is "no data"), `GlanceLayout` (pure), `Service` (X Weigh-in's: draw on
  `EVENT_GLANCE_START`, push on the tick, exit on stop). It never writes.
- **What it reads**: `../SharedData/HybridX/vo2max.json`, now version 2:
  version 1's keys plus `beforeX10` and `trend`, the shown figure after each
  kept run, oldest first:
  `{"v":2,"vo2maxX10":523,"runs":5,"utc":1791000000,"beforeX10":519,"trend":[505,512,519,523]}`.
  The app writes it when it opens (new) as well as on save, so an updated
  app gives the glance a version 2 file before the next run. A version 1
  file reads as "No runs yet" until then.
- **The shown value is kept per run**: each run in `vo2.json` now ends with
  the value shown once it was added: `[end, estimate, windows, source, shown]`,
  file version 4 (version 3, from the Run link, added the source; this adds
  the last number). Working the trend out from the kept runs alone was wrong
  for the oldest bars once older runs had dropped out (the first preview
  showed it: the oldest bar was one run's raw figure). Files of every earlier
  version still read; where the number is missing it is worked out from the
  runs read so far: exact for histories under 10 runs, an approximation for
  the oldest four of a full one, and exact for every run saved from now on.
  The 17 October build (CI run 171) wrote a version 2 file with four numbers
  a run, the fourth being the shown value; that reads too.
- **Trail runs counted later** (the Run link, `addSorted`) can land in the
  middle of the history, which changes the figure shown after every run from
  there on: those values are worked out again at the insertion (the same
  approximation for the oldest four of a full history). A run older than all
  ten kept is dropped and changes nothing.
- **Layout** (the watch's 240x60, in the list's middle 172 px): "52.7" in
  SemiBold 25 with a small grey "VO2", under it "+0.4 last run" (the change
  at the latest run; "First run" for the first; 10 or more loses its decimal
  so it fits); on the right, a bar a run, 4 px wide, newest white on the
  right. Compact ("52.7 VO2" / "+0.4 last run") for a smaller area or
  budget, tiny ("52.7 VO2") for a short one. No runs: "VO2max" / "No runs
  yet". "since last run" (the app's page) does not fit the column.
- **Bar scale** (a display choice, mine, for Jon to judge on the watch): the
  bars' own range padded 0.5 either side and at least 4.0 ml/kg/min tall, so
  run-to-run scatter of a few tenths looks steady, not like a cliff.
- **Preview**: `docs/experiments/glance_preview.py build-tests/glance_preview
  docs/screens/vo2-glance-preview.png [--store Resources/Glance/previews]`
  (the simulator cannot run glances; the tool runs the real history, shared
  file, view and layout code on made-up runs, in the SDK's Poppins).
- `Resources/Glance/app-manifest.json` (type glance) and three previews, for a
  store listing later.

### R6 verified
- Host tests: 118 after merging the Run link (76 before it; 18 of those new: trend and shared file, the per-run shown value and
  old files, the glance's view and layout against the real font widths),
  also under ASan + UBSan.
- Watch target compile checks (not installable): the app 447,128 B, the
  glance 28,404 B; no warnings in our code. Simulator builds; a seeded run
  (old three-field `vo2.json`) wrote
  `{"v":2,"vo2maxX10":522,"runs":3,"utc":1791300000,"beforeX10":519,"trend":[512,519,522]}`.
- `docs/screens/vo2-glance-preview.png`: every panel fits the band.
- Not yet seen on a watch: the glance's area (assumed 240x60 and 32
  controls, as Streak's and X Weigh-in's), and whether the small "VO2" sits on
  the figure's baseline (6 px down, from the font heights).

## Gate R: what Jon tests (the field test)

1. **Install.** From the CI run's `watch-apps` artifact, copy
   `HybridXRun_*.uapp` into `Apps/HybridXRun/` on the watch (as Race, README
   "Installing on a watch"). Never a container build. For the glance (R6),
   also copy `HybridXRunGlance_*.uapp` into `Apps/HybridXRunGlance/`; eject,
   unplug and power-cycle the watch. It appears on the glances screen as
   "VO2max". Open HybridX Run once after installing, so the glance has its
   file.
2. **Your numbers, on the watch.** Open HybridX Run, scroll to **VO2max**,
   press R1 for the page, R1 again for the profile: set your birth date, and
   your max HR if you know it from a test (leave it on "---" otherwise).
   Leave resting HR on "---" to use the watch's. The tuning log is on.
3. **Run** with the UNA on one wrist and a Garmin (with a VO2max) on the
   other: three to five outdoor runs of 20+ minutes, mostly steady, on
   fairly flat ground, after five minutes of easy warm-up.
4. **Before the phone syncs**, copy off over USB: `Apps/HybridXRun/Logs/`
   and the run's `.fit` from `Apps/HybridXRun/Activity/<month>/` (the sync
   deletes the `.fit`).
5. **Note** after each: the UNA's "THIS RUN" and "Recent" figures (or the
   reason), Garmin's VO2max, the route (flat or hilly), and whether you used
   a chest strap. Send me those with the logs; I'll tune `Vo2Config.hpp`
   from them (`vo2_log_report.py`).
6. **Check** after one phone sync that `Apps/HybridXRun/vo2.json` and
   `Logs/` are still there, and whether UNA's app shows VO2max (R4).
7. **The glance**: after a saved run, check it shows the page's figure and
   change, and a new bar. Send a photo: the area and the "VO2" baseline are
   still unseen.

Open questions for the field test: does the watch report a daily resting HR
to apps (and when); does the GRADE sensor read sensibly; how often do wrist
HR trust levels drop at tempo pace.

## Sources
- ACSM's Guidelines for Exercise Testing and Prescription, metabolic
  calculations (running equation).
- Swain DP, Leutholtz BC. Heart rate reserve is equivalent to %VO2 reserve,
  not to %VO2max. Med Sci Sports Exerc 1997;29(3):410-414.
- Tanaka H, Monahan KD, Seals DR. Age-predicted maximal heart rate revisited.
  J Am Coll Cardiol 2001;37(1):153-156.

---

## SDK v1.6.0 (14 October 2026)

The SDK released v1.6.0 (structured workouts, GPS speed filter, buzzer pitch, shared `:/Workouts`). Our pin is unchanged (`a7a995a1`); ABI 3 / kernel 1.4.0 at both. What it means for this app, and the plan to move the pin, are in `hybridx-race/docs/NOTES.md`, "Amendment — UNA SDK v1.6.0 reviewed" (5.21-5.24).

**Decision (14 October 2026):** re-sync with UNA's updated Run app (v1.6.0) and add `GpsSpeedFilter`. See `hybridx-race/docs/NOTES.md` 5.25.

## R6a: UNA's RunLVGL (sdk-v1.6.0) merged in (14 October 2026)

Run is a copy of RunLVGL, so v1.6.0's changes (structured workouts, the GPS speed filter, the
56 KB LVGL pool, assorted fixes) were brought in with a **three-way merge**, not by copying
files over: base = RunLVGL at `a7a995a1` (what Run was copied from), theirs = RunLVGL at
`sdk-v1.6.0`, ours = Run. `git merge-file` per file, with the SDK's `Libs/Header` and
`Libs/Sources` mapped to our `Libs/App/Header` and `Libs/App/Sources`.

| Result | Files |
|---|---|
| Untouched by us, taken from UNA | 27 |
| New in v1.6.0 (workout screens, `WorkoutUi`, `WorkoutFaces`, `WorkoutLabel`, ...) | 13 |
| Merged cleanly | 3 |
| Conflicted, resolved by hand | 12 (20 hunks) |

**How the conflicts were resolved.** Every `HybridX Run` block was kept; UNA's structure was
adopted around them.
- **Message IDs collided.** UNA's `WORKOUT_LIST`..`WORKOUT_STEP` took `0x13`-`0x16` and
  `WORKOUT_DETAILS_REQUEST`/`WORKOUT_END` `0x17`-`0x18`, which our `VO2_UPDATE` and
  `VO2_PROFILE_SAVE` already used (`0x13`, `0x14`). Ours moved to `0x19` and `0x1A`. They are only
  ever used by name and never stored, so nothing else changes.
- **Menu.** `Root::Id` is now Start, Workouts, Intervals, VO2max, Settings; the item table and the
  enum agree (checked). `vo2Profile` is still reset with the other children.
- **FIT local message numbers.** `L_MAX_MET` (ours) and `L_MEMO_GLOB` (UNA's workout writer) both
  kept: 13 and 14, under FIT's limit of 15 local message types.
- **Service.** Our speed and grade freshness stamps (`mSpeedLastMs`, `mSpeedSeen`) now sit inside
  UNA's peak-speed logic; the VO2 reset in `trackStart` stays, and UNA's `mLapCarryMs = 0`; UNA
  moved the `mSummary` set-up lower (it sizes the lap reserve for workout steps), so ours was
  dropped as a duplicate.
- Simulator `CMakeLists.txt` takes UNA's `LV_MEM_SIZE` 56 KB with our app ID and name.
  `HybridXRun-CMake` got the same define from the merge.

**Behaviour of ours is unchanged in this commit:** the VO2 windows still read the receiver's raw
speed (`mGpsSpeedMs`). The corrected speed comes in R6b, so any change in results is separable.

### R6a verified
- Host tests: 58 pass (core unchanged).
- Simulator builds against `sdk-v1.6.0` with no warnings in `hybridx-run/` and no conflict
  markers left. `sim_run.sh` (150 s, with a seeded history): the run starts, saves a FIT, the VO2
  lines, `vo2.json`, the SharedData copy and the tuning log are all written as before.
- Screens: the main menu shows Workouts and the recent VO2max; the VO2 page is intact; the
  Workouts list opens (empty in the simulator: "Copy .fit files to the Workouts folder").
- Not yet: the watch build (CI) and its size and RAM; no watch run.

## R6b: VO2max on UNA's corrected speed (14 October 2026)

**The change.** `feedVo2()` now feeds the windows `mSpeedFilter.getInstantSpeed()` (the latest
sample times the filter's scale) with `hasCurrentSample()` as the validity gate, in place of the
receiver's raw `mGpsSpeedMs`. Same 1 Hz cadence and the same freshness and dead-reckoning gates
as before; `getSpeed()` (the 2-tick mean) was not used, so the window's 8% spread test sees the
same kind of per-second values as it did. UNA's filter is ticked earlier in `processTrack()`, so
the VO2 feed reads the current second.

**Why.** UNA measured the receiver's speed over ground against the position track on eight
simultaneous recordings: 0.7% low on an open road race, 5.8% low under tree cover, while the
position track matched an odometer to 1.2%. The ACSM running cost is `3.5 + 0.2 v + ...`, and
v is the term that carries almost all of it, so a speed that reads low by a few percent made the
VO2max low by about as much. The filter measures the correction itself (an exponential average
of position distance over Doppler distance, time constant 180 s) and applies it.

**What to expect.** The same run now reads higher: roughly +1% of VO2max on open road, up to
about +5% under trees: a 4% correction moves 52.5 to 54.4 in the hand-checked case below (about
+2 ml/kg/min). The correction needs about 3 minutes to settle; the VO2 warm-up is 300 s,
so no kept window used a half-converged scale (about 80% settled at the first kept window).

**Tuning log.** A `scale` column is appended to each window line (the mean correction over the
window's good seconds, 1.000 = none; empty logs from before R6b simply lack the column).
`vo2_log_report.py` now also prints what the same windows would have given on the raw speed
(speed / scale), so one log shows the size of the correction. Hand-checked: 200 m/min, scale
1.040, HR 160 (rest 50, max 190) gives 54.4 corrected and 52.5 raw.

### R6b verified
- Host tests: 59 pass (one new: the window log records the mean correction over good seconds
  only; the log format test updated).
- Simulator: builds with no warnings in `hybridx-run/`; a 200 s run writes the log with the
  `scale` column. In the simulator the scale is about 1.000 (its simulated track agrees with its
  simulated speed), so this shows the plumbing, **not the size of the effect**.
- Not verified: the effect on a real run, and against Jon's Garmin. That needs a field run on the
  CI build: compare the report's corrected and raw figures and the Garmin's.

### R6 CI (run 138 on `fe0fa67`: green)
Both jobs passed: the watch build (every app, ST's toolchain) and the host tests, including Run's
59. The `watch-apps` artifact (a zip of all the `.uapp` files) is 1,810,144 bytes against
1,781,923 on the previous run (136), **+28,221 bytes**. Run is the only app whose code changed
between the two, so that growth is Run's: the structured-workout engine, screens and FIT reader
UNA added. Run's own `.uapp` size was not read separately (the log's storage host is blocked from
the container); earlier figures were 419,996 B at R0 and 430,276 B at R2. The app linked with the
56 KB LVGL pool, so RAM fits. Still no run on a watch.

## L1: windows built once, judged per window, and a shared file format (15 October 2026)

First build phase of the Run and Trail link (`VO2_LINK_PLAN.md`, approved the
same day). Pure core only: no service, GUI or Trail change.

- **`WindowBuilder`** (`Vo2Window.*`, `Vo2WindowConfig.hpp`): the per-second
  accumulation and the auto max HR, moved out of `Vo2Run`. It makes a
  `WindowRecord` of integers per minute and judges nothing. **These two files
  and `Vo2WindowFile.*` are the ones Trail will copy byte for byte** (L4, with a
  CI identity check). Config the judge alone uses stays in `Vo2Config.hpp`, so
  Run's tuning never touches Trail.
- **`Vo2Run`** keeps the same public API (the service is unchanged) and now
  judges records; **`addWindow()`** judges a window built elsewhere, by the same
  rules, and `noteSustainedMax()` takes that run's max HR.
- **Two rule changes**, from Jon's 14 October log: the HR range limit is 12 bpm
  (was 10: eight windows missed it by 1-3 bpm), and a window is "too steep" when
  its **mean** grade is outside -3% to +10% or more than 10 of its seconds were
  (was: any one second). The builder counts the steep seconds; a window
  record carries the count.
- **`Vo2WindowFile.*`**: one run as JSON, `{"v":1,"id":<start utc>,"max":<HR>,
  "w":[[activeSec,goodSec,speed,spread,grade+3000,steep,hr,hrMin,hrMax],...]}`,
  at most 120 windows (the first ones), 8 KB buffer. Every value bounded on
  read; malformed, empty or newer files read as nothing. Windows are read by
  index (the SDK reader searches from the start each time).
- **Found: the SDK's JSON writer prints signed 32-bit values with `%ld`**
  (`JsonStreamWriter.cpp:526`). That is right where `int32_t` is a `long` (the
  watch's compiler) and wrong where it is an `int` (the host: a -27 came out as
  4294967269). So the file holds no negative number: grade is stored offset by
  3000. The SDK is read-only here; nothing else in Run writes a negative.
- **A spike still moves the figure a little**: one +15% second per minute is
  0.25% of mean grade, adding ACSM cost (55.0 not 54.4 on the test run). It is
  no longer rejected, which is the point; whether to drop such seconds from the
  mean is for the field data (the log has only window means).

### L1 verified
- Host tests: 77 (18 new: the builder, the judge on records, the file codec,
  and Jon's run as a fixture), green, also under ASan + UBSan.
- **Jon's 14 October run replayed** (47 windows, max HR 182, resting 41): old
  rules 22 windows, 45.2; L1 rules 34 windows, 45.1 (the test also checks the
  `vo2_log_whatif.py` numbers independently).
- Watch target compile check (Ubuntu's arm-none-eabi-gcc 13.2 with Race's
  syscall stubs, so **not for a watch**; the watch copy comes from CI):
  `HybridXRun_0.0.0-dev.uapp` 487,824 B, no warnings outside SDK files. Needed
  `pip install pyelftools` for the SDK's packer.
- Simulator builds; a 120 s run (`SEED=1 sim_run.sh 120`) saves the FIT,
  `vo2.json`, the shared copy and the tuning log as before.
- Not done, by design: nothing reads or writes the window file yet (L2, L4).

## L2: Run counts the runs Trail left behind (15 October 2026)

Second build phase of the link (`VO2_LINK_PLAN.md`). Run now reads Trail's
window files; nothing writes them yet (Trail is L4), so on a real watch this
does nothing until then. Tested with synthetic files.

- **Where and when.** At start-up (`loadVo2`, once: the apps never run
  together), `Service::ingestTrailRuns()` reads the three slot files
  `../SharedData/HybridX/trail_vo2_0.json` to `_2.json` (`kWindowFilePaths`), by
  name, never by listing the folder. Each is read from its primary, else its
  `.bak`, as Run's own files are. An 8 KB buffer is allocated for the
  duration (the service stack is 10 KB) and freed.
- **What is counted.** Runs newer than the history's **Trail mark** (the newest
  Trail run ID already handled), oldest first; the mark moves as each is
  handled. So a run is never counted twice even if it has rolled out of the
  ten-run ring, and a copy in two slots counts once. The mark also moves past a
  **corrupt** file (a good header but a window out of range), so it is not
  retried for ever, and past a run with too few steady minutes (counted as
  nothing).
- **Judged by Run.** Each window goes through `Vo2Run::addWindow()`: the same
  rules and the same profile as a run recorded here. Its sustained max HR joins
  the run maxima (so Trail runs help the auto max HR, which needs two runs). The
  run joins the history **in time order** (`addSorted`), tagged as from Trail.
- **No profile, no counting.** If the profile is not complete (no birth year, no
  resting HR), the files are left for another day and the mark does not move.
- **`vo2.json` is version 3**: each run's source (0 Run, 1 Trail) and the
  Trail mark. Version 1 and 2 files still read (sources all Run, mark 0).
  `vo2max.json` (the public copy) is rewritten after an ingest too.
- Core: `Vo2TrailIngest.*` (judge one file, pick the next slot), `Vo2History`
  (source, mark, `addSorted`). Host tests: **90** (13 new).
- **Not shown yet:** the screens do not say how many runs came from Trail
  (L3), and the log line (`Trail run <id>: ...`) is the only trace.

### L2 verified
- Host tests: 90, also under ASan + UBSan.
- Simulator (`docs/experiments/sim_trail_ingest.sh`, three starts):
  1. two new Trail runs (12 and 8 windows) and a junk file: both counted, in
     time order between Run's own, history 5 runs, auto max HR 176 from two
     runs, `vo2.json` v3 with sources and mark. The figures match the arithmetic
     by hand (200 m/min, HR 160, rest 50, Tanaka 180: 50.8; 190 m/min, HR 158:
     49.2);
  2. the same files: nothing counted again, files unchanged;
  3. a newer run with 3 windows: the mark moves, no estimate, history unchanged.
- Watch target compile check (not for a watch): 490,260 B, no warnings
  outside SDK files. Simulator builds.
- Not yet verified: a real Trail file from a watch (L4), and the time Run takes
  to read three files on the watch (the SDK reader searches from the start for
  each value: about 120 windows x 9 values per run).

## L4 (Run's part): the average grade-adjusted pace, and the slot rule (15 October 2026)

L4 is mostly Trail's (`hybridx-trail/docs/NOTES.md`, "Run link"); two things
changed here, both in the files Trail copies, so Trail's copies were refreshed
(`hybridx-trail/Utilities/check-vo2-link-copies.sh --sync`).

- **Average grade-adjusted speed** (`WindowBuilder::avgGradeAdjustedSpeedMs()`,
  `Vo2Run` passes it through). Over every second moving at 0.5 m/s or more with
  a valid speed (heart rate not needed), the speed as if level, using ACSM's own
  uphill gain: `v x (1 + 4.5 x grade)`, grade clamped to 0..20%, downhill
  counted as level. Uphill only, until a downhill model exists (Minetti was
  deferred). Pace is `1 / speed`. The service logs it at save
  (`Average grade-adjusted pace: N s/km over S s moving`); no screen yet.
- **`windowFileSlotToWrite()`**: Trail's three slot files; an empty one first,
  else the oldest run's.
- Host tests: **96** (6 new), also under ASan + UBSan. Watch target compile
  check 490,732 B (not for a watch), simulator builds.

## Activity link: Run tells Streak about each saved run (27 October 2026)

Phase 2 of the HybridX activity link (`hybridx-link/docs/ACTIVITY_LINK.md`).
When a run is saved, Run adds it to `../SharedData/HybridX/act_run.json`, the
file HybridX Streak reads. Run is its only writer. The Streak side is phase 3,
so nothing reads it yet.

- **Where**: `Service::writeActivityLink`, called inside the success branch of
  `if (mActivityWriter.stop(fitTrack))` in `stopTrack`, after
  `notifyNewActivity()`. So it happens only for a run whose `.fit` closed
  properly: not on discard, not for an empty session, not when the `.fit` fails
  to close. Run's `vo2max.json` is written earlier than the `.fit`; this is not.
- **The start** is the time `startTrack` was given, kept in the new `mStartUtc`
  (there was no member for it). The activity file is named from the same time, so
  the file's local start and the summary's agree. `fitTrack.timeStart` is worked
  out from the counters and could differ by a second.
- **The entry**: start UTC; the zone offset (from `mTimeTracker.getLocalTime`,
  turned into minutes by `HybridXLink::tzOffsetMinutes`); the active time as
  duration; the distance; the average and maximum heart rate; sport Run.
  `HybridXLink::makeEntry` rounds and bounds the float totals (NaN or negative
  become 0). An entry that fails `valid()` (a start before 2020, so an unset
  clock) is not written; the log says why.
- **The file**: the old file is read with `SafeFile` (the primary if it decodes,
  else its backup), `HybridXLink::append` adds the run and keeps the newest 16,
  and `SafeFile::write` saves it. A failure is a warning and never affects the
  save. Two 1 KB buffers come from the heap (`new (std::nothrow)`) rather than
  the 10 KB service stack, as Trail's ingest does; the library itself uses about
  400 bytes of stack.
- **Code**: `Software/Libs/Link/` is a byte-identical copy of `hybridx-link`'s
  library (change that, then `hybridx-link/Utilities/check-link-copies.sh
  --sync`; CI checks). `libs.cmake` adds it to the service. The GUI and the
  glance do not use it.
- **Verified**: Run's host tests are unchanged, 118, green. The library's own
  are 29 (`hybridx-link`), also under ASan + UBSan. Simulator, with
  `docs/experiments/sim_run.sh` (which now lists `act_run.json`): a 40 s run
  wrote `{"v":1,"app":"HybridXRun","a":[[1793099301,1440,41,215,144,160,1]]}`,
  its start the same as the `.fit`'s name and the tuning log's; a second run
  added an entry, oldest first, with a `.bak` kept; a discarded run left the
  file byte for byte as it was and wrote no `.fit`. The container's clock is
  UTC, so the zone is 0 (1440 stored); a non-zero zone is covered by the library
  tests only. Watch target compile check 494,012 B (not for a watch); glance
  28,404 B; simulator builds, no new warnings.
- **Not checked**: anything on a watch. Whether the summary's start matches the
  `.fit` file's name on a watch is the same code path as the simulator's, but
  the zone offset is not tried with a real zone until Jon's test.

## P1: cadence and step length, live and saved (27 October 2026)

First phase of the plan for cadence and step length, a run detail page and VO2max
reliability (the detail page is P2; the VO2max work, which changes nothing in the
estimate until Jon has compared with his Garmin, is P3). Cadence and step length were
already in every FIT **record**; P1 puts them on the screen while running, averages them,
and writes the averages to the FIT lap and session.

- **`FormStats`** (`Core`, pure): fed once per active second; keeps the run's and each lap's
  average cadence (mean over seconds with a valid cadence) and step length (cadence-weighted:
  sum(step x cadence) / sum(cadence), which is distance / steps, so it is right when the
  cadence changes). Integers: cadence in tenths of a step per minute, step length in mm; 0 is
  no data. **`FormText`** (`Core`, header-only because the GUI links no Libs sources): "172",
  "1.08", "---".
- **`Service::updateForm()`**, once per track tick, before the record is made: a cadence is
  used only if the sensor has reported in the last 2.5 s (`kFormFreshMs`, the VO2max inputs'
  limit; the sensor has no timestamp, so arrival is stamped in the callback) and is valid;
  step length is `StrideMath::impliedStepLengthM` from the corrected GPS speed and that
  cadence (the SDK's 0.15-2.50 m gate). The screen, the FIT record and the averages all take
  these same values. **A behaviour change for the FIT record**: before, the record's cadence
  flag was just the sensor's valid bit, so a stalled sensor held its last value in every
  record; now the record carries none after 2.5 s.
- **`Track::Data`** gained `cadenceSpm`, `stepLengthMm`, `cadenceAvgSpm`, `stepLengthAvgMm`
  (integers, 0 = none). `TrackDataUpd` is 176 bytes (136 of data) on the host; a
  `static_assert` now keeps it inside the kernel's 256-byte message block.
- **FIT**: the lap and the session each gained three native fields, after the existing ones and
  before the developer fields (the local-type count is unchanged, still 15 of 16). The numbers
  are Garmin's profile as fitdecode 0.11.0 generates it, read from its `profile.py`, not from
  memory: **lap** `avg_cadence` 17 (uint8), `avg_fractional_cadence` 80 (uint8, scale 128),
  `avg_step_length` 120 (uint16, scale 10, mm); **session** 18, 92 and 134 (same types). They
  are encoded as the record's are (`SDK::FitRecordCadence`: strides per minute split into a
  whole and a fractional byte; step length as mm x 10) and are FIT's invalid value when a lap
  or session had no cadence. **Found**: fitdecode decodes session field 18 as
  `avg_running_cadence` (strides per minute, a subfield the profile selects when the sport is
  running; the same field number), so the value 85 is 170 steps per minute.
- **Screens**: a **FORM** face on the live screen (cadence large, then step length, then the
  run's averages as "170 spm / 1.71 m"), after the lap face in L2 order in every mode; and a
  **FORM** summary face (average cadence and step length) after VO2MAX. It is a new
  `TrackView::ID_FORM` appended to the enum, so no saved face id changes. "---" while there
  is no valid cadence. Step length is in metres whatever the pace unit (it matches the FIT).
  `Activity/summary.json` and the `activity_*.json` sidecar are **not** changed (R4's decision
  stands); the summary's averages live in memory for the screen only.
- **Memory**: the live screen builds all its faces up front, so the Form face is resident for
  the whole run. Measured in the simulator (pool 56 KB): the Track screen went from 32,816 B
  used (peak 68%) to 36,232 B (peak 76-78%), about 3.4 KB. That is under the 80% line set in
  the plan for building it on demand instead, but it leaves about 12 KB. **If P2 pushes the
  peak past 80%, build the Form face when shown** (as the summary does). The summary Form face
  is built on demand like the others (about 9.4 KB while shown).
- **Simulator**: `IMU_RUNNING_CADENCE_SIM_ENABLE 1` in Run's `ConfigurationSimulator.hpp` (the
  SDK's mock: a constant 170 steps per minute). Its step length is 1.5-2.4 m at the GPS
  simulator's 15-25 km/h, beside the 2.50 m gate: it checks the plumbing, not real strides.
  `sim_run.sh` now photographs the Form faces and prints the pool lines.
- **`docs/experiments/fit_form_report.py <file.fit>`** (needs `pip install fitdecode`): decodes
  a saved file and checks that every lap's and the session's average cadence and step length
  equal what its own records say (cadence within 0.6 spm, step length within 3 mm), and that a
  lap or session without cadence carries none. A lap ends at the second the next starts, and
  the record written at that second belongs to the lap that was running (the checker's first
  version counted it twice and reported 10-14 mm errors that were its own).
- **Verified**: host tests 131 (13 new: `FormStatsTest`, `FormText`), also under ASan + UBSan.
  Simulator: a 3-lap run with the laps' step lengths all different (1999, 1601 and 1811 mm):
  every lap and the session match their records exactly, and the lap record counts add up to
  the session's (13 + 18 + 20 = 51). With the cadence mock off: the screens read "---", the
  records carry no cadence and the lap and session averages are absent. The screens were read
  as round-face screenshots, which caught a clipped "CADENCE" label (fixed). Watch target
  compile check 496,284 B (+2.3 KB; not for a watch), glance unchanged, Trail's copy check and
  the link copy check pass.
- **Not checked**: anything on a watch; how well the wrist's cadence tracks real steps (Run's
  notes already record optical artefacts, "cadence lock"); that Garmin Connect or Strava show
  the lap and session averages (Race's findings suggest they build their series from the
  records; the standard fields are the likeliest to be used, but that is not tested).
