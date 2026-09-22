# HybridX Race — Architecture

How the app is put together, and why it is put together that way. For what it
is supposed to do, read `HYBRIDX_RACE_BRIEF.md`. For what was found while
building it — measurements, dead ends, every place the SDK and the brief
disagree — read `NOTES.md`.

## Overview

A HYROX race is a sequence of alternating 1 km runs and eight fixed stations. The
athlete presses one button at every transition, and the watch records each
segment as its own lap. That is the whole app.

What makes it more than a lap counter is that **a split can be taken back**. An
athlete who thumbs the button on the way into the Roxzone rather than on the way
out must be able to undo it, and the times either side must come out exactly as
if the mistake had never happened. That single requirement shapes most of the
design below: it is why the race state lives in a pure data structure with an
explicit undo, why heart-rate samples are accumulated per segment as sum, count
and maximum rather than averaged eagerly, and why every FIT lap is written in one
batch at save time rather than streamed as it happens.

The second shaping force is the platform. There is no MMU, so a bad index is
memory corruption rather than a crash. Kernel messages come from a 256-byte pool,
and an oversized send returns null and is dropped **in silence** — on the watch
only; the simulator allocates with `new[]` and never shows the fault. The
millisecond clock wraps. Those are not edge cases to handle later; they are the
reason for a good deal of the code's shape.

## Architecture

A UNA app is two processes that share nothing but messages.

```
        HybridXRaceService.elf                 HybridXRaceGUI.elf
   ┌──────────────────────────────┐      ┌───────────────────────────┐
   │ RaceModel  (pure C++)        │      │ Model     (GUI-side state)│
   │ ActivityWriter  -> FIT       │ <──> │ ScreenManager             │
   │ AppConfig, Settings          │ msgs │ 9 screens, LVGL widgets   │
   │ sensors, haptics, backlight  │      │                           │
   └──────────────────────────────┘      └───────────────────────────┘
            owns the race                      owns the pixels
```

The service owns the truth. The GUI owns nothing but a cached copy of what it
was last told, and never computes a race time of its own.

**The message set** (`Software/Libs/Header/Commands.hpp`) follows two rules the
SDK's own apps do not:

- **Every struct is `static_assert`ed against the 256-byte pool block.** A
  message that outgrows the pool fails the build instead of vanishing at
  runtime. The largest today is `SummaryPage` at 140 bytes.
- **The ID space is split by direction.** Service→GUI below `0x80`, GUI→service
  at `0x80` and above. A message sent the wrong way is then obvious in a log
  rather than a mystery.

```
service -> GUI                       GUI -> service
0x01 SETTINGS_UPDATE                 0x80 SETTINGS_SAVE
0x02 LOCAL_TIME                      0x81 RACE_START
0x03 BATTERY                         0x82 RACE_SPLIT
0x04 HR_UPDATE                       0x83 RACE_UNDO_SPLIT
0x05 RACE_STATE_UPDATE               0x84 RACE_PAUSE
0x06 RACE_DATA_UPDATE                0x85 RACE_RESUME
0x07 SPLIT_EVENT                     0x86 RACE_FINISH_EARLY
0x08 RACE_FINISHED                   0x87 RACE_UNDO_FINISH
0x09 SUMMARY_META                    0x88 RACE_SAVE
0x0A SUMMARY_PAGE                    0x89 RACE_DISCARD
0x0B ACCESSORY_STATUS                0x8A SUMMARY_REQUEST
```

The summary is **paged** — `SUMMARY_META` then a `SUMMARY_PAGE` per eight
segments — rather than passed as a pointer into service memory as RunLVGL does.
A pointer works today and breaks the moment the two processes stop sharing an
address space; 31 segments of summary also do not fit in one 256-byte message.

## Service

`Software/Libs/Header/Service.hpp`, `Sources/Service.cpp`. Adapted from the SDK's
Workout example.

### The race model

`RaceModel` (`Libs/Header/RaceModel.hpp`, `Sources/RaceModel.cpp`) is **pure
C++** — `<cstdint>` and `<cstddef>`, no SDK headers, no heap, no floats, no
`std::string`. That is what lets 78 host tests simulate a 90-minute race
instantly, on any machine, with no watch and no ARM toolchain.

It takes time as a parameter rather than reading a clock. Brief §7.4 requires a
split to be stamped at the button press, and the GUI runs at 10 Hz: a model that
read its own clock would re-time every split up to 100 ms late. So the GUI
stamps `sys.getTimeMs()` in the key handler and sends it; the service passes it
straight through.

The state machine is `Idle → Running ⇄ Paused → Finished → Saved | Discarded`.
Every illegal transition is a no-op returning `false`, never an assert — a
dropped or duplicated press must not be able to corrupt a race in progress.

