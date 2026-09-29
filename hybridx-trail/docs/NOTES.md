# HybridX Trail: Notes

The running log for the breadcrumb-navigation app: SDK findings, decisions and
open questions. Same conventions as the other apps' NOTES: evidence first,
file:line citations into `una-sdk/` at the pinned commit (`a7a995a1`), nothing
claimed that wasn't read.

---

## Why this project exists, and why it's separate

Jon coaches runners and trail runners as well as HYROX athletes, and wants
breadcrumb navigation like the older and entry-level Garmins: load a GPX, follow
the line. It is its own app, like Streak and Intervals, not a Race feature.

## Research pass: what the platform can and can't do (26 September 2026)

### No navigation in the SDK, but the parts are there

Nothing in the SDK imports a route, follows one or alerts off course. What it
has:

- **GPS position:** `GPS_LOCATION` (0x110), precision, valid flag, lat, lon,
  alt as `float` (`Docs/SensorsLayer.md:41`, parser
  `SensorDataParserGpsLocation.hpp`). RunLVGL connects it at period 1000,
  latency 1000 (`Examples/Apps/RunLVGL/Software/Libs/Header/Service.hpp:39-40`).
- **Compass:** `MAGNETIC_FIELD` (0x30) with a `MAG_CALIBRATED` flag, and a
  level bearing (`getAzimuthDeg`) and a tilt-compensated one
  (`getAzimuthDegTilted`, which takes an accelerometer sample)
  (`Docs/SensorsLayer.md:140-212`). **Magnetic north only: "No declination is
  applied"** (`SensorDataParserMagneticField.hpp:40`).
- **Track drawing:** `SDK::TrackMapBuilder` fits a whole track to a size and
  can rotate it (`Libs/Header/SDK/TrackMap/TrackMapBuilder.hpp`). Its screen
  points are `uint8_t`, and it uses `std::vector`. RunLVGL's LVGL GUI has a
  `Widgets::Map` that draws one as an `lv_line`
  (`RunLVGL/Software/Apps/LVGL-GUI/gui/src/widgets/Widgets.cpp:260-298`). Good
  for a whole-route overview; the zoomed, centred-on-you view needs our own
  projection (`MapView`, brief section 5).
- **Files:** `IFileSystem` is sandbox-rooted per app, `"/"` being the app's own
  folder (`Docs/app-config-fields.md:27`), with directory listing, open, read,
  seek (`Libs/Header/SDK/Interfaces/IFileSystem.hpp`).

### USB delivery into an app's folder is a documented, supported path

- Apps are installed by copying over USB mass storage into `Apps/<AppName>/`
  (`Docs/deploy.md:5-11`).
- The SDK says an app's folder is "readable over USB mass storage and over BLE"
  (`Docs/app-config-fields.md:43-44`), that a values file "can also arrive from
  a hand edit over USB" (`:325`) and that "a user can delete it over USB"
  (`:498`).
- This repo already relies on it: Streak's and Intervals' probes read their
  logs back over USB (`hybridx-streak/docs/PROBE.md`).

Not documented: **whether an app sees a file copied in over USB without a power
cycle.** A power cycle is documented after installing an app, not after adding
a data file. The probe asks (PROBE.md, step 4).

### What the platform can't do

- **No map tiles or street data:** no renderer, and a route line plus a marker
  is all the memory and display suit.
- **No turn-by-turn:** that needs cue points worked out off the watch. Possible
  later if a converter creates them; out of scope.
- **Phone delivery of a GPX:** UNA's phone app writes only an app's `configFile`
  (`Docs/app-config-json.md:220`); its settings fields are scalars, at most 32
  of them (`Docs/app-config-fields.md`, section 8). That's one waypoint, not a
  route. The BLE File Transfer Service can write any file
  (`Docs/BLE-File-Transfer-Service.md`), but needs a bonded, encrypted link and
  a sender we'd have to build: HybridX Intervals' probe is testing exactly that
  transport. Draft request to UNA: `UNA_GPX_REQUEST.md`.

### Sensor connection period: the docs disagree with RunLVGL

`SensorConnection.hpp:45` says the period's unit is "defined by the driver".
`Docs/SensorsLayer.md:111,163` connects the accelerometer and magnetometer with
`0.1f`; RunLVGL connects GPS at `1000` and its fusion sensor at
`1000.0f / skFusionSampleRateHz` (`RunLVGL/.../Service.cpp:75`), which reads as
milliseconds. The probe follows RunLVGL: GPS 1000, compass and accelerometer 200
(5 Hz). If the compass sample count in `probe.txt` is far from ~5 per second,
the unit is something else. **Not logged as an SDK conflict yet**: it's an
ambiguity, and the probe will say which reading is right.

