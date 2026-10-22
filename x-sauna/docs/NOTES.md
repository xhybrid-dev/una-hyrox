# X Sauna: notes

An on-watch sauna and cold-plunge tracker. Start a session as **Sauna → Cold
plunge** or **Just sauna**, press a lap button to swap phase, and record time,
heart rate and temperature. Based on a workout: the session becomes an activity
(a FIT file that syncs to UNA and on to Strava), not a utility. Independent of
the other apps.
Started from Knitting's layout (itself from the SDK's Waypoint tutorial, LVGL).

## Why this app (20 October 2026)

Jon's request: "a sauna tracking app, probably based on workout. Two options:
Sauna → Cold plunge, or Just sauna. Lap buttons to swap. Time, heart rate and
temperature recording." He asked for further ideas; of those offered he chose
**HR recovery** and a **Rest phase**.

## Decisions

Planned with Jon on 20 October 2026 (plan approved for SA0 only):

- **Name: X Sauna** (working name; Jon can change it, as Knitting's changed).
  Folder `x-sauna/`, `X Sauna` on the watch (7 bytes), `XSauna` as the build and
  file name, tags `sauna-vX.Y.Z`. The development APP_ID is the first 16 hex of
  md5("XSauna"), `400AE282F6C2C161`, the convention Knitting and X Weigh-in
  followed (checked: md5("XWeighIn") gives their development ID). The portal
  issues the real one at SA4.
- **In the first version:** the two session types and the lap swap; time, heart
  rate and temperature recorded to a FIT file; **HR recovery** (how far heart
  rate falls in the minute after a phase change); an optional **Rest phase**
  (Sauna → Plunge → Rest → ...).
- **Offered and not chosen:** target times with a buzz and a heart-rate ceiling
  from the phone settings; a typed-in sauna temperature. The second comes back
  at Gate 0 if the sensor turns out to be no use in the heat (below).
- **Gate SA0 result (Jon, 21 October 2026, after asking UNA and testing):** the
  temperature reads **well above room temperature but below body temperature**:
  a temperature inside the watch, with a long lag, not the air. **Decision: use
  it only as a rough guide** to sauna versus ambient versus cold plunge, never
  as a monitored temperature. Nothing in the app presents it as degrees. The
  test was room and wrist only; there is **no sauna or plunge data yet**, so no
  threshold is set from evidence (see SA1).
- **Open for Jon:** whether the watch is rated for a sauna and for water (below).
  Nothing in the SDK says either way.

## What the platform gives us (20 October 2026)

- **Heart rate:** `HEART_RATE` (bpm and a trust level) and `HEART_RATE_EX` exist.
  HybridX Race already records HR a second into a FIT file with laps, developer
  fields and crash recovery (`hybridx-race/Software/Libs/Header/ActivityWriter.hpp`).
- **Temperature: the risk.** `SDK::Sensor::Type::AMBIENT_TEMPERATURE` (0x70),
  parser `SensorDataParserTemperature.hpp`: one float, "°C (or device-specific
  units)", `-273.15` when invalid. `Docs/SensorsLayer.md` marks it `TEMP (float
  °C?)`. Nothing says whether it is the air around the watch or a body or board
  reading, how quickly it responds, or where it tops out. HybridX Race's
  `ROADMAP.md` section 2 reached the same point and never connected it. Sauna
  air is typically 80 to 100 degrees, far beyond what a wearable is designed for.
- **The simulator cannot supply it.** `Docs/Simulator.md` lists GPS, heart rate,
  battery, pressure, the IMU and steps. The service makes a reading up under
  `SIMULATOR` (`Core/Header/SimTemp.hpp`) and the screen says "made up".
- **FIT:** `FitProfile.hpp` has no temperature field on `Record`. The public
  profile has `Record.temperature` as field 13, `sint8`, whole degrees C (from
  Race ROADMAP 2.2: **not yet checked by a decode round trip**, which is SA2's
  job). No sauna sport is known; the safe default is `training`/`generic`, as
  Race's NOTES 5.12 settled (Garmin ignored the other sub-sport candidates). Never
  invent FIT numbers: look one up in the profile tables and decode it back.
- **Buttons:** four. R2 is back. A long press has to be derived from press and
  release codes (`HoldTimer.hpp` in Knitting). The GUI runs at 10 frames a second;
  the face is round.
- **Streak interplay:** HybridX Streak counts every activity the watch records,
  so a sauna session will count towards the streak once it is an activity.
- **Appending to a file that already exists works on the watch** (X Weigh-in W2),
  which the probe's log relies on.

## Phases

| Phase | Work | Gate |
|---|---|---|
| **SA0** | Identity, scaffold, CI build, and a **probe screen** that logs temperature and heart rate once a second to a CSV | Jon runs the staged test and sends the CSV: is the sensor there, what unit, ambient or skin, how it behaves when hot, wet or cold |
| SA1 | The pure core with an injected clock, host-tested: session model, phases, laps, HR recovery, heat hint, text. **Done (22 October 2026)** | Host tests pass; Just-sauna rule settled |
| SA2 | The service and the FIT file: HR and temperature a second, laps with developer fields, crash recovery; APP_TYPE moves to Activity. **Done (22 October 2026)**, on branch `claude/x-sauna-sa2` | A FIT file decodes (done here); the UNA app, Garmin and Strava take it (Jon) |
| SA3 | The LVGL screens: start, live, summary | Jon uses it on the wrist in a real session |
| SA4 | Store release: icon, package, `SUBMISSION.md`, 1.0.0 | Installed from the store |

Outline of later phases (refined at each gate):

- **SA1:** `SessionModel` with an injected clock and unsigned deltas for the
  wrapping millisecond clock; modes Sauna+Plunge and Just sauna, optional Rest;
  per-phase laps with HR average and maximum and temperature average and
  maximum; HR recovery as the drop from the heart rate at the end of a phase to
  the heart rate 60 s later, "no result" when HR is missing; integer formatting.
- **SA2:** a trimmed copy of Race's `ActivityWriter` (Race's carries race-specific
  fields; the apps stay independent). HR and temperature per record, laps with
  developer fields for phase type, round and HR recovery. Fixed-size messages.
- **SA3:** R1 swaps phase (the main action); L1 pause and resume; a way to
  insert Rest; R2 exits with a hold-to-stop confirm. Final map to match Race's
  conventions once read. Phase colours: sauna warm, plunge cool, rest neutral.
  The summary shows rounds, time in each phase, peak temperature, peak HR and
  best recovery.

## SA0: the heat probe (20 October 2026)

> **Superseded by SA2 on the `claude/x-sauna-sa2` branch**, which replaces the
> probe with the real service. The probe build stays on `claude/x-sauna-sa0`
> (Jon is testing it in a real sauna). This section and the next two describe it.


### Layout

- `Software/Libs/Core` (pure C++, host-tested; header-only where the GUI uses it):
  - `FixedPoint.hpp`: a float as hundredths and back to text, with no `%f`;
  - `ProbeStats.hpp`: last, lowest, highest, count and the gaps between samples
    (the sensor's own timestamps, unsigned differences);
  - `ProbeSnapshot.hpp`: the readings as the screen shows them;
  - `ProbeCsv.hpp` and `Sources/ProbeCsv.cpp`: the log's lines, and a counter
    for the "# run" lines so each run can be numbered;
  - `SimTemp.hpp`: the made-up temperature for the simulator.
- `Software/Libs/App`: `Service` (sensors, log, messages) and `Commands.hpp`
  (one Service → GUI message, two GUI → Service; the update is 96 bytes on a
  64-bit host, less on the watch, and has a `static_assert` against the 256-byte
  pool block).
- `Software/Apps/LVGL-GUI`: `ProbeScreen`, the model, the simulator.
  Fonts are Knitting's generated Poppins files, unchanged.
- `Software/Apps/Sauna-CMake`: the watch build. `APP_TYPE` is `Utility` for now.
- `Resources/make_icons.py`: a placeholder icon (steam over hot stones).
- CI: `watch-builds.yml` builds it (`sauna`), collects the `.uapp`, and runs the
  host tests.

### What the probe does

It connects `AMBIENT_TEMPERATURE` and `HEART_RATE` at their default period and
latency, as the SDK's Sensors tutorial does (the point is to see what they do
by themselves), for as long as the screen is open and for at most 45 minutes.
Once a second it writes a row to `heat-probe.csv` in the app's folder
(`Apps/XSauna/` over USB), in batches of five seconds, so a watch that gets too
hot to carry on has still saved everything but the last few seconds.

The log (`Core/Header/ProbeCsv.hpp` has the full column notes):

```
ms,t_s,mark,temp_x100,temp_n,temp_bad,temp_fields,temp_gap_ms,hr,trust_x100,hr_n
# run 1 2026-10-20 19:28:21
# sensors: temperature connect failed, heart rate no data
1001,1,0,2200,1,0,1,0,,,0
2003,2,0,2200,2,0,1,1002,62,300,1
```

- `temp_x100` is the sensor's number times 100, **in whatever unit the driver
  uses**: the probe assumes nothing. Blank until a valid sample.
- `temp_bad` counts samples the parser rejected, and `temp_fields` how many
  fields the driver sends (the parser wants exactly one).
- `temp_gap_ms` is the time between the last two temperature samples, from the
  sensor's own timestamps: how often it really reports.
- `mark` counts presses of R1: the way to say "I have just gone in".
- Each run starts with a `# run N <local time>` line. One file holds every run.
  Read it in a spreadsheet or pandas with comment lines skipped (`#`).

