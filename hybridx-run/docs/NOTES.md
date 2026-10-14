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

## Gate R: what Jon tests (the field test)

1. **Install.** From the CI run's `watch-apps` artifact, copy
   `HybridXRun_*.uapp` into `Apps/HybridXRun/` on the watch (as Race, README
   "Installing on a watch"). Never a container build.
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