## Decision: GPX read on the watch, as exported

Recommended to Jon and taken for T0: the watch reads the GPX file exactly as
exported, with no laptop tool. The alternative, converting to a compact format
on a PC first, adds a step athletes would get wrong. It costs a streaming
parser (`GpxReader`), which is small and fully host-tested.

## Decision: coordinates as integer degrees x 10^7

A float holds about 7 significant digits: at latitude 54 its step is about
0.4 m, and near longitude 180 up to 1.7 m, so subtracting two floats a few
metres apart loses much of the difference.
The core stores points as `int32` degrees x 10^7 (`GeoPoint.hpp`), takes
differences in integers, and only then uses float. 8 bytes a point. The SDK's
`float` degrees are converted once, as each GPS sample arrives.

## Decision: route size is fixed; long routes are thinned evenly

`RouteBuilder` keeps points at least 10 m apart and, if its array fills,
doubles the spacing and thins what it has in place, so any file fits. Length
and ascent come from every point in the file, so thinning never changes them.
The probe uses 2,000 points (16 KB); the real figure is set at Gate T0 from the
memory measurement.

## T0: the probe (built 26 September 2026)

- **What:** `Tools/Probe`, `HXTrailProbe` (`APP_ID 0BFC65580EBE7A2B`, a
  development ID: the first 16 hex digits of md5("HXTrailProbe")). Steps for Jon:
  `PROBE.md`.
- **Built from:** HybridX Intervals' probe: the same service/GUI shape, fonts,
  simulator and `FlatFileSystem` test fake. The route check uses the real app's
  core (`Software/Libs/Core`), so T0 tests the code T1 builds on.
- **Verified here:**
  - 37 host tests pass (`Tests/Host`): GPX parsing at chunk sizes from 1 to
    512 bytes, single quotes, CRLF, namespaces, comments, CDATA, entities, bad
    and out-of-range points, long tags; thinning, length and ascent; distance
    to a route; the folder check, including macOS's `._` files.
  - The watch target builds (local ARM compiler with syscall stubs: a compile
    check only; the `.uapp` for the watch comes from CI). Service image:
    `.text` 18,200 B, `.bss` 25,944 B (of which 16,000 is the route),
    `.stack` 10,240 B.
  - The simulator reads the 1.2 MB, 5,001-point loop from `make_test_gpx.py`
    in 16 ms on a PC: 10,053 m, 1,001 points kept at 10 m spacing, 80 m
    ascent, all as expected. The simulator has no magnetometer (connect fails,
    as the screen shows) and its GPS circles a stadium elsewhere, so its
    off-route figure is thousands of km.
- **Not verifiable without a watch:** everything Gate T0 asks.

## Gate T0: run 1, on Jon's watch (28 September 2026)

`AR_Ham2Lyme_50k_26.gpx` (an Anquet/UKC-style trail race route, 50 km, in
Dorset/Devon), copied into `Apps/HXTrailProbe/Routes/` and opened once, no
separate install-then-cycle step recorded: **GO on the first ever run.**
Evidence: `probe.txt`, `probe-history.txt`, one photo.

| Check | Result |
|---|---|
| GPX read | 128,857 bytes, 1,418 `trkpt`, 0 bad, 1 long tag (an extra attribute past 256 B, harmless), in **334 ms** |
| Route built | 1,190 of 1,418 points kept **at the starting 10 m spacing** — thinning barely engaged; 49,504 m, 845 m ascent |
| Memory | largest single allocation **510 KB** |
| GPS | first fix at **27 s**, precision reported 0.7-1.4 m (tight; the sensor's own claim, not independently checked) |
| Compass | **never calibrated**: 807 samples over 129 s, 0 with `MAG_CALIBRATED` set. Bearing and tilted bearing stayed `-1` throughout |
| Off route | steady ~24.3-24.4 km — the watch was nowhere near the Dorset route, so this is the maths working correctly, not a fault |

**Reading:**

- **USB delivery works, and fast.** No power cycle appears to have been needed
  to see a GPX dropped straight into a freshly created `Routes/` — worth
  Jon confirming explicitly next time, but nothing in the run history suggests
  otherwise (a single run, straight to GO).
- **A real 50 km ultra route is only 1,418 raw points** (one every ~35 m) —
  nowhere near the 2,000-point/16 KB cap set in the probe. `RouteBuilder`'s
  thinning stayed at its starting 10 m spacing the whole way. This suggests
  planner-exported GPX (as opposed to a densely recorded track) is naturally
  sparse, and 2,000 points has real headroom for longer routes too.
- **334 ms to parse and thin 126 KB is well inside "instant"** — no
  `RouteCache` needed at T1 (brief §5) unless a much denser file is tried.
- **510 KB free is far more than the ~16 KB route needs** — memory is not a
  constraint at this route size.
- **The compass never calibrated in over two minutes of normal outdoor use.**
  This is the one real open question for heading-up (brief F5): either the
  watch needs an explicit calibration gesture the SDK doesn't document (a
  figure-of-eight motion is the common pattern on other platforms), or
  calibration takes longer than tested, or GPS course-over-ground has to be
  the primary heading-up source with the compass as a fallback once/if it
  calibrates. Needs a UNA question (brief §8) and a longer/gestured retest.