Screen (simulator frame; on a watch "made up" reads "temperature sensor, raw"):

![The heat probe in the simulator](screens/sa0-3-climbing.png)

| Button | Action |
|---|---|
| R1 | Next phase, recorded in the log (a short buzz); see "The probe, second version" |
| L1 | Start the lowest and highest readings again |
| R2 | Exit |

Colours: the big temperature is white while it is being read, amber while
waiting for the first sample and red if the sensor is not offered; the heart
rate is green once samples arrive. "NO LOG" in red means a write to the file
failed.

### SA0 verified (20 October 2026)

- **Host tests:** 41 pass, no warnings (`FixedPointTest`, `ProbeStatsTest`,
  `ProbeCsvTest`, `SimTempTest`): rounding and saturation, text at every buffer
  size with a guard byte, the wrapping clock, run counting at every chunk size.
- **Simulator** (`docs/experiments/capture_screens.sh`, about 90 seconds):
  - a log with a header, a run line, a sensors note, a row a second, the two
    markers in the `mark` column (first at 11 s) and a closing note;
  - the sensors note says "temperature connect failed" in the simulator, as it
    has no temperature sensor, and the screen shows the made-up curve in blue;
  - R2 exits and takes the service with it.
- **Watch target, local compile check:** Ubuntu's `arm-none-eabi-gcc` with Race's
  syscall stubs (recipe in Trail Quest's NOTES, "Toolchain notes"), both
  processes link, `XSauna_0.0.0-dev.uapp` is 194,728 bytes, no warnings from our
  code. **Not installable**; the watch copy comes from CI.
- **Not verified:** nothing has run on a watch. CI has not run on this branch.
  Whether `AMBIENT_TEMPERATURE` can be connected on the watch at all (the
  simulator says nothing about that). That is what Gate 0 is for.

## SA1: the session core (22 October 2026)

Pure C++ in `Software/Libs/Core/Header`, header-only (the GUI will use it), no
heap, integer text, the clock passed in. Nothing in the service or GUI uses it
yet: that is SA2 and SA3. The probe app is unchanged.

- **`SessionModel.hpp`**: a session as laps. `start / lap / pause / resume / stop`,
  `addSample(nowMs, hr, tempX100)` once a second, `summary()`.
  - Phase order on a lap press: Sauna + Plunge: Sauna → Plunge → (Rest) → Sauna;
    **Just sauna: Sauna ↔ Rest, always** (Jon, 22 October 2026: for those
    without a plunge; the Rest setting is ignored in that mode). The round counts
    Sauna phases.
  - Active time excludes pauses; elapsed is wall time and freezes at the stop.
    Differences are unsigned, tested across the 32-bit wrap.
  - 64 laps at most: the next press returns `LapResult::Full` and changes
    nothing. The model is about 4 KB (3.7 KB on the watch's CPU), so the service
    must keep it in static storage; `start()` resets in place rather than
    building a second copy.
  - HR average is over valid samples only (0 means "no reading", never a zero in
    the mean). Lap statistics count only while running.
  - The model itself does not use heart-rate trust. (SA2 applies HybridX Race's
    gate to what reaches it: `HEART_RATE_EX`, a real bpm above 20 with a trust
    level of 1 to 3, and not older than 5 s.)
- **`HrRecovery.hpp`**: the heart rate when a phase ended minus the heart rate
  from 60 s later (the first valid sample in the next 5 s). A phase's end rate
  must be under 5 s old. Several can be pending at once (a 30 s plunge ends
  before the sauna's minute is up). No result when HR is missing, late, or the
  session stops first. Positive means it fell.
- **`HeatHint.hpp`**: cooler / similar / warmer than the session's first minute,
  and rising / steady / falling over about a minute, from a smoothed reading,
  with hysteresis. **Off by default and every threshold is an untuned
  placeholder**: there is no sauna or plunge reading to set them from. It only
  suggests; the lap button is the only thing that changes phase. The tests use
  invented curves to check the logic and say nothing about the real sensor.
- **`SessionText.hpp`**: `mm:ss` / `h:mm:ss`, bpm, recovery as the heart rate's
  change (`-23`, `+4`, `..` waiting, `--` none), and the temperature change as
  a signed one-decimal number. Buffer-safe, tested at every size with a guard byte.

### SA1 verified (22 October 2026)

- **Host tests:** 94 pass, no warnings (53 new: model, recovery, hint, text).
  Two test bugs were found on the way and fixed: a wrong expected value, and a
  loop that was not wrap-safe, which had left a clock-wrap case feeding no
  samples. The wrap cases now run for real.
- **Simulator and watch target:** both still build, unchanged. The new headers
  also compile for Cortex-M33 (`arm-none-eabi-g++ -Wall -Wextra -Wpedantic`).
- **Not verified:** nothing here has run on a watch or met real data. The hint's
  usefulness is unknown until sauna and plunge logs exist.

### For Jon at Gate SA1

- ~~Confirm the Just-sauna rule~~ Settled (22 October 2026): Sauna ↔ Rest.
- **Sauna and plunge logs.** When convenient, and only if UNA's limits allow it,
  run the probe through a real sauna and plunge and send `heat-probe.csv` with
  the markers. That sets the hint's thresholds, and says whether the hint is
  worth showing at all. SA2 does not need it.

## The probe, second version: judging the heat hint (22 October 2026)

Jon asked to set up sauna and plunge monitoring "to see how it performs". The
probe now doubles as the test rig for the heat hint (SA1), still with no claim
that the number is a temperature.

- **R1 steps a phase** (the screen's title; the log's `phase` column): AMBIENT at
  the start, then SAUNA, PLUNGE, REST, SAUNA ... Press it as you do each thing.
  It buzzes. (The probe cannot tell where you are; what you pressed is the
  reference the hint is judged against.)
- **The hint runs live** with its untuned placeholder thresholds, and under the
  big number says how the reading compares with the first minute ("+5.9 warmer,
  rising"). **Start the probe in the room you will treat as ambient and give it
  a full minute before the first R1**: that minute is the baseline.
- **New log columns:** `phase`, `hint_delta_x100`, `hint_level`, `hint_trend`
  (full notes in `Core/Header/ProbeCsv.hpp`). The column header is now written at
  the start of every run, after its `# run` line, because the columns grew: a
  `heat-probe.csv` left over from the first version has the old shape above the
  new runs. Delete or rename the old file first if that is untidy.
- The status line says `lap`, not `mark`; the log's `mark` column counts R1
  presses, as before.
- Verified in the simulator (capture script) and by 94 host tests, including the
  CSV's new columns. The simulator's temperature is made up, so what the hint
  says there tells you nothing about the sensor.

### What Jon runs (only if UNA's limits allow sauna and water)

1. Install the new CI build as before (branch `claude/x-sauna-sa0`).
2. Open X Sauna in the room, and leave it for a full minute.
3. R1 as you enter the sauna (SAUNA). R1 as you leave it for the plunge (PLUNGE),
   R1 when you get out (REST), and R1 again for the next sauna round.
   Several rounds are better than one.
4. R2 to finish, then send `Apps/XSauna/heat-probe.csv`, and anything odd you
   noticed (the watch getting hot, the screen, how it felt).

What I will look at: how far and how fast the reading moves in each phase, how
it differs between sauna, plunge and rest, how long it lags, whether the hint's
verdicts line up with what you pressed, and whether HR holds in the heat and the
cold. From that I set the thresholds, or recommend dropping the hint.

## SA2: the service and the FIT file (22 October 2026)

Branch `claude/x-sauna-sa2`, cut from the SA0 branch so the probe build Jon is
testing stays as it is. The service now runs a real session and writes an
activity file. The probe's CSV code and screen are gone (they stay in git history
on the SA0 branch); the FIT file is the data log.

### What it does

- **Service** (`Software/Libs/App`): owns the `SessionModel` (static storage, about
  4 KB) and the sensors: `HEART_RATE_EX` gated as HybridX Race gates what goes in
  its file, and `AMBIENT_TEMPERATURE`. Once a second while running it writes a
  FIT record and feeds the model. Messages from the screen: start (mode, rest),
  lap, pause, resume, stop (save or discard). To the screen: the session once a
  second and on every change, and a summary when it ends. All fixed size, with
  `static_assert`s against the 256-byte pool block.
- **If the screen goes away mid-session** the service keeps recording, and saves
  and exits if the screen stays away for 5 minutes (`kGuiGoneGraceMs`, Race's
  figure). If the kernel stops the app it saves what it has with `completed = 0`.
  A boot that finds an interrupted recording finishes the file first.
  **Not verified in the simulator**, which cannot close the screen without ending
  the whole process; the logic is Race's, proven on a watch there.
- **`ActivityWriter`**: a trimmed copy of Race's (same layout, crash marker, flush
  every 30 s, `Activity/YYYYMM/activity_<time>.fit`, a `.json` sidecar with
  `activity_type` "sauna", `CommandAppNewActivity` to register it). Laps and the
  session are written **at stop**, because a lap's HR recovery is known only a
  minute after it ends. So a crash recovers the per-second records, **not the
  laps or the session**. Accepted.
- **Pure core** (`Core/Header`): `SessionFit.hpp` turns the finished model into the
  lap and session values; `LapTiming.hpp` (Race's) makes laps chain and add up
  exactly; per-lap paused time is now kept by `SessionModel`; `HoldTimer.hpp`
  (Knitting's) times the R2 hold.

### What is in the file

| Message | Standard fields | Developer fields (this app's own numbers) |
|---|---|---|
| record, every second | timestamp, heart_rate (invalid when none, never 0) | `watch_temp_x100` (sint16) |
| lap, one per phase | times, avg and max HR, lap_trigger manual | `phase` (0 sauna, 1 plunge, 2 rest), `round`, `hr_recovery_bpm` (sint16, invalid when none) |
| session | times, num_laps, avg and max HR, sport Training (10), sub_sport Generic (0), `sport_profile_name` "Sauna" | `mode`, `rest_enabled`, `completed` |
| event | timer start and stop, in pairs (a pause is a stop and a start) | |

- **The temperature is a developer field, not the standard record temperature
  (field 13).** Gate SA0 found it is a lagging temperature inside the watch;
  writing it as field 13 would tell Garmin and Strava it is the air. Developer
  fields are not shown by either, but they are in the file for us. No unit is
  claimed (`watch_temp_x100` is the sensor's number times 100).
- **Sport: Training / Generic**, as Race settled (its NOTES 5.12). The profile has
  no sauna sport that I can source. `sport_profile_name` "Sauna" is Race 5.28's
  experiment: **whether the UNA app or Strava honours it is unknown** (a Gate
  question for Jon).
- Heart-rate recovery (SA1) is written per lap.

### The interim screen

`SessionScreen`: text only, four views, and **SA3 replaces it with the real
design**. Menu (R1 Sauna + plunge, L1 Just sauna, L2 Rest on or off); live (the
phase, the lap's time, heart rate, total time and round, the previous lap's
recovery; R1 next phase, L1 pause or resume, **hold R2** to stop); the stop
question (R1 save, L1 discard, R2 back); the summary (rounds, time in each phase,
heart rate, best recovery). The temperature is not shown. "NO LOG" appears in red
if a write to the file fails.

![Menu, sauna, plunge](screens/sa2-3-sauna.png)

### SA2 verified (22 October 2026)

- **Host tests:** 99 pass, no warnings (new: lap timing, per-lap paused time, the
  FIT mapping, the hold timer).
- **The file, decoded by an independent reader** (`docs/experiments/fit_check.sh`:
  builds `Tests/Host/tools/make_fit.cpp`, which writes sessions through the app's
  own `ActivityWriter` and `SessionModel`, then `fitdecode` checks each). Four
  scenarios, all passing: a full session with a pause; Just sauna; a session with
  no heart rate (invalid, not zero); and a crash, where the records survive a
  recovery and no laps or session exist. Checked: header and CRC, identity,
  developer fields declared and typed, timer events in pairs, a record a second
  with a gap only at the pause, the temperature on every record, laps that chain
  and sum to the session, phase, round and recovery values, the session's sport and
  name.
- **A whole session in the simulator** (`docs/experiments/capture_screens.sh`, about
  50 s): keys drive menu, start, two laps, a pause, the R2 hold, save; a real
  `.fit` and `.json` land in `Software/Output/Activity/`, and the same decoder
  passes on it. R2 exits.
- **Watch target, local compile check:** both processes link, `XSauna_0.0.0-dev.uapp`
  263,600 bytes, no warnings from our code. **Not installable**; CI makes the
  installable one.
- **Not verified:** anything on a watch. Whether the UNA app registers the file and
  shows it, and what Garmin and Strava make of it. The 60 s HR-recovery wait is not
  exercised in the simulator (its session is 25 s); the host tests and the file
  checks cover it. Closing the screen mid-session (above). CI has not run on this
  branch.

### For Jon at Gate SA2

1. **Install the build from the `claude/x-sauna-sa2` CI run** (as for the probe).
   Note it **replaces the probe**: to keep testing the probe, use the SA0 branch's
   build instead (same app name and folder, so copying one into `Apps/XSauna/` replaces the other).
2. **A dry session in the room** with Sauna + plunge: press R1 a few times, pause
   and resume once, hold R2, save. Open the UNA app: does the activity appear?
   Then upload it to Strava and Garmin if you can: how is it titled, are the laps
   and heart rate right, does "Sauna" show anywhere?
3. **Decide the sport label** if "Training" reads badly. I will look a value up in
   the public profile tables rather than guess.
4. The sauna and plunge probe results are still wanted for the heat hint.

## Gate SA0: what Jon tests on the watch

Do these in order and stop at the first surprise. Steps 1 and 2 need nothing
special; steps 4 and 5 put the watch in the heat and in water, **so do them only
if UNA's published limits for this watch allow it**. The SDK documents neither,
and I cannot tell you.

1. **Install.**
   - On GitHub, Actions, take the newest green **Watch builds** run on the branch
     `claude/x-sauna-sa0` and download **watch-apps**.
   - Over USB, create `Apps/XSauna/` on the watch and copy `XSauna_*.uapp` into it.
   - Eject safely, then power-cycle the watch (as for the other apps).
2. **On a table, indoors, ten minutes.** Open X Sauna, leave it, note the room
   temperature from a thermometer you trust.
   - Is the big number a plausible room temperature? About 20 to 25 means
     degrees C; about 295 would be kelvin; about 70 would be Fahrenheit.
   - Is "no temperature sensor" shown? Then say so and stop: that is an answer.
3. **On the wrist, indoors, ten minutes.** Press R1 as you put it on. Does the
   number climb towards body temperature (30 or more, a skin or board
   reading) or stay near the room's (an ambient one)? Does heart rate show, and
   what does "trust" say?
4. **Optional, the sauna (only if rated for it).** Lowest bench, ten minutes at
   most. R1 when you go in and when you come out. Watch whether the number keeps
   rising or stops at a ceiling, and how long it takes to settle.
5. **Optional, a cold plunge (only if rated for water).** R1 in and out. Heart
   rate in cold water is a separate question from temperature: does it hold?
6. **Send me `Apps/XSauna/heat-probe.csv`**, and what each marker meant.

What the answers decide:
- **A real air temperature that reads true in the heat:** record it as it is.
- **A body or board temperature, or one that tops out low:** record it labelled
  "watch temp", or drop it.
- **No use at sauna heat:** the typed-in sauna temperature comes back as a
  question for you.
- **No sensor:** the same, and temperature in the FIT file goes.
