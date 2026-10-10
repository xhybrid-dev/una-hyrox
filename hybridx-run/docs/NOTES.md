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

## Sources
- ACSM's Guidelines for Exercise Testing and Prescription, metabolic
  calculations (running equation).
- Swain DP, Leutholtz BC. Heart rate reserve is equivalent to %VO2 reserve,
  not to %VO2max. Med Sci Sports Exerc 1997;29(3):410-414.
- Tanaka H, Monahan KD, Seals DR. Age-predicted maximal heart rate revisited.
  J Am Coll Cardiol 2001;37(1):153-156.
