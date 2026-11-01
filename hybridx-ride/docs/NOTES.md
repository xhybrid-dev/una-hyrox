# HybridX Ride: notes

A bike app for the UNA Watch that counts in HybridX Streak. It has two parts:
- **The app** (`Software/`, phase R1, section 5): heart-rate and GPS rides, outdoors or
  indoors, saved as a cycling FIT file, counted in Streak through the activity link.
- **The Gate B0 probe** (`Tools/Probe`, sections 1 to 4), which asked whether the watch
  can read a smart trainer or bike sensors. It cannot: heart rate only. Jon's test
  steps are in `PROBE.md`; the draft request to UNA is `UNA_BIKE_SENSORS_REQUEST.md`.

## 1. What the SDK offers (v1.6.2, `6bd43f04`)

- **Apps have no Bluetooth of their own.** The one route to an external
  sensor is the kernel's accessory contract (`SDK/Messages/AccessoryMessages.hpp`,
  `Docs/ExternalSensors.md`): the app sends `RequestPrepare(kinds)`, the kernel
  scans and connects, and reports `EventStatus` (kind, state, name). The
  readings arrive through the ordinary sensor types.
- **v1 acquires heart-rate straps only.** `Kind::HRM = 1u << 0` ("Heart Rate
  Service (0x180D)"). The header goes on:

  > v1 ships HRM only; the other bits are reserved so the contract is stable as
  > cycling cadence/power profiles are added later.

  with `// CADENCE = 1u << 1,` and `// POWER = 1u << 2,` commented out.
- **No sensor type carries bike data.** `SensorTypes.hpp` has no speed,
  cadence or power types. Even if the kernel connected a cycling sensor, an
  app would have nothing to subscribe to.
- **The SDK's Cycling example** uses GPS, heart rate and the barometer only.
- **`HEART_RATE_EX`** says which source the heart rate came from (0 unknown,
  1 optical, 2 external) and the strap's own bpm. The probe logs it, so a
  device that answers as a heart-rate source (some trainers and the PM5
  relay a strap) would show up there.
- **External HR must be switched on in the watch's Settings.** If it is off,
  or no strap is remembered, the kernel answers `UNAVAILABLE`.
- **The simulator's mock kernel does not answer accessory requests.** It
  sends no `EventStatus` at all, so only a watch can show what the kernel
  does with the reserved bits.

So the expected answer is "heart rate only". The probe exists to confirm it
on Jon's firmware and to give UNA evidence, not because we expect a surprise.

## 2. The probe (Gate B0)

`Tools/Probe` is its own CMake project, built from Knitting's skeleton (one
LVGL screen) and Race's accessory handling. App folder `RideProbe`, shown as
**Bike Probe**, development APP_ID `10808682C0E2ED6A`, version 0.1.0.

- **R1** releases everything (`RequestRelease(0)`) and sends the next
  `RequestPrepare`, in this order, then round again:

  | Step | `kinds` | What it is |
  |---|---|---|
  | 1 | `0x1` | HRM: the control, which must reach CONNECTED with a strap |
  | 2 | `0x2` | bit 1, reserved for cadence |
  | 3 | `0x4` | bit 2, reserved for power |
  | 4 | `0x6` | bits 1 and 2 |
  | 5 | `0x7` | bits 0, 1 and 2 |

  The reserved bits are sent as plain numbers: no SDK name is invented.
- **What it records.** Every `EventStatus`, as "12.3s 0x2 SEARCHING name"
  (seconds since the last request); and `HEART_RATE_EX` (source, strap bpm)
  when it changes or every 5 s.
- **Where.** The last 32 lines on screen (6 shown; L1 newer, L2 older), and
  `Logs/probe_<utc>.csv` in the app folder, for USB:
  `ms,event,request,kind,state,name,hr_source,ext_bpm`. `event` is
  `request`, `status` or `hr`; `state` is -1 on rows that are not statuses.
  A device name is cleaned of quotes, commas and line breaks.
- **R2** exits. The service releases on the GUI's stop and on `APP_STOP`, and
  the kernel releases anyway when an app stops.
- The text and CSV formatting is pure, header-only (`Libs/Core/Header/ProbeText.hpp`)
  and host-tested (`Tests/Host`, 3 tests).

### Verified in the container (28 October 2026)

- Host tests: 3 of 3 pass.
- Simulator: five R1 presses sent the five requests in order; no status came
  back (the mock is silent, as above); the wrist heart rate was logged as
  source 1; the CSV was written. The screen fits the round face.
