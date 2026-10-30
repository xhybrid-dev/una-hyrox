# HybridX Ride: notes

Findings and decisions for a future bike app. So far there is only the
**Gate B0 probe** (`Tools/Probe`), which asks whether the watch can read a
smart trainer, bike sensors or a Concept2 BikeErg. Jon's test steps are in
`PROBE.md`; the draft request to UNA is `UNA_BIKE_SENSORS_REQUEST.md`.

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
  says: heart rate only. Not yet tested with this probe: the trainer, bike
  sensors and BikeErg with external HR on and the strap off (the kernel looks for
  a *remembered* heart-rate device, so a trainer will only show if it can be
  paired in Settings as one). The firmware version is not yet noted.
- **Still to note from Jon:** firmware version; whether the watch's Settings or
  the UNA app list any cycling sensor, trainer or speed/cadence/power option.

## 4. Decisions

- **Probe first, then a request to UNA** (Jon, 28 October 2026). No bike app
  is planned until the probe's result is in.
- If a bike app comes before UNA adds sensors, it can only be GPS and heart
  rate outdoors, or time and heart rate indoors.
