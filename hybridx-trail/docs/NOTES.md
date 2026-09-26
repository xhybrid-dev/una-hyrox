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

## Findings for Jon (fill in after running `PROBE.md`)

- [ ] Did the probe see the GPX copied over USB **without** a power cycle?
- [ ] Verdict, file size, points read and kept, and read time for a long route.
- [ ] Largest single allocation in the service.
- [ ] GPS: seconds to first fix, and the precision it reported.
- [ ] Compass: calibrated or not; how close the bearing was to a known
      direction, level and raised; samples per second (the period unit).
- [ ] Off-route distance, if the route started nearby.
- [ ] Where the GPX came from (OS Maps, Komoot, Strava, other).

## Gate T0: what decides next steps

**GO:** the GPX copied over USB is read on the watch. Record the read time,
memory and power-cycle answer; set the route point limit; decide whether a
`RouteCache` is needed (only if a long route takes seconds to read); choose
heading-up's source (compass, GPS course, or both). Then T1.

**Not GO:** USB delivery into an app folder doesn't work as documented. Ask UNA
before any workaround.