## Gate T0: **GO.** T1 can start.

Decisions from this run:
- Route point cap: keep 2,000 for now — real routes use far fewer.
- `RouteCache`: not needed yet; revisit only if a denser file is slow.
- Heading-up source: default to **GPS course over ground**, not the compass,
  until calibration is understood. Compass can enhance it later if calibrated.

## Open

- [ ] Confirm with Jon: was a power cycle needed at any point to see the GPX?
- [ ] Try a figure-of-eight motion with the watch, then re-run the probe, to
      see if that calibrates the compass.
- [ ] Ask UNA how compass calibration is meant to be triggered (brief §8).

## T1: the route core (28 September 2026)

Pure C++ in `Software/Libs/Core`, no kernel, all host-tested (73 tests in
`Tests/Host`, synthetic routes and runs from `support/RunSim.hpp`).

| Part | Job |
|---|---|
| `RouteTracker` | Matches each fix to the route: distance done, remaining, off-route distance, finish |
| `OffCourse` | When to buzz: the alert state machine |
| `CourseOverGround` | Direction of travel from GPS, for the heading-up map |
| `MapView` | The route as clipped screen lines around the runner, at a zoom and rotation; whole-route fit |
| `GeoPoint` | gained `bearingDeg`, `offsetM`, `projectOntoSegmentM` |

### T1.1 How the tracker avoids jumping legs

Matching a fix to the nearest point anywhere fails on exactly the routes trail
runners use: a loop's start and finish are the same place, an out-and-back
runs one path twice, a figure-of-eight crosses itself. The tracker searches a
window around its last match (150 m back, 600 m ahead) and scores each
candidate as *metres off the route + 0.2 x metres from where the runner is
expected to be along it*, where "expected" is the last match plus the recent
progress per fix, and being behind that costs double. At the first lock the
along part is simply the distance from the start, so a loop starts at its
start. It leaves the window only when the runner is clearly (30 m) nearer
another part of the route: a shortcut, or a wrong turn that rejoins.

**Tried and dropped:** using the GPS direction of travel to tell the two legs
of an out-and-back apart. With +-8 m of noise per fix at 3 m/s, a heading from
10 m of movement swung between 29, 85, 239 and 343 degrees on a straight run,
and put the runner on the return leg 600 m ahead. Expected progress is far
steadier. `CourseOverGround` stays, for the map only.

**Known and accepted:** with noise, a fix inside a sharp corner projects back
onto the incoming segment by up to ~12 m (geometry, not a wrong leg). Tests
check progress stays within 20 m of the true distance on noisy figure-of-eights
and parallel out-and-backs.

### T1.2 The alert, as a runner would see it (for Jon to agree, Gate T1)

Each line is a host test (`OffCourseTest.cpp`):

1. **Walking to the start**, 800 m away for ten minutes: **no buzz**. Nothing
   happens until you first reach the route.
2. **A wrong turn:** 5 seconds more than **50 m** from the line, **one buzz**
   ("Off course"). While you stay off, **a reminder every minute**. Once back
   within **30 m** for 3 seconds, **a different buzz** ("Back on course").
3. **A GPS spike under trees**, 90 m off for 4 seconds, twice: **no buzz**.
4. **A switchback that doesn't match the GPX**, wobbling 35-48 m off: **no
   buzz**, ever. And once genuinely off, wobbling 35-48 m doesn't clear it:
   you have to be within 30 m.
5. **Bad fixes** (the GPS itself says +-40 m): ignored completely, and a single
   bad fix restarts the 5-second count.
6. **After the finish:** **one "Finished" buzz**, then nothing, whatever you
   do next (walking to the car park).
7. The whole chain on a real-shaped wrong turn (miss a turn at 300 m, 650 m
   detour, rejoin at 700 m): off, three reminders, back on, finished.