- Watch compile check: `RideProbe_0.1.0.uapp`, 179,008 bytes (a local build,
  never for a watch; the one to install comes from CI's `watch-apps`).

### What the result will mean

| What Jon sees | Meaning | Next |
|---|---|---|
| Step 1 CONNECTED with the strap; steps 2-5 UNAVAILABLE, IDLE or nothing | The kernel knows only HRM, as the SDK says | A bike app waits for UNA, or ships on GPS and heart rate only |
| A reserved bit reaches SEARCHING, CONNECTING or CONNECTED with a bike device | The firmware is ahead of the SDK | Tell UNA, ask for the matching sensor types; still no data until they exist |
| The trainer or PM5 shows as the heart-rate source (`hr_source` 2) | It relays a strap as a heart-rate service | Heart rate only; no power or cadence |

## 3. Gate B0 result so far (30 October 2026, Jon's watch)

Logs: `probe-logs/b0_run1_no_strap.csv`, `probe-logs/b0_run2_strap.csv`.

- **Run 1 (strap not connected, trainer on):** 15 requests, three rounds, and
  **no `EventStatus` at all**, not even for the HRM control; `hr_source` stayed 1
  (wrist). Silence, not `UNAVAILABLE`. It says nothing about the reserved bits,
  because the control gave no answer. (Why the kernel was silent is not known; the
  likely cause is external HR off or no strap remembered, but this run cannot
  show it.)
- **Run 2 (strap `HRM-Dual:866224` working): the control passes.** `0x1` went
  SEARCHING (0.2 s), CONNECTING (3.4 s), CONNECTED (6.6 s); `hr_source` became 2
  with the strap's bpm (82-84). So the probe does receive statuses, and run 1's
  silence was the kernel's.
- **The reserved bits do nothing.** For `0x2`, `0x4` and `0x6`, no status with
  kind `0x2` or `0x4` ever came: not SEARCHING, not UNAVAILABLE. The only row was
  `kind 0x1, IDLE` straight after `0x2` (the release of the strap, because
  `nextRequest` releases everything first), and `hr_source` fell back to 1.
  For `0x7` (all three bits), every status was kind `0x1` (SEARCHING, CONNECTING,
  SEARCHING, CONNECTING, CONNECTED in 15 s: the strap had only just been
  released); nothing for bits 1 or 2. Round two repeated `0x1` and `0x2` with the
  same result.
- **Meaning:** on this firmware the kernel ignores the reserved bits, as the SDK
  says: heart rate only.
- **Jon's report (30 October 2026):** the watch sees and connects an external
  heart-rate device only. No other sensor type is supported, and it does not see
  any (his turbo trainer, which sends power, cadence and speed over Bluetooth,
  never appeared). This is Jon's account of the watch's Settings and the trainer
  test; no probe log of a trainer run with a working control exists, and the
  bike sensors and BikeErg were not tried. The firmware version is not noted.
- **Gate B0 is closed: heart rate only.**

## 4. Decisions

- **Probe first, then a request to UNA** (Jon, 28 October 2026). No bike app
  is planned until the probe's result is in.
- If a bike app comes before UNA adds sensors, it can only be GPS and heart
  rate outdoors, or time and heart rate indoors. (Built as R1, section 5.)
- **Gate B0 result** (Jon, 30 October 2026): for now a bike app can only monitor
  heart rate, plus GPS when riding outside. No power, cadence or speed
  sensors, and nothing from a trainer or BikeErg. The next step is the request
  to UNA (`UNA_BIKE_SENSORS_REQUEST.md`, still a draft for Jon to check and
  send); whether to build the GPS and heart-rate app meanwhile is Jon's call.

## 5. Phase R1: the ride app (30 October 2026)

Jon, 30 October 2026: "build the app with what we currently have, then we have the ability
to update when UNA adds more functionality. Also it allows bike workouts to be added to
Streak." Plan approved the same day.

### Phases

| Phase | Work | Status |
|---|---|---|
| **R1** | The ride: indoor and outdoor, FIT, screens, the activity link and the Streak line | **Built (30 October 2026)**, simulator-checked. Gate: Jon's watch test (below) |
| R2 | Not planned yet. Candidates: auto-pause (the Cycling example's), the example's speed correction, distance and time auto laps, road or mountain sub-sports (needs verified FIT numbers), the request to UNA for sensors, power and cadence once UNA adds them | |

### Decisions (Jon, 30 October 2026)

- **One app, a main-menu switch** between Outdoor and Indoor (remembered). Outdoor uses
  the GPS; indoor records time and heart rate only.
- **Calories: a heart-rate estimate**, Workout's: each active second, MET x weight / 3600,
  with the Workout example's MET per heart-rate zone. That table is the SDK Workout
  example's, not a cycling one, so for a ride it is a **rough figure**; no cycling table has
  been invented. The profile's weight, or 75 kg.
- **FIT:** sport Cycling (2). Outdoor sub_sport Generic (0); indoor IndoorCycling (6).
  Both are in the SDK's `FitProfile.hpp`. No road or mountain: their numbers are not in
  the SDK, so they stay out until verified (ask first).
- **Name and IDs:** folder `hybridx-ride/` (the probe stays in `Tools/Probe`), `HybridX
  Ride` on the watch (12 bytes), `HybridXRide` as the build, file and app-folder name (the
  name Streak keys sessions by), tags `ride-vX.Y.Z`. Development APP_ID `250009C2B1049597`
  (first 16 hex of md5("HybridXRide")); the portal issues the real one at release. The
  probe keeps its own ID.
- **Base:** HybridX Workout (X Sauna's layout, the watch-proven FIT writer with the
  dropped-seconds fix, the strap request, the activity link and the Streak line), plus the
  SDK Cycling example's GPS handling. Streak needed **no change**: `Sport::Ride`, `Slot::Ride`
  (`act_ride.json`) and Streak's `Kind::Ride` already existed, and Streak's own folder scan
  maps FIT sport 2 to Ride.
- **Left out of R1, on purpose** (changes from the approved plan's "optional" items):
  - **auto-pause**: an intricate state machine (hysteresis, manual versus auto, dead
    reckoning) that is better added once the basic ride is proven on a watch;
  - **the Cycling example's speed correction** (`GpsSpeedFilter`): R1 uses the raw GPS speed
    while it is valid and under 3 s old, and treats anything under 0.5 m/s as standing still.
    Distance comes from the kernel's `GPS_DISTANCE` (so it does not depend on that speed);
  - **time or distance auto laps**: only Workout's time-based auto lap (Off, 1 to 30 minutes).

### What it does

- **Main menu** (wheel; L1 up, L2 down, R1 selects, R2 exits; exits on 30 s idle):
  Start (hint: "Finding GPS", "GPS ready" or "Indoor ride"), Ride (Outdoor / Indoor), Heart
  rate (only shows the source: strap state or "Wrist heart rate"), Auto lap. Outdoors with no
  fix, the first R1 on Start warns ("No GPS: R1 again") and the second starts anyway. The
  GPS location is wanted from the moment the screen opens (outdoor), so the fix is ready by
  Start; it is released when the ride ends or the screen goes, and asked for again when the
  menu is back.
- **Live, outdoors** (L1/L2 turn the faces; R2 laps, R1 the action menu): Main (speed
  large, then distance and total time, heart rate and zone); Lap (lap time, lap distance, lap
  average speed); Totals (time, distance, climb, kcal); Zones; Clock. **Indoors:** Main (lap
  time large, total, heart rate, zone), Totals (total, kcal, laps, auto lap), Zones, Clock.
  Units (km, km/h, metres, or miles, mph, feet) follow the watch profile.
- **Saved** (with the Streak line, "3 of 4 this week" and "7-week streak", when Streak has left
  its summary), then the summary: overview, outdoors a Ride page (average and top speed,
  climb), zones, heart rate and calories, laps three to a page (outdoors with each lap's
  distance).
- **The file:** `Activity/YYYYMM/activity_<local time>.fit`, a sidecar `.json` with
  `activity_type` "cycling". Outdoor records carry position (when there is a fix), distance,
  enhanced altitude (from the barometer), enhanced speed and heart rate; laps and the session
  carry distance, average and maximum speed, ascent and descent; every file has calories,
  metabolic calories, and the lap's `resting_calories` developer field. Laps and the session
  are written at stop (a crash recovers the records, not the laps), as Workout's.
- **The link:** after the `.fit` closes, `Service::writeActivityLink` adds the ride to
  `../SharedData/HybridX/act_ride.json` as `Sport::Ride`, with the real distance outdoors and 0
  indoors, then `sendStreakLine()`.

### Layout

- `Software/Libs/Core` (pure, header-only where the GUI uses it): `RideModel` (laps, zones,
  calories, and the motion: distance and climb in centimetres, speed in mm/s, all integers),
  `RideFit`, `RideText` (clock, speed, distance, climb in either units), `RideSnapshot`,
  `RideSettings` (`{"v":1,"autolap":5,"outdoor":1}`), and Workout's `HrZones`, `LapTiming`,
  `SecondGate`, `HoldTimer`, `LapLockout`.
- `Software/Libs/App`: `Service`, `ActivityWriter`, `SafeFile`, `Commands.hpp`.
- `Software/Libs/Link`: a byte-identical copy (`hybridx-link/Utilities/check-link-copies.sh`,
  now with this app in its list).
- `Software/Apps/LVGL-GUI` and `Software/Apps/HybridXRide-CMake`; tests in `Tests/Host`
  (the probe's three stay there too); scripts in `docs/experiments`.

### Verified (30 October 2026, in the container)

- **Host tests:** 91 pass, also under ASan and UBSan. They include the motion in the model
  (distance, climb, top and average speed, per lap, saturation, nothing while paused, nothing
  indoors), the FIT values (climb rounded to whole metres), the text in both units and at the
  caps, and the settings file (the mode, a file without it, a bad mode refused with nothing
  changed). The link library's 40 tests pass; the copy check passes for six apps.
- **The FIT file from the real writer** (`docs/experiments/fit_check.sh`, five scenarios,
  decoded by fitdecode): `full` and `outdoor` (415 s, 7 m/s: 2905.00 m on the last record and
  the session, the laps summing to it, +15 m climb in lap 1 and -5 m descent in lap 3, no
  position for the first 30 s), `autolap`, `nohr`, `crash` (records recovered). Calories
  worked out again from the records match.
- **Every screen in the simulator** (`capture_screens.sh`, 41 frames, about 3 minutes):
  an outdoor ride with an auto lap and a pause, a discarded ride, an indoor ride; both
  saved `.fit` files decoded; `act_ride.json` holds two entries with each file's start and
  timer time, the outdoor one's distance (530 m) and the indoor one's 0; the Streak line read
  "3 of 4 this week" and "7-week streak", then "4 of 4" after the second save.
- **Text-fit audit** (`audit.sh`): 0 problems across 34 cases, outdoor and indoor,
  metric and imperial. It found one real overflow on the way (a 9999.99 km distance), so the
  display is capped at 999.99, as the activity link holds at most 1000 km. The wheel menu is
  the SDK's widget and is not audited: its hints were checked by eye in the captures.
- **Family check** (`hybridx-link/docs/experiments/family_check.sh`, now five apps): Run,
  Race, Trail, Workout and Ride saved one after another; their lines read "1 of 7" to "5 of 7";
  Streak's first open found five new and counted the ride as **Ride**; the second open found
  none new and still held five.
- **Watch target, local compile check:** both processes link, `HybridXRide_0.0.0-dev.uapp`
  326,208 bytes, no warnings from our code. Not installable: CI makes the one to install.

### Barometer readings and NaN (31 October 2026)

Altitude and climb come from the **barometer** (the PRESSURE sensor), not the GPS, and only
on an outdoor ride (indoors the sensor is not subscribed: on a turbo the only pressure
changes would be weather and air conditioning, shown as made-up climb). Altitude is the SDK's
ISA formula from the station pressure and the sea-level pressure (QNH) the kernel sends with
it; the climb is the sum of rises and falls above 2 m of the smoothed altitude (as the Cycling
example), so it does not depend on the QNH being right; the altitude on each FIT record does,
and where the kernel gets its QNH is not known.

The SDK returns NaN when either pressure is missing, and the Cycling example feeds that
straight into its filter and counter, where one NaN would spoil both for the ride. Ride drops
such a reading first (`Core/Header/RideAltitude.hpp`: both pressures finite and above zero,
the altitude finite and within -1000 to 10000 m). The first good reading sets the ride's
sea-level reference; unusable ones are counted and logged when the ride saves. Three host
tests (94 now); the simulator ride still records altitude on every record after the first
few seconds, so its readings pass the check.

### Not verified

- **Anything on a watch.** In particular: the GPS fix time and the "GPS ready" hint; the
  speed and distance (the simulator's are invented); the barometric altitude and climb (the
  simulator's pressure is constant, so climb reads 0; the sea-level reference is the
  first reading's `getP0()`, as the Cycling example); the strap states; the profile's weight,
  zones and units arriving; whether the UNA app and Strava treat a Generic cycling ride and an
  IndoorCycling one as expected, and what they show for calories; the LVGL pool on the watch.
- **Imperial units** have been audited for fit and unit-tested, not driven end to end in the
  simulator (it reports the profile as metric).

### For Jon at Gate R1

1. **Install** `HybridXRide_<version>.uapp` from the CI run's `watch-apps` into a new folder
   `Apps/HybridXRide`, then restart the watch. (The probe's folder, `Apps/RideProbe`, can stay.)
2. **Open Streak once** if it has not been opened this week, so it leaves its summary.
3. **An outdoor ride:** open the app, watch the Start hint go from "Finding GPS" to "GPS
   ready" (outdoors, clear sky), Start, ride a few minutes with a lap or two on R2, then End
   by holding R1 (R1 menu, L2 L2).
4. **An indoor ride** (the turbo): Ride row, R1 to Indoor, Start, a few minutes, End.
5. **Look at:** the Saved screen's Streak line; the summary pages (Ride page outdoors); the UNA
   app's activities (a cycling ride, and an indoor cycling one; distance, speed, calories,
   heart rate); then open Streak: "+1 Ride" each, counted once.
6. **Tell me:** how long the GPS took to find a fix; whether the speed and distance look
   right against a bike computer or phone; whether the climb is sensible; whether calories
   look reasonable (they are rough); whether "Strap connected" showed with your strap on
   (external HR on in Settings); and anything that was awkward on the wrist.