All time arithmetic is unsigned subtraction, so the wrapping kernel clock needs
no special case.

**Undo** is the interesting part. Closing a segment banks its start, active time,
paused time and heart-rate sum/count/max; `reopenLast()` restores the original
start and folds the banked pause and heart-rate accumulators back into the live
ones. Sum and count merge losslessly, which an average would not. The
accumulators are cleared in `closeCurrent()` rather than in `openSegment()`,
because the finishing split closes a segment without opening another — leaving
them set made a later `undoFinish()` count that segment's pause and heart rate
twice. A host test caught it.

### Sensors

- `HEART_RATE_EX` for heart rate, which reports arbitrated, optical and external
  values with a trust level. The **FIT gate is `bpm > 20` and trust 1–3**; the
  live display is deliberately ungated, because the trust gate is about what
  gets recorded, not about what the athlete is allowed to see.
- `FUSION_RAW` at 100 Hz during a race, feeding an app-local
  `WristTiltDetector` for the backlight. The kernel's own `WRIST_MOTION` fires
  constantly during sleds, rowing and wall balls and is useless mid-race; it is
  used outside a race, where it is fine.

### Lifecycle

The kernel does **not** stop a service when its GUI closes —
`COMMAND_APP_NOTIF_GUI_STOP` is a notification, not an order. The SDK's activity
apps treat it as one anyway and exit about 500 ms later, losing an in-progress
FIT file until the next launch repairs it (`NOTES.md` 0.7).

HybridX Race does not. When the GUI goes away with a race outstanding, the
service stays resident and keeps timing. If the GUI has not come back after five
minutes (`skGuiGoneGraceMs`), the race is finished and saved and the service
exits. An athlete who glances at their watch face mid-race loses nothing; an
athlete who walks away does not leak a service.

## GUI

`Software/Apps/LVGL-GUI/`. LVGL v9.5, built from the SDK's RunLVGL example.

`Model` holds the GUI's cached state and is the only thing that talks to the
service. Screens read it and never message the service directly, with one
exception: `TrackScreen::onKey()` sends `RACE_SPLIT` with the press instant,
because that timestamp is only correct if it is taken in the key handler.

Screen switching goes through `ScreenManager`, which defers with
`lv_async_call`. A screen is never deleted inside its own event handler, and SDK
widgets — which own timers and animations keyed on the C++ object — are destroyed
before their parent LVGL objects.

Nine screen classes: the main wheel menu, settings, "on your marks", the race
face (which cycles a main and a status face), the split toast, the action menu,
hold-to-confirm, the result screen (finished, saved and discarded are one class
with three modes), and the paged summary.

### Idle and suspend

Menu screens exit the app on idle (30 s). The race, the action menu, the finished
screen and the summary never do. The base `onIdleTimeout()` is empty, so a screen
that wants no timeout simply does not override it — the safe default is the
do-nothing one, which is the opposite of RunLVGL, where ten menu screens got the
do-nothing behaviour by accident.

"On your marks" has no timeout on purpose: it is where an athlete stands waiting
for the gun, and thirty seconds is not long enough. That is also what allows the
main menu to have no exemptions.

A suspend during a hold-to-confirm cancels the hold rather than completing it.

### The screen-switch peak

`switchNow()` creates and loads the new screen before deleting the old one, so
**both widget trees are alive for the duration of a switch** and LVGL's pool has
to hold the largest such pair. Measured across the whole app, that peaks at
**91 % of the 40 KB pool**, which is set in the SDK's `lv_conf.h` and cannot be
enlarged by an app.

Three kilobytes of headroom on a device with no MMU is the app's tightest
margin, and every screen added later spends some of it. `NOTES.md` 5.6 has the
numbers and the two candidate fixes; neither is applied yet, because changing
how screens are torn down wants a watch to verify.

### The round display

The usable width is a chord, not 240 px, and it narrows sharply towards the
bottom: 240 px on the centre line, 208 px at y=180, 179 px at y=200, 159 px at
y=210. And what binds is a line's lowest pixel, not its baseline. Three separate
pieces of text were running off the edges before anyone worked this out. The
table is in `NOTES.md` 4.3 and every screen is laid out against it.
That is also why `Station` carries a `brief` name used only in the split list,
where a full station name plus a time does not fit at any font available.

## Data

### The FIT file

`ActivityWriter` (`Libs/Sources/ActivityWriter.cpp`) writes a standard FIT
activity through the SDK's `FitWriter`, with two departures.

**Laps are written in one batch at save time**, after every Record and
immediately before the Session. A lap is not final until the race is saved,
because it can still be undone. The SDK's own writer streams each lap as it
happens, so this ordering is exercised nowhere in the SDK — it was proven
against an independent decoder before anything depended on it
(`docs/experiments/batched_laps.cpp`).