The numbers (50 m, 30 m, 5 s, 3 s, 60 s, 25 m) are one `Config` struct:
changing any is one line. **Jon to confirm or change them.**

### T1.3 Map

`MapView` projects the route around the runner with zoom levels of 100 m,
250 m, 500 m, 1 km and 2.5 km from the runner to the edge of the round screen,
north-up or turned to the heading. Segments are clipped to the screen, a route
that leaves and re-enters becomes separate lines, and points closer than 2 px
are dropped. At most 512 points in 16 lines per frame (2 KB, fixed); a busier
view is cut short and flagged, never overflowed. `fit()` frames the whole
route inside the circle, north-up, for the route preview.

### Gate T1

- [x] Host tests pass: 73 (T0's 37 plus 36 new).
- [ ] Jon agrees the alert behaviour (T1.2), or gives new numbers.

## T2: the service (28 September 2026)

### T2.1 Started from RunLVGL, unchanged first

`Software/` is a verbatim copy of the SDK's RunLVGL (commit `a7a995a1`),
committed on its own, so every Trail change is a readable diff against UNA's
original: the LVGL GUI in `Apps/LVGL-GUI`, the service in `Libs/App`. Trail is
an `Activity` app (it records runs, as RunLVGL does), `HybridXTrail`, APP_ID
`71ABD15ED7526601` (development: the first 16 hex of md5("HybridXTrail")),
versioned from `trail-v*` tags (`Software/cmake/trail-version.cmake`, as
Streak's). The GUI also compiles the route core, for the map.

### T2.2 Navigator

All the route work sits in `Core/Navigator` (pure over `IFileSystem`, 9 host
tests), so the service diff stays small:

- **`scan()`** lists `Routes/` and summarises each GPX (name, length, climb).
  Summaries are cached in `routes.idx`, so an unchanged file is never parsed
  twice: a second app start parses nothing. Sorted by name.
- **`load()`** reads one route into 2,000 fixed points and remembers the
  choice in `route.sel`; **`restoreSelection()`** brings it back next time.
- **`update()`** runs the tracker, the heading and the alert for one fix. The
  alert only runs while an activity runs: frozen on the start screen and while
  paused.

26 KB in all, so it lives in static storage (`Service.cpp`), not in the
Service object on the 10 KB service stack.

### T2.3 The service's changes

- On start: `scan()`, `restoreSelection()`.
- Every second: a new GPS fix (by its timestamp) goes to `Navigator::update`
  with the GPS's precision; an alert buzzes and is sent to the GUI; a
  `NavUpdate` goes to the GUI.
- Alerts: **off course**: backlight, 3 x 300 ms beeps, 2 x 750 ms vibration;
  **reminder**: one of each; **back on**: two short beeps and a double click
  (short and different: good news); **finished**: as RunLVGL's lap end, with
  a 1 s vibration.
- A new activity resets progress to the start of the route.
- Messages 0x20-0x25 (`Commands.hpp`): the route list and the route travel as
  pointers into the service's static storage, as RunLVGL's own Summary does
  (no MMU: the app's two processes share memory); the GUI copies them. The
  route is only changed on the start screen, never during an activity.

### T2.4 Found in the simulator: a runner drifting off "finished"

`Tools/TestRoutes/make_sim_routes.py` draws routes on the SDK simulator's own
400 m stadium track. On `sim-wrong-turn.gpx` (an out-and-back 10 m wide that
the lapping runner leaves at the first bend) the first run logged "went off"
and then, 24 s later, **"finished" 57 m from the route**. Reproduced on the
host (`RouteTrackerTest.DriftingAwayNearAReturnLegNeverJumpsToIt`): drifting
56 m away, the runner was matched to the return leg 545 m ahead (within the
60 m reach, and nothing nearer), and later to the finish.

Fixed in `RouteTracker`: a match far along the route (more than 30 m back, or
further ahead than 7 m/s since the last match) needs the runner within 20 m of
the route there; and the finish needs the runner within 30 m of it. A real
shortcut or rejoin still works: the runner is on the route when they rejoin it.

### T2.5 Verified

- 85 host tests pass.
- Watch target builds (local compile check; the watch `.uapp` comes from CI).
- Simulator, end to end (`docs/experiments/sim_run.sh sim-wrong-turn.gpx 90`):
  route restored, "went off (61 m off)", "back on (20 m off)" when the runner
  rejoins at the start, and the activity saved as a FIT file.
- CI now builds the app as well as the probe.

The screens are still RunLVGL's: choosing a route and seeing the map is T3.

## T3: the screens (28 September 2026)

### T3.1 What there is

Screenshots of every step: `docs/screens/` (made by
`docs/experiments/capture_screens.sh`, which drives the simulator).

- **Start screen:** RunLVGL's wheel, with Intervals replaced by **Route**; its
  hint is the route in use (amber) or "No route".
- **Route list:** "No route" (a plain run), then the routes on the watch by
  name with distance and climb ("1.19 km, 0 m up"; "can't read this file" in
  red for a broken GPX), then "Add routes" (copy GPX by USB). Back from a
  preview, it opens on the route previewed.
- **Route preview:** the whole route, north-up (amber line, lime start, red
  finish), its name and summary. R1 keeps it, R2 goes back and restores the
  route chosen before.
- **Run faces** (L1/L2), with a route: **near map** (250 m to the edge),
  **far map** (1 km), **navigation** (to go, done, route length, on/off course,
  metres from the line), then RunLVGL's totals, lap and status faces. The maps
  are heading-up once the runner is moving (north-up before), with a white
  arrow a little below centre, an "N" marker, and "x to go" (or "start x m"
  before joining the route).
- **Alerts:** going off course jumps to the near map and shows a red
  "OFF COURSE 61 m" banner over every face until back on; then a green
  "BACK ON COURSE" for 4 s; at the finish "ROUTE COMPLETE" for 8 s. Buzz and
  beep patterns as T2.3.

### T3.2 LVGL's pool: one screen, one face at a time

The SDK's `lv_conf.h` fixes LVGL's pool at 40 KB (about 36 KB usable).
Starting a run crashed the simulator: RunLVGL builds the next screen before
freeing the last, and its four run faces alone take about 24 KB, so the start
screen (18 KB) plus the run screen with maps did not fit. Two changes, both in
our copy of RunLVGL (the SDK is untouched):

- `ScreenManager` frees the old screen before building the new one (the new,
  empty screen is loaded first, all inside one LVGL tick, so nothing flickers).
- `TrackScreen` builds only the face on display; paging deletes it and builds
  the next (about 10-14 KB per run screen; no growth over repeated paging).

Measured peak over the whole walkthrough: 82 % (RunLVGL's own summary screen,
28.5 KB). Every screen and face change logs `LVGL pool (...)`.

### T3.3 Text on a watch with ASCII fonts

- Route names are UTF-8; the fonts are ASCII. `Core/TextFold` folds accents
  (Welsh ŵ, ŷ; é, ü, ł...), dashes and curly quotes to ASCII (host-tested).
- Long names step down the font (SemiBold 30, 25, 20, Medium 18) and are cut
  with ".." only if they still do not fit.
- The 40 pt number faces are digits only (RunLVGL's `gen_assets.py`), so the
  navigation face prints "691" and "m" as separate labels.

### T3.4 Found in the simulator: "route complete" from a noisy fix

With the run started elsewhere on the stadium, the watch said "finished" 9 s
after going off course. On the west bend the runner passes 20 m from both the
wrong-turn route's start and its finish (10 m apart); one noisy fix put the
finish within the 20 m rejoin reach and the start just outside it.
Reproduced on the host from every start point round the track
(`TheWrongTurnRouteIsNeverFinishedFromAnywhereOnTheTrack`).

Fixed in `RouteTracker`: the finish is earned. The route is split into 64
parts; a part counts as covered only by ordinary steps (from one matched fix
to the next, at most 2 missed, at running speed). A finish needs half the
parts covered. Rejoins still move the position (a shortcut is followed) but
cover nothing, and laps of the same stretch count once
(`SkippingMostOfTheRouteIsNoFinish`; the 40 % wrong-turn test still
finishes).

**Known limit:** on a route whose legs run 10-20 m apart, a runner off course
who crosses the other leg can be placed on it, so "to go" can read wrong
until they rejoin properly. Alerts and the finish are unaffected. Real trails
rarely have parallel legs that close; the field test (T4) will tell.

### T3.5 Verified

- 95 host tests pass (TextFold 8, new tracker tests 2).
- Watch target builds with no warnings (compile check; the `.uapp` for the
  watch comes from CI).
- Simulator walkthrough (`capture_screens.sh`): list, preview, back, choose,
  start, every face, off course at 61 m (jump to map, banner), back on at
  17 m, pause, save, summary. No crash; pool peak 82 %.

Not done: RunLVGL's intervals screens are still compiled but unreachable
(harmless; tidy up later). The look is RunLVGL's; if the promo videos show a
different style, send screenshots and it can be matched.

## T3b: the promo look and features (28 September 2026)

Jon's promo slides (concept preview) set the target; the screens now follow them.
Screenshots: `docs/screens/`.

- **Colour.** Route magenta with a dim wider line beneath it for the glow; start
  a green dot, finish red; red "N"; white arrow. The Route hint on the start
  screen and RunLVGL's summary map are magenta too.
- **Run map.** "9.9 km to go" at the top under the red N; a scale bar at the
  bottom naming a round distance (200 m, 500 m, 1 km, 2 km; ft and miles when
  the watch is set to imperial).
- **Zoom on one button.** R2 on the map cycles 300 m, 750 m, 1.5 km, 3 km (the
  radius to the screen edge) and the whole route with the runner on it
  (`MapZoom.hpp`; kept in the Model). The lap button (R2) works on the other
  faces; laps are not needed on the map.
- **Heading up or north up.** Settings, "North-up map" (a toggle, kept in
  `settings.json` as `map_north_up`). Default heading-up. The heading is GPS
  direction of travel (held while standing). **Not as in the promo:** "the
  compass when you stop": the compass has never calibrated on the watch (T0),
  so it is not used.
- **Off course banner.** A yellow band across the middle, "Off course" and
  "48 m from the line", until back on; then a green "Back on course" (4 s);
  "Route complete" with the distance (8 s).
- **Run face** as in "It's a run, too": distance big, pace and time, heart
  rate, and the lap in magenta. RunLVGL's lap and status faces follow.
- **Route preview** as in "Pick a route": the name (cut to fit) above the glowing
  route, "1.19 km, 0 m up", and **R1 starts the run** on it (with the usual "no
  GPS, start anyway?" if there is no fix).
- Face order with a route: map, navigation, run, lap, status.

Not built: the promo slides' own extras (the dotted "48 m" leader line and the
"along the line" bar are slide art, not watch screens). Ascent shows 0 m for
the simulator's routes because they carry no elevation.

Verified: 95 host tests; watch target builds (compile check); the simulator
walkthrough `capture_screens.sh` (zoom cycle, off course at 50 m, back on at
18 m, save); pool peak 82 %.

## T3c: compass, turn cues, the way back, elevation (28 September 2026)

Jon's compass works now (T0's "never calibrated" was before calibrating it),
so it is used. Screenshots: `docs/screens/` (17 elevation, 21-22 the way back,
27 turn cue).

- **Heading (HeadingFusion).** GPS direction of travel above about 1.8 m/s,
  the compass below 1.2 m/s (hysteresis between). The compass is tilt-levelled
  with the accelerometer (SDK `getAzimuthDegTilted`), averaged as a vector (so
  359 and 1 make 0), and given an offset **learnt while running** (GPS heading
  minus compass, averaged slowly, only above 2.5 m/s). The SDK applies no
  declination and reports the bearing of 12 o'clock; the learnt offset covers
  both, so no declination table and no setting. Until it has learnt, the offset
  is 0 (about 1 degree in the UK). The sensors are connected only while a run
  with a route is going (battery), 5 Hz, as the probe read them. Untested on the
  watch: the simulator has no magnetometer, so only the GPS half was seen there.
- **Turn cues (TurnFinder).** A turn is a change of direction of 45 degrees or
  more over 30 m chords either side of a point, sampled every 10 m along the
  route (a grid fixed to the route, so a turn is found in the same place every
  second and cued once). The map shows "Right 120 m" from 400 m out; 50 m
  before, one cue: **left = one firm click** and one beep, **right = the double
  click** and two short beeps; a sharp turn (110 degrees+) or a U-turn (145+)
  says it twice, and a magenta "Turn right / in 50 m" band shows for 4 s.
  Cues only while an activity is running, never while off course. Limit: the
  route on the watch is thinned (10 m and up between points), so a turn is
  found to within that spacing; very long routes (wide spacing) round corners
  and gentle ones can be missed.
- **The way back.** Off course (25 m and more), the yellow band has an arrow
  pointing at the nearest part of the route, turned by the way you face (or
  true north on a north-up map), with the distance. The status also carries a
  "to the start" guide before the route is joined (not shown yet).
- **Elevation profile.** A face after Navigation, only for routes with
  elevation: the profile as a line (run part bright, to come dim, a dot where
  you are), "269 m to climb", "+95 m in 218 m" (the next climb of 25 m or
  more, dips under 8 m allowed) or "Climbing +47 m", and "Now 250 m" (the
  route's elevation at your progress, not the noisy GPS altitude). 96 bins, so
  climbs are placed to about 100-200 m. The builder now keeps elevation and
  climb-so-far per point (re-thinned with the points).
- Face order: map, navigation, elevation, run, lap, status.
- 135 host tests (40 new: fusion, turns, elevation, navigator); watch build
  clean; simulator walkthrough with the new faces.


---

## Phone delivery over BLE (29 September 2026)

### Why

USB works: Jon has copied a GPX into `Apps/HybridXTrail/Routes/` from a laptop
and, with a USB-C to USB-C cable, **from his phone** (the watch mounts as a
removable drive on both). But it relies on the user knowing that folder, which
athletes won't. The goal for release: a GPX on the phone, a tap, and it's on the
watch.

### What the platform offers (research pass)

- **Hardware** (`UNAWatch/una-hardware`, BOMs): STM32U5A5 MCU, eMMC storage,
  USB (mass storage), and a **BlueNRG-2, Bluetooth Low Energy only**. No
  Wi-Fi, no NFC. So a file reaches the watch by USB or by BLE, nothing else.
- **Watch apps have no phone channel.** No SDK interface lets an app talk to the
  phone (`Libs/Header/SDK/Interfaces/*`); an app only sees its own folder
  (`IFileSystem`). Anything from the phone must arrive as a file.
- **The BLE File Transfer Service** (`Docs/BLE-File-Transfer-Service.md`,
  service `0xFEBB`) reads and writes any file, including
  `/Apps/<AppDir>/...`. It needs a bonded, encrypted link
  (`Docs/BLE-Services-Overview.md:7-8`). This is how the UNA app writes
  `configFile` (`Docs/app-config-fields.md` §9.2).
- **UNA's config fields can't carry a route:** at most 32 fields
  (`app-config-fields.md:135`) of at most 128 bytes (`:755`), so 4 KB, typed
  in by hand.

Options weighed with Jon:

| Option | Verdict |
|---|---|
| USB from laptop or phone | Works (Jon, T0 and 29 Sep). Needs folder knowledge, so it stays a developer path |
| UNA app config fields | Not a route (above) |
| A web page (Web Bluetooth) | **No.** iPhone browsers have no Bluetooth at all. On Android, Chrome's device chooser only lists advertising devices, and the watch doesn't advertise while the UNA app is connected (test 1 below) |
| The HybridX app | Not in any store yet (a PWA for users; a personal test APK). Jon prefers not to tie this to it |
| UNA adds "send GPX to an app" to their app | The best outcome, and the only easy route for iPhone users. Asked (`UNA_GPX_REQUEST.md`, forum post) |
| **A small standalone Android app** | **Chosen as the fallback and the proof.** Test 2 shows a second app can use FTS. Built: `Tools/RouteSender` |

### Tests on Jon's phone (Android, nRF Connect, 29 September 2026)

1. **Scan:** the watch **did not appear**, even with the UNA app open. Most
   likely it stops advertising once connected. Not yet tried: the UNA app
   force-stopped, or a pairing mode on the watch.
2. **Bonded tab → Connect:** "UNA WATCH 042648" (`7B:9D:B9:72:A9:44`) is listed,
   and connecting **worked while the UNA app stayed connected**
   ("CONNECTED / BONDED"). There was no pairing prompt; the bond the UNA app
   made belongs to the phone, and Android shares the link between apps.
3. **Services the watch exposes:** `0x1801`, `0x1800`, `0x180A` (Device
   Information), `0x1805` (Current Time), `0x180F` (Battery), **`0xFEBB`**, and
   two custom services, `554e4100-a2cf-4df8-0000-7e1e48595106` and
   `554e4100-28e7-4811-0000-141f8b92ee40` (`55 4E 41` is "UNA" in ASCII: UNA's
   own, undocumented, not ours to use).
   - **Conflict with the SDK docs:** `Docs/BLE-Services-Overview.md` lists a
     Nordic UART service (`6E400001-…`), but the watch doesn't expose one.
     Nothing here needs it; it's worth mentioning to UNA.
4. **FTS characteristics:** `adaf0001` (READ) = `05-00-00-00`, so **protocol
   version 5** (fast transfer: windowed writes, `DIGEST`). `adaf0002` is NOTIFY
   and WRITE NO RESPONSE, as documented.
5. **A real FTS command from a second app:** with notifications on, Jon wrote
   `50 00 05 00 2F 41 70 70 73` (LISTDIR `/Apps`) by hand. The last reply was
   `51-01-00-00-1F-00-00-00-1F-00-00-00-…`: status OK, entry 31 of 31 with no
   name, i.e. the listing's terminator. **`/Apps` holds 31 entries, and the
   watch answered a file command from another app while the UNA app was
   connected.** (Decoded in `Tools/RouteSender/tests/.../HostTests.java`.)

**Reading:** a native phone app can reach the watch through the phone's existing
bond (no scan, no extra pairing) and use FTS. That's the whole transport a
sender needs. What's left to prove is a real write that Trail then shows
(Route Sender's test plan).

### HybridX Route Sender: the test app (`Tools/RouteSender`)

A one-screen Android app, written in plain Java with no Gradle or libraries:
- pick a GPX, or **Share → Route Sender** from any app;
- it finds the paired watch ("UNA" in the name), reads the FTS version and
  checks `/Apps/HybridXTrail` exists;
- it runs MKDIR `Routes` if missing, WRITEs the file (v5: 2 KB window with
  go-back-N; v4: stop-and-wait), then compares the watch's DIGEST (CRC-32 and
  size) against the phone's;
- a log to copy back.

Names follow Trail's own rules (`Navigator.cpp` `isGpx`, `RouteInfo::file[48]`).
The pure parts (`Fts`, `RouteFile`) have host tests, 47 checks, including
Jon's bytes above.

- **Builds:** `dl.google.com` (Google's Android SDK) is blocked from the Claude
  container, so `build.sh` also builds with Ubuntu's Android packages (API 23
  `android.jar`, `aapt2`, `dx`, `apksigner`). The source sticks to API 23
  calls, and uses the Android 13+ write and notify forms by reflection so
  that a reply can't overwrite a packet being sent. CI builds it with GitHub's
  full SDK (`.github/workflows/route-sender.yml`, artifact
  `route-sender-apk`).
- **Signing:** a test-only key in the repo (`debug.keystore`), so builds
  install over each other. A store release needs its own key, kept out of git.
- **Untested:** no emulator or watch here. Everything past the connection is
  checked only against the protocol doc and the host tests until Jon runs it.

### Route Sender run 1 (Pixel 7, Android 17, 29 September 2026)

- **What happened:** version 0.1.0 connected in about 70 ms and read MTU 220,
  then failed with "The watch has no file transfer service (0xFEBB)" 3 ms
  later, on both tries. The file was a 1.27 MB recorded activity GPX.
- **Cause (in the app, not the watch):** every GATT wait shared one
  semaphore. A second MTU callback (Android reports the link's MTU by
  itself, as well as answering the request) released it just after it was
  drained for service discovery. So the app read the service list before
  discovery had finished. nRF Connect's list shows FEBB is there.
- **Fixed in 0.1.1:** one semaphore per callback kind (MTU, discovery, read,
  write, descriptor). Discovery is also retried up to three times if FEBB is
  missing, and every attempt logs the services it saw.

### Route Sender run 2: **phone delivery works** (0.1.1, 29 September 2026)

The first end-to-end send, from a Pixel 7 with the UNA app connected:
- **Connection:** connected in 94 ms, MTU 220 (205 bytes of file per packet).
  Discovery found all eight services, FEBB included, on the first try.
  Version 5.
- **Before the write:** LISTDIR `/Apps` gave 31 entries and found
  `HybridXTrail`. `Routes/` held 3 routes (`AR_Ham2Lyme_50k_26.gpx`
  128,857 B, `125 in the lanes.gpx` 18,371 B, `ridge-loop.gpx` 94,093 B).
  These were the USB copies, including a name with spaces.
- **The write:** 685 bytes in 0.79 s, then **DIGEST matched** (size and
  CRC-32 `d7c6c871`). A second listing showed 4 routes.
- **Not measured yet:** speed. The file was too small to show it; 0.79 s is
  mostly round trips. The log showed "0 KB/s" from integer rounding, so
  0.1.2 logs bytes per second instead.

**Reading:** a GPX chosen on the phone lands in Trail's `Routes/` over BLE,
verified, with no folders for the user and no USB. The transport question is
answered.

### Open (phone delivery)

- [x] Jon: run Route Sender's test plan. The core send passed in run 2.
- [ ] Trail shows and loads a route sent this way (check on the watch).
- [ ] A real-sized route (about 100 KB) for speed, and the Share entry from
      another app.
- [ ] Does Trail see a route that arrives while it's running, or only after
      reopening? `Navigator::scan()` runs when the route list is built.
- [ ] Does the UNA app notice or mind another app using FTS on the same link?
      Both apps share one FTS channel and its replies.
- [ ] Transfer speed for a typical route (the log's `Written in` line).
- [ ] UNA's answer to the forum post and the request.
- [ ] Scan with the UNA app force-stopped: does the watch advertise then?
      This only matters for a web-page sender, which is ruled out anyway on iPhone.
