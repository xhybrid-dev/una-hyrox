# X Velocity: notes

A gym tool on the wrist: rep counting, bar speed (velocity-based training) and
a jump test. Independent of the other apps. The brief and plan are in
`X_VELOCITY_BRIEF.md`; the Gate V0 plan is in `PROBE.md`.

## Why this app (12 October 2026)

An ideas pass looked for the most original app the UNA Watch could run. Its
top three were Velocity (bar speed and a jump test), Heartbeat (beat-by-beat
haptics and heart-rate variability) and Hunt (a GPS treasure hunt played by
vibration). Jon picked Velocity because he wants the watch to be a useful gym
and strength tool, and asked for **accurate rep counting** to be part of it.
The brief puts the counter first; bar speed and the jump test build on the
same motion pipeline.

## Desk study: what the SDK says (12 October 2026)

Every claim below cites the SDK at the pinned commit (`una-sdk/`) or our own
notes. Nothing here has been run on a watch yet; Gate V0 is where it is tested.

### The motion sensor

- **The IMU is a BMI270** (`una-sdk/Docs/development-workflow.md:436`, the
  driver diagram). The BMI270 itself supports several accelerometer and gyro
  ranges; **which ones the kernel sets is not documented**.
- **100 Hz six-axis is the SDK's own practice.** RunLVGL, Running, Treadmill
  and Workout subscribe to `FUSION_RAW` at `1000 / 100` ms with 100 ms latency
  (`Examples/Apps/RunLVGL/Software/Libs/Header/Service.hpp:46`,
  `.../Sources/Service.cpp:75`; Workout: `Service.hpp:38`, `Service.cpp:48`).
- **`FUSION_RAW` is six `int16` values with no scale given**: accelerometer
  X, Y, Z then gyro X, Y, Z (`Libs/Header/SDK/SensorLayer/DataParsers/SensorDataParserFusionRaw.hpp`).
  The SDK's own wrist-tilt detector works in "raw LSB units (not converted to
  physical units)" throughout
  (`Examples/Apps/Workout/Software/Libs/Header/WristTiltDetector.hpp:28`).
  So the scale has to be measured: the probe's calibration (U1).
- **`ACCELEROMETER` is documented in g; `GYROSCOPE` has no unit**
  (`Docs/SensorsLayer.md:18`, `:20`). The probe records `ACCELEROMETER` next to
  `FUSION_RAW` during calibration as a direct check of the accelerometer scale.
- **`GESTURE_RECOGNITION` and `HEART_BEAT` have no parser**
  (`Docs/SensorsLayer.md:24`, `:36`). Not needed here.

### Output and input

- **Haptics:** 21 named effects and sequences of up to 8 steps, each an effect
  with up to 3 loops or a pause of up to 1,270 ms in 10 ms steps
  (`Libs/Header/SDK/Interfaces/IVibro.hpp:15-46`, `:63`). Enough for a rep tick,
  a "set slowed" pattern and a "rest over" pattern that feel different.
- **Buzzer:** duration and three levels only, no pitch, up to 10 notes
  (`Libs/Header/SDK/Interfaces/IBuzzer.hpp:17-23`).
- **Buttons:** four, L1 up, R1 select, L2 down, R2 back, with click, long press
  and holds (`Libs/Header/SDK/Messages/CommandMessages.hpp:519-548`). No touch
  screen.
- **Display:** 240 x 240 (Race NOTES, the RunLVGL run at line 134), 64 colours
  (Intervals NOTES, line 518).

### Recording

- **The SDK's FIT profile has no strength-training set message**: the message
  list is file id, session, lap, record, event, workout, workout step,
  activity, field description and developer data id
  (`Libs/Header/SDK/Fit/FitProfile.hpp:28-39`). Sports are generic, running,
  cycling, training, walking and hiking; there's no strength sub-sport
  (`FitProfile.hpp:46-48`). **Developer fields are supported** by the writer
  (`Libs/Header/SDK/Fit/FitWriter.hpp:21`, `:51`, `:101`).
  - **Consequence:** v1 writes a lap per set, with reps, weight, speeds and
    velocity loss as developer fields. Adding the FIT set message would mean
    taking numbers from outside the SDK, which the standing rules forbid
    without asking; it is a question for UNA (brief §12, Q4).
- **Shared storage:** `Apps/SharedData/` is the one folder apps share (Streak
  NOTES; Weigh-in NOTES line 77). The session log keeps a copy there.

### The simulator

- Its IMU models are a step counter, running cadence and wrist motion
  (`Libs/Header/SDK/Simulator/Components/Sensors/IMU/`); none gives raw
  accelerometer and gyro samples. V2 adds a source that replays recorded
  `FUSION_RAW`.

### Related work in this repo

- **Race's roadmap has IMU rep counting for wall balls and burpees as v2**, "a
  research project; needs recorded training data"
  (`hybridx-race/docs/HYBRIDX_RACE_BRIEF.md:550`). Velocity's core and its
  recorded data are built so that Race can reuse them.

## Open questions

For UNA (brief §12):
1. the BMI270 ranges and the `FUSION_RAW` scale; whether an app can ask for
   200 Hz;
2. the gyro's units;
3. haptics during motion capture;
4. FIT strength sets;
5. flash space for logs.

For Jon: the decisions in brief §11.
