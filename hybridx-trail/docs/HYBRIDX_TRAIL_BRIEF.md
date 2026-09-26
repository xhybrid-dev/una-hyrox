# HybridX Trail for UNA Watch: Brief and Plan

**Owner:** Jon
**Status:** Phase T0 (the probe) built, waiting for Jon's run on a watch. Nothing
past T0 is started.
**Read with:** `NOTES.md` (findings, with SDK citations), `PROBE.md` (Jon's
steps for T0), `UNA_GPX_REQUEST.md` (the note to UNA about phone delivery).

---

## 1. The idea

Breadcrumb navigation for runners and trail runners, the way the older and
entry-level Garmins did it. You plan a route in OS Maps, Komoot, Strava or
anything else that exports GPX, put the GPX on the watch, and follow the line.

That's all it is. No street maps, no turn-by-turn directions, no rerouting.
Just the line, where you are on it, and a buzz if you leave it.

It is sport-agnostic and separate from HybridX Race, like Streak and
Intervals. HybridX coaches runners and trail runners as well as HYROX
athletes; this is for them.

## 2. What it does (v1)

| # | Feature | Notes |
|---|---|---|
| F1 | **Routes from GPX files** copied over USB into `Apps/HybridXTrail/Routes/` | The watch reads the GPX as exported, with no conversion step. See section 4 |
| F2 | **Route list** on the watch: name, distance, ascent | The name comes from the GPX, or the file name if it has none |
| F3 | **Map screen:** the route as a line, a marker for you, a start and finish marker | Drawn as an LVGL line, like RunLVGL's `Widgets::Map` |
| F4 | **Zoom:** fixed scales, such as 200 m, 500 m, 1 km, 2 km and the whole route | One button cycles them |
| F5 | **North-up or heading-up** | Heading-up rotates the map to the way you're facing: compass when standing, GPS direction of travel when moving. The default is decided at Gate T0 |
| F6 | **Off-course alert:** a buzz and a banner when you're more than a set distance from the line, and another when you're back on it | Default 50 m, with some hysteresis so it doesn't chatter on switchbacks. The threshold is a setting |
| F7 | **Distance done and remaining along the route,** not in a straight line | Tracks your progress along the line, so an out-and-back or a loop that crosses itself doesn't jump to the wrong leg |
| F8 | **A normal run recording at the same time:** time, distance, pace, heart rate, laps, saved as a FIT activity | Started from RunLVGL's recording, so it arrives in Strava or Garmin Connect like any run |
| F9 | **Data screens** as in a normal run app, alongside the map | Reuses RunLVGL's faces |

**Later (P1), not v1:** follow a route in reverse; "back to start"; a
breadcrumb of where you've actually been drawn under the route; elevation
profile with climb remaining; phone delivery of routes (section 4); courses
exported from the HybridX platform.

**Never, on this hardware:** street or topo maps, turn-by-turn directions,
rerouting (`NOTES.md`, "What the platform can't do").

## 3. The watch, briefly (what shapes the design)

- **Display:** 240 x 240, round, four levels per colour channel (ABGR2222). A
  thin line must be readable in daylight; colours are limited. The route line
  will be at least 3 px wide, as RunLVGL's map is.
- **Memory:** no MMU and a fixed RAM budget. The route has a fixed number of
  points (2,000 in the probe, 16 KB) and the GPX is read a chunk at a time,
  never held whole. The real limit comes from the probe (Gate T0).
- **Apps run one at a time, full-screen.** As far as the SDK shows, a
  navigation overlay on top of another app isn't possible, so Trail is itself
  a run-recording app with navigation built in. To be confirmed with UNA
  (`NOTES.md`, open questions).
- **Sensors:** GPS position at 1 Hz, and a magnetometer with a calibration flag
  and a bearing in the SDK (magnetic north, no declination applied).

## 4. How routes get onto the watch

**v1: USB.** The watch appears as a USB drive (SDK `Docs/deploy.md`); that's
how apps are installed, and the SDK says an app's own folder can be edited over
USB. Jon or an athlete:

1. plans and exports a GPX on a laptop;
2. connects the watch by USB and waits for its drive;
3. copies the GPX into `Apps/HybridXTrail/Routes/`;
4. ejects safely and unplugs;
5. opens the app and picks the route.

Whether step 5 needs a power cycle first is one of the probe's questions. The
watch can't be used while its drive is attached, so this is a "night before"
flow, not one for the trailhead.

**Later: from the phone.** Either UNA adds "open GPX in UNA, send to app" to
their phone app (`UNA_GPX_REQUEST.md`), or HybridX builds its own sender over
the documented BLE File Transfer Service (the path HybridX Intervals' probe is
testing). **The watch app doesn't change either way:** it reads whatever is in
`Routes/`, however it got there.

## 5. Architecture

Same shape as Race and Streak: a pure C++ core with host tests, a service
process that owns sensors and recording, and an LVGL GUI process.

**Core** (`Software/Libs/Core`, pure C++, no kernel, host-tested). The first
four parts already exist, written for the probe so it tests the real code:

