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
| Auto max hold | 5 s | chosen |
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

## Sources
- ACSM's Guidelines for Exercise Testing and Prescription, metabolic
  calculations (running equation).
- Swain DP, Leutholtz BC. Heart rate reserve is equivalent to %VO2 reserve,
  not to %VO2max. Med Sci Sports Exerc 1997;29(3):410-414.
- Tanaka H, Monahan KD, Seals DR. Age-predicted maximal heart rate revisited.
  J Am Coll Cardiol 2001;37(1):153-156.
