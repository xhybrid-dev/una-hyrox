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