**Developer fields carry the race structure**, so an importer can tell a run from
a sled push without parsing lap names:

| Field | On | Meaning |
|---|---|---|
| `segment_type` | lap | 0 run, 1 Roxzone in, 2 station, 3 Roxzone out |
| `round` | lap | 1–8, the real round number even in a half race |
| `station_id` | lap | 1–8, or 0 for a run or Roxzone |
| `race_format` | session | 0 full, 1 rounds 1–4, 2 rounds 5–8 |
| `roxzone_mode` | session | whether Roxzone was split out |
| `completed` | session | whether the race finished or was ended early |

**Distance is written from the format, not measured.** A run is however long the
sim is set to (1 km by default), each station carries the metres the format
states, and a Full race at the race distance totals 10 480 m.
Nothing on the watch measures it — there is no GPS and no foot pod — but without
it Garmin Connect and Strava show `--` for Distance, Pace and Moving Time, which
is what the first round of candidate files did (`NOTES.md` 5.9). Average speed
per lap and for the session is derived from it.

The laps also reference a **workout**: one workout for the race and one step per
planned segment, each step named from `RaceModel::name()` and pointed at by the
lap's `wkt_step_index`. That is the only way to label a lap — the FIT lap message
has no name field of its own, and neither Garmin nor Strava displays developer
fields. `wkt_step_name` is not in the SDK's `FitProfile.hpp`, so its field number
is written directly, taken from the published FIT data dictionary and verified by
decoding what we wrote (`NOTES.md` 5.11).

Calories are **not** written. The app has no source for them, and a fabricated
number in a FIT file is worse than an absent one. Both platforms therefore show
a blank calorie figure.

The session's `sport` and `sub_sport` are parameters of
`ActivityWriter::TrackData`, defaulting to training/generic. Which values make
Strava and Garmin label a race most usefully is decision D2; candidate files are
in `docs/experiments/fit-candidates/`.

### Settings

Two sources that must not fight:

- **On the watch**: the race format, chosen on the main menu, kept in the app's
  own settings file.
- **From the phone**: Roxzone splits, run length, split lock, vibrate on split
  and target finish, through `SDK::AppConfig`.

The AppConfig fields are declared twice — in C++ at
`Libs/Sources/AppConfigFields.cpp` and in `Resources/app-manifest.json` — because
the phone needs the declaration and the watch needs the table. They are kept in
step mechanically: `pack-store-zip.sh` runs the SDK's
`validate_app_config.py --check-bounds` against both files and refuses to build a
package if a default or a bound has drifted.

### Reference data

`Libs/Header/RaceData.hpp` is the only place the race format is defined — station
order, distances, reps, segment counts, label text, and the bounds on the
adjustable run length. A rule change is a one-file edit. Station weights are
deliberately absent: the app never shows them.

## Build

```
Software/Apps/HybridXRace-CMake/CMakeLists.txt   the watch build
Software/Libs/libs.cmake                         shared sources
Software/Apps/LVGL-GUI/lvgl-gui.cmake            GUI sources
Software/Apps/LVGL-GUI/simulator/CMakeLists.txt  the PC build
Tests/Host/CMakeLists.txt                        host tests
```

Everything resolves through `$UNA_SDK`; the app lives entirely outside the SDK
checkout and never modifies it.

The watch build produces two ELFs, merges them and emits
`Output/HybridXRace_<version>.uapp`. `<version>` comes from a git tag via the
SDK's `una-version.sh`, which is invoked with the prefix `apps-` — so a release
needs an `apps-vX.Y.Z` tag, and a plain `vX.Y.Z` leaves the build at
`0.0.0-dev`.

Build the watch target **and** the simulator after every change. Their source
lists are maintained separately and a file present in one can be missing from the
other.

## Simulator

`Software/Apps/LVGL-GUI/simulator/` builds the whole app — service included — as
one native binary against the SDK's kernel test doubles. Keys 1–4 are L1, L2, R1
and R2.

It is close to the watch but not identical, in ways worth knowing:

- **Messages are heap-allocated**, so the 256-byte pool limit does not exist
  there. A message that is silently dropped on the watch works perfectly in the
  simulator. Hence the compile-time assertion.
- **There is no screenshot capability** (`LV_USE_SNAPSHOT` is 0), so
  `docs/experiments/capture_screens.sh` drives it from outside with Xvfb,
  xdotool and ImageMagick. It renders at 2×, so captures are 480×480 for a
  240×240 watch.
- Sensors are simulated, so heart rate is plausible rather than real, and the
  wrist-tilt backlight path cannot be exercised at all.

What the simulator **can** prove is everything logical: the state machine, the
message flow, the FIT output, the screen layouts and the idle rules. What it
cannot prove is on `ON_WATCH_TESTS.md`.