| Part | Job | State |
|---|---|---|
| `GeoPoint` | Positions as integer degrees x 10^7; flat-earth distances, exact for short spans | Done, tested |
| `GpxReader` | Streaming GPX reader: any chunk size, fixed memory, `trkpt` and `rtept`, namespaces, comments, CDATA, entities | Done, tested |
| `RouteBuilder` | Thins any file into a fixed array, evenly along the route; length and ascent from every point | Done, tested |
| `RouteMath` | Distance from a position to the route | Done, tested |
| `RouteTracker` | Progress along the route: which segment you're on, distance done and remaining, with continuity so crossings and out-and-backs don't jump | T1 |
| `OffCourse` | The alert state machine: threshold, hysteresis, a few seconds' confirmation, and "back on course" | T1 |
| `MapView` | Projects the route around you at a zoom level and heading into screen pixels, clipped to the display | T1 |
| `RouteCache` | A compact binary copy of a parsed route next to its GPX, so a big file is parsed once, not every time | T1, if T0's read times say it's needed |

**Service:** RunLVGL's service (GPS, heart rate, FIT recording, laps) plus the
route: load the chosen route, feed each GPS fix to `RouteTracker` and
`OffCourse`, buzz on alerts, and send the GUI what it needs once a second.

**GUI:** route list, route summary, the map screen, RunLVGL's data screens,
and the off-course banner. How the route's points reach the GUI (a run of
messages at start, or the GUI reading a compact route file the service wrote)
is decided in T2: a 256-byte message holds about 30 points.

## 6. Phases and gates

Same rules as the other apps: plan mode at the start of each phase, stop at
each gate, summarise what was done, how it was verified and what Jon must test
or decide. Build the watch target and the simulator after every change.

| Phase | Work | Gate: what must be true to go on |
|---|---|---|
| **T0: probe** | The Trail Probe (`Tools/Probe`, `PROBE.md`): a GPX copied over USB is found and read on the watch; memory; GPS time to fix; compass calibration and bearings; distance to the route | **Gate T0:** a GPX copied over USB is read on the watch. Record the read time, the memory and whether a power cycle was needed; decide heading-up's source and the route point limit from what the probe shows |
| **T1: core** | `RouteTracker`, `OffCourse`, `MapView`, maybe `RouteCache`, all host-tested with synthetic routes (loops, out-and-backs, figure-of-eights, GPS noise) | **Gate T1:** host tests pass; Jon agrees the alert behaviour from a written walk-through of the test cases |
| **T2: service** | Start from RunLVGL's service: recording, GPS, route loading, tracking, alerts, messages to the GUI. App manifest and icons | **Gate T2:** watch build and simulator run a route end to end in the simulator, recording a FIT file |
| **T3: screens** | Route list, summary, map with zoom and heading, data screens, off-course banner; British English; captures from the simulator | **Gate T3:** Jon approves the screens from simulator captures |
| **T4: on the watch** | Jon's field test: a known local loop, with a deliberate wrong turn. Battery over a long session | **Gate T4:** alerts fire when they should and not when they shouldn't; the FIT imports into Strava/Garmin Connect; battery life recorded |
| **T5: release** | Store package, version tags `trail-vX.Y.Z` (a version script like Streak's) | **Gate T5:** Jon uploads to the portal |

## 7. Decisions for Jon

1. **App name.** "HybridX Trail" is the working name.
2. **USB only for v1?** Recommended: yes, and add phone delivery when UNA answers.
3. **Off-course threshold default:** 50 m suggested. You know what trail runners
   will tolerate better than I do.
4. **Units:** kilometres only, or miles as a setting? (The other apps are metric.)
5. **Heading-up or north-up by default:** decide after T0, once the compass has
   been seen on a real wrist.

## 8. Questions for UNA

1. Phone delivery of a GPX to a third-party app (`UNA_GPX_REQUEST.md`).
2. Can two apps run at once (a navigation overlay on another app), or is one
   full-screen app at a time the model?
3. How does a user calibrate the compass, and does `MAG_CALIBRATED` stay set
   once they have? The SDK reports the flag but doesn't say how calibration
   happens.
4. Is there a recommended way to get magnetic declination (the difference
   between magnetic and true north) for a position? The SDK leaves it to the
   app. At UK latitudes it's currently small, but not everywhere.

## 9. Risks

| Risk | What happens | Mitigation |
|---|---|---|
| GPS accuracy under trees and in valleys | False off-course alerts | Hysteresis and confirmation time in `OffCourse`; tune on Jon's field test |
| Compass not calibrated, or unreliable near metal | Heading-up map spins | Fall back to GPS direction of travel when moving; north-up as the safe default |
| Battery on long days | An ultra outlasts the watch | Measure in T4; offer a lower GPS rate if the SDK allows |
| USB delivery is too fiddly for athletes | Low use | Phone delivery (section 4); a one-page picture guide meanwhile |
| Very long routes | Coarser line | `RouteBuilder` thins evenly to fit; T0 measures what memory allows |
