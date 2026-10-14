# X Velocity for UNA Watch: Brief and Plan

**Owner:** Jon
**Status:** Brief only (12 October 2026). Nothing is built yet. Next: Jon's
decisions (section 11), then V0: build the probe and record one gym session.
**Read with:** `NOTES.md` (the desk study, with SDK citations) and `PROBE.md`
(the Gate V0 plan and Jon's session protocol).

---

## 1. The idea

A gym tool on the wrist. You pick the exercise and lift. The watch counts every
rep, measures how fast the bar moved on each one, and tells you, without your
having to look, when the set has slowed enough to stop.

Three things in one app:

1. **Rep counting you can trust.** Live, with a tick on the wrist per rep, and
   a final count at the end of the set that you can fix with one press.
2. **Bar speed (velocity-based training).** The average speed of the lifting
   phase of each rep, and how much the set has slowed since its fastest rep.
   A buzz when it passes your cut-off (say 20 %), so athletes stop on fatigue,
   not on a number. Gyms pay hundreds of pounds for bar-mounted sensors that do
   this; here it's the watch you already wear.
3. **A 30-second jump test.** Three countermovement jumps with hands on hips.
   The watch times each flight and works out jump height: a daily check of how
   fresh the legs are, tracked against your own baseline.

It came out of the ideas pass on 12 October 2026 (the "Velocity" proposal).
Jon asked for accurate rep counting to be part of it, so that it's a useful
everyday strength tool and not only a speed meter. It is separate from the
other apps, like Trail and Weigh-in. The rep counter is built as a shared core,
so HybridX Race can use it later for wall balls and burpees (Race brief §16,
v2).

## 2. What it does (v1)

| # | Feature | Notes |
|---|---|---|
| F1 | **Exercise list** on the watch; the last one used comes first | A fixed built-in list (section 6), plus "Other" (logged, not counted) |
| F2 | **Live rep count** with a haptic tick per rep | Within about half a second of each rep's finish (target) |
| F3 | **Final count** at the end of the set, from a second pass over the whole set | Replaces the live count if they differ. See section 3 |
| F4 | **One-press fix:** L1/L2 adjust the count on the set summary, R1 accepts | Every fix is saved with the set's motion summary, so misses become test data |
| F5 | **Automatic sets and rest:** a set starts with the first real rep and ends after a few seconds of stillness; the rest timer starts by itself | R1 starts and ends a set by hand for anyone who prefers it |
| F6 | **Bar speed per rep** in m/s for the lifts that support it | Mean speed of the lifting phase; peak speed for cleans and swings. See section 4 |
| F7 | **Velocity loss:** the drop from the set's fastest rep, as a bar and a number | A distinct buzz when it passes the cut-off. The cut-off is a setting, and off by default |
| F8 | **Weight per set** on a wheel, remembered from the last set | kg (decision 6). Optional: a set can be saved without one |
| F9 | **Set summary:** reps, weight, best and last speed, velocity loss | Shown during the rest, with heart rate coming down |
| F10 | **Session summary and a FIT activity:** one lap per set, rest laps between | Reps, weight, speeds and velocity loss as developer fields. See section 7 |
| F11 | **Jump test:** three jumps, height of each, best, and today against your baseline | Its own short session; history kept on the watch |
| F12 | **Session log** as a plain file in the app's folder | For HybridX to read later. A copy in `Apps/SharedData/`, as Weigh-in does |

**Later (P1), not v1:**
- planned sessions from a JSON file in `Workouts/` (exercise, sets, reps,
  load, cut-off), as Intervals does, and from the phone later;
- a load-velocity profile and an estimated 1RM. This needs each athlete's
  minimum velocity threshold from the coach: no built-in numbers;
- target speed zones set by the coach;
- the exercise recognised automatically;
- HYROX station rep counts in Race.

**Never, on this hardware:**
- counting or speed for lifts where the watch hand doesn't move (section 6);
- form analysis or bar path;
- speed to laboratory accuracy. The wrist isn't the bar. Gate V0 measures how
  close it gets.

## 3. Rep counting: how it gets accurate

Counting from the wrist is a known, solvable problem when the app knows the
exercise. Research prototypes have done it with an arm-worn sensor (for
example Microsoft Research's RecoFit, 2014). What makes it *trustworthy* in
use is the design around the detector, and that's where most of this section
goes.

1. **You tell it the exercise.** One pick, remembered between sets. Knowing
   the lift removes the hardest problem, which is guessing what you're doing.
   Each exercise has a profile: which motion to follow, the shortest a rep can
   take, and how big a rep must be. Profiles are tuned on recorded sets, never
   guessed.
2. **It counts movement, not shaking.** The service builds the vertical motion
   of the hand from the accelerometer and gyro together, so turning the wrist
   doesn't look like a rep. A rep must have a down phase and an up phase, and
   its range of motion must be close to the set's typical rep. So unracking,
   walking a squat out, re-gripping and resting at the top don't count. The
   rule for partial reps is decision 4.
3. **Two passes.**
   - **Live:** a causal detector ticks your wrist after each rep. A missed tick
     is noticed at once.
   - **Final:** when the set ends, a second pass looks at the whole set. It
     finds the set's rhythm (the repeating pattern) and recounts with it. A
     detector that can see the whole set does better than one that only sees
     the past, so the final count wins if they differ.
4. **Set edges are trimmed.** Motion before the first real rep (unracking) and
   after the last (re-racking) is cut off before counting.
5. **A one-press fix, and every fix is data.** The summary shows the count big.
   If it's wrong, L1/L2 fix it, and the fix is saved with a compact copy of the
   set's motion. Repeated misses on one exercise show what to tune.
6. **Measured, not claimed.** Every recorded set from the probe and from
   athletes' sessions becomes a host test ("this set has 8 reps"), run in CI.
   Accuracy is reported per exercise in `NOTES.md`, and a change that makes it
   worse fails the build.

**Targets (proposed; decision 3):** for the exercises in section 6, the final
count is exact on at least 95 % of sets and never more than one rep out. The
live tick comes within half a second of the rep's finish.

What it can't count is in section 6. Those sets are logged with the reps
entered on the wheel, so a whole session still ends up in one file.

## 4. Bar speed: how it's measured

- **What:** the **mean concentric velocity** (the average speed of the lifting
  phase, the usual figure in velocity-based training) for each rep; peak speed
  for fast lifts; and range of motion as a sanity check.
- **How:**
  1. An orientation filter works out where gravity is from the gyro and
     accelerometer, 100 times a second.
  2. Taking gravity away leaves the hand's own vertical acceleration.
  3. Integrating that over the lifting phase, between the still point at the
     bottom and the still point at the top, gives speed.
  4. Speed is forced back to zero at both still points. That removes the drift
     that ruins long integrations; a rep is short (well under 2 s), so little
     error builds up.
- **Velocity loss:** the drop from the set's fastest rep, in per cent. A
  distinct buzz pattern when it passes the cut-off (10, 20 or 30 %; the coach's
  choice).
- **Shown** to two decimals (0.62 m/s), with integer formatting as the
  embedded rules require.
- **If absolute speed fails Gate V0:** the slowdown within a set is a ratio, so
  a constant error largely cancels out. The fallback is to show velocity loss
  and relative speed, not m/s. That's still the signal autoregulation needs.

## 5. The jump test

- **Protocol:** countermovement jump, hands on hips (so the wrist moves with
  the body), three jumps about 30 s apart, landing as you took off, legs
  straight.
- **How:** in the air, the accelerometer reads close to zero. The watch times
  that flight and works out the height: h = g·t²/8.
- **Precision:** at 100 Hz each edge is within 10 ms, and interpolating the
  crossing does better. A 0.5 s flight is about 31 cm; 10 ms of error is about
  1.2 cm.
- **Readiness:** today's best of three against your rolling baseline, in cm
  and per cent. The coach decides what size of drop matters: no built-in
  thresholds.
- **The catch:** landing with bent knees lengthens the flight and inflates the
  height. The screen says how to land, and a jump far off the other two is
  flagged.

## 6. Which exercises work

The rule: **the watch can only see what the watch hand does.**

| Exercise | Count | Speed | Why |
|---|---|---|---|
| Back and front squat | Yes | Yes | Hands ride the bar |
| Bench press | Yes | Yes | |
| Deadlift, Romanian deadlift | Yes | Yes | |
| Overhead press, push press | Yes | Yes | |
| Barbell row | Yes | Yes | |
| Power clean, hang clean | Yes | Peak | Fast: tests whether 100 Hz is enough |
| Thruster | Yes | Yes | |
| Kettlebell swing | Yes | Peak | The hand moves in an arc, not straight up |
| Wall ball | Yes | No | For Race later |
| Dumbbell press, row or curl, **watch arm** | Yes | Yes | |
| Lunges holding dumbbells or a sandbag; burpees | Probe will tell | No | The body carries the hands up and down |
| Pull-ups, push-ups, dips | Probe will tell | No | The hand stays on the bar or the floor; only the forearm's tilt changes |
| Any one-arm lift with the **other** arm | No | No | The watch doesn't move |
| Leg press, leg extension, leg curl, calf machine | No | No | Hands still: reps entered on the wheel |
| Planks, carries | No | No | Timed, not counted |

The v1 list is decision 2. It is everything in the first ten rows that passes
Gate V0.

## 7. The watch, briefly (what shapes the design)

- **Motion sensor:** a BMI270 accelerometer and gyro (SDK diagram). The SDK's
  own Running, RunLVGL, Workout and Treadmill services read it at **100 Hz** as
  `FUSION_RAW` (six raw integers per sample). Neither the conversion to g and
  °/s nor the measuring range the kernel sets is documented. The probe
  measures both. Whether a faster rate (200 Hz) is possible is another probe
  question.
- **Display:** 240 x 240, round, 64 colours. Big numbers only: you glance at
  it between reps, or not at all.
- **Buttons:** four (L1 up, L2 down, R1 select, R2 back), no touch screen. In
  a gym that's an advantage: chalky hands, gloves.
- **Haptics:** about 20 named vibration effects, and sequences of up to 8 steps
  with pauses. Enough for a rep tick, a "set slowed" pattern and a "rest over"
  pattern that feel different. Whether the motor shakes the IMU enough to fool
  the counter is a probe question.
- **FIT:** the SDK's FIT profile has laps, records, sessions, events and
  developer fields, but no strength-training "set" message and no strength
  sub-sport. v1 writes a lap per set, with developer fields. Whether UNA's app
  would show real sets is a question for UNA (section 12).
- **Processes:** as in the other apps, a service owns the sensors and the
  recording, and an LVGL GUI draws the screens. No MMU: fixed buffers, no heap
  in the 100 Hz path.

## 8. Architecture

Same shape as the other apps: a pure C++ core with host tests, a service, and
an LVGL GUI.

**Core** (`Software/Libs/Core`, pure C++, no kernel, sample timestamps
injected, host-tested):

| Part | Job |
|---|---|
| `ImuScale` | Raw integers to g and °/s, from the probe's measured scale |
| `Gravity` | The orientation filter: where down is, at 100 Hz |
| `ExerciseProfiles` | A data table: per exercise, the motion to follow, minimum rep time and size, and whether speed applies |
| `RepDetector` | The live, causal counter |
| `SetRecount` | The end-of-set second pass over a compact trace of the set (not raw samples), with edge trimming |
| `RepVelocity` | Integration over the lifting phase, still-point resets, mean and peak speed, range of motion |
| `SetTracker` | The set and rest state machine; velocity loss and the cut-off |
| `JumpTest` | Flight detection with sub-sample edges, height, baseline |
| `SessionLog` | The plain-file session log and the fixes |

**Recorded sets are the test data.** The probe's session files are cut into
one file per set, labelled with the true count (from video) and the reference
speeds. The host tests replay them through the core. Synthetic signals cover
the edge cases (noise, very slow reps, pauses). A set is about 40 KB raw.

**Service:** from RunLVGL (FIT recording, heart rate, laps). Adds
`FUSION_RAW` at 100 Hz into the core, the haptic patterns, and fixed-size
messages to the GUI: one per rep, one per set.

**GUI:** LVGL, from the scaffold the other apps use:
- exercise picker (the SDK's `WheelMenu`);
- the set screen: count huge, last speed, velocity-loss bar;
- the set summary with the fix;
- the rest screen;
- the session summary;
- the jump test.

**Simulator:** the SDK simulator's IMU models give steps, running cadence and
wrist motion, not raw motion data. V2 adds a source that replays recorded
`FUSION_RAW`, so whole recorded sessions run on the desktop.

## 9. How it knows a set from a rest

Getting set edges wrong is the commonest way for a counter to look stupid, so
this gets its own rules, tuned in V1:

- **Idle → in set:** the first rep that fits the exercise profile. Earlier
  motion (unracking, walking out) is trimmed.
- **In set → rest:** a few seconds' stillness, or a re-rack (the hand stops at
  rack height), or R1.
- **Rest → in set:** the next real rep. The rest timer stops and the new set
  carries the weight forward.
- **A pause mid-set** (a breath at lockout) shorter than the stillness window
  stays in the set. Long pauses for rest-pause sets are a later setting.

## 10. Phases and gates

Same rules as the other apps: plan mode at the start of each phase, stop at
each gate, summarise what was done, how it was verified and what Jon must test
or decide. Build the watch target and the simulator after every change.

| Phase | Work | Gate: what must be true to go on |
|---|---|---|
| **V0: probe** | The Velocity Probe (`Tools/Probe`, `PROBE.md`): a recorder for raw IMU at 100 Hz, with set marks, reps and weight entered on the watch, and a calibration routine. Jon records one filmed gym session (ideally with one athlete too). Offline analysis against the video | **Gate V0:** the data is clean (rate, range, scale, an hour's logging). Offline counting, speed and jump height meet the targets, or Jon accepts a narrower scope (count only, or speed only on the lifts that pass). Details in `PROBE.md` |
| **V1: core** | The core in section 8, with recorded sets as host tests, and a per-exercise accuracy report | **Gate V1:** targets met on the recorded data. Jon agrees the exercise list and the counting rules |
| **V2: service** | From RunLVGL: IMU into the core, haptics, sets and laps, FIT with developer fields, session log. Simulator replay of recorded sessions. App manifest and icons | **Gate V2:** watch build, and a recorded session replayed end to end in the simulator, giving the same counts as the host tests and a FIT that decodes |
| **V3: screens** | Picker, set, summary and fix, rest, session summary, jump test; British English; simulator captures | **Gate V3:** Jon approves the screens from captures |
| **V4: in the gym** | Live counting on the watch, filmed, with Jon and at least one athlete; battery over a 90-minute session | **Gate V4:** live accuracy matches the offline numbers; the haptic cues are right; the FIT reaches UNA and Strava; battery recorded |
| **V5: release** | Store package; tags `velocity-vX.Y.Z` (a version script like Streak's) | **Gate V5:** Jon uploads to the portal |

## 11. Decisions for Jon

1. **Name.** "X Velocity" follows Weigh-in's naming. Now that it's a general
   gym tool, "X Lift" or "X Strength" may say it better. The folder and tag
   prefix follow the name; renaming is cheap until V0 is built.
2. **The v1 exercise list.** Proposed: the first ten rows of section 6, cut to
   what passes Gate V0.
3. **Accuracy targets.** Proposed:
   - counting: exact on 95 % of sets, never more than one out;
   - speed: the figures in `PROBE.md`;
   - jump: within 2 cm.
   Tighter or looser?
4. **Partial reps.** Count them, or not? Proposed: not, below a fraction of the
   set's typical range of motion, tuned from data.
5. **Velocity-loss cut-off.** Off by default, with 10, 20 or 30 % as the
   choices. Per exercise or one setting?
6. **Weight entry.** On the watch per set, in kg only like Weigh-in? Or
   optional and left blank?
7. **The probe session.** Jon plus at least one athlete with a different
   build, if possible. A borrowed linear encoder or a bar-mounted VBT sensor
   would make a much better speed reference than video, if your gym or
   network has one.
8. **Recorded test data in the repo.** About 40 KB per set, so a few MB in all.
   Fine as plain files, or use Git LFS?

## 12. Questions for UNA

1. Which ranges does the kernel set on the BMI270 (accelerometer g, gyro °/s),
   and what is the scale of `FUSION_RAW`? Can an app ask for a different range
   or for 200 Hz?
2. What units are the `GYROSCOPE` and `FUSION` floats in? `SensorsLayer.md`
   gives g for the accelerometer but nothing for the gyro.
3. Does the vibration motor disturb the IMU? Is there guidance on haptics
   during motion capture?
4. If an app writes FIT strength-training sets (the FIT profile's set message,
   a strength sub-sport), does UNA's app or platform show them? Would UNA add
   them to the SDK's `FitProfile.hpp`?
5. How much flash may an app use for its own logs?

## 13. Risks

| Risk | What happens | Mitigation |
|---|---|---|
| Wrist motion isn't bar motion | Speeds off on some lifts | Speed only where Gate V0 shows it works; relative speed as the fallback (section 4) |
| Range set too low on the IMU | Fast lifts or landings clip | The probe measures clipping; ask UNA (section 12) |
| 100 Hz too slow for cleans | Peak speed under-read | The probe tries 200 Hz; cleans show mean speed only if peak fails |
| The haptic tick shakes the sensor | Phantom reps or noisy speed | The probe's buzz test; time the tick away from the next rep's start, or skip ticks on that lift |
| Counting tuned to one lifter | Worse for athletes | At least two lifters in V0, more in V4; every fix becomes test data |
| Set edges wrong | Counts and rest timer look stupid | Section 9 rules, tuned on messy recorded sets; R1 as the manual override |
| Video is a weak speed reference | Gate V0 can't tell good from bad | Film at 120 fps or more, with the plate as the scale; a borrowed encoder if possible |
| FIT sets not shown by UNA or Strava | Reps live only in our app | Laps per set and developer fields for v1; the session log for HybridX; ask UNA |
