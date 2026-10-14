# The Velocity Probe: Gate V0 plan

**Status:** plan (12 October 2026). The probe isn't built yet; building it is
V0's first job. The install steps follow the other probes and will be
confirmed against the real app before Jon runs it.

X Velocity depends on things only a real watch on a real lifter can show: how
the motion sensor is set up, whether 100 samples a second arrive cleanly for
an hour, and whether the wrist's motion is good enough to count reps, measure
bar speed and time a jump. The **Velocity Probe** is a small, throwaway app
that **records** the raw motion data during a gym session, with marks for each
set. It does no counting itself. All the analysis happens afterwards, on a
computer, against video of the same session.

Recording first is deliberate. One good session gives us a data set that we can
run every version of the counter against for months, and every set in it
becomes a host test.

It takes one gym session, about 75 to 90 minutes, plus five minutes of
calibration at a table.

---

## What the probe must answer

| # | Question | How |
|---|---|---|
| U1 | The scale of the raw numbers: what one unit is in g and in °/s | Calibration: the watch still in six positions (gravity is exactly 1 g), and turned round flat on a table. For a direct check, the probe also records the documented `ACCELEROMETER` stream (in g) next to `FUSION_RAW` |
| U2 | The measuring range: do fast lifts or landings clip? | The largest raw value on each axis, per set. A value pinned at the limit is a clip |
| U3 | Do 100 samples a second arrive with no gaps, for an hour? Is 200 Hz possible? | Sample timestamps for the whole session; a 30-second trial asking for 200 Hz |
| U4 | Can an hour of raw data be written to a file without losing any? | Blocks written once a second; the probe counts lost or late blocks |
| U5 | Rep counting from the wrist | Offline: counts on recorded sets against the counts from video |
| U6 | Bar speed from the wrist | Offline: the watch's speeds against speeds measured from the video (or an encoder) |
| U7 | Jump height from flight time | Offline: against flight time counted in slow-motion video frames |
| U8 | Does the vibration motor fool the sensor? | A "buzz test" set: the probe buzzes every 2 s while you lift |

## What you need

- The watch, worn **snugly** on the same wrist all session. Note which wrist.
- A phone that films in slow motion: 120 fps or more, 240 fps best for jumps.
  Something to stand it on at bar height, about 3 m away, side-on to the lift.
- A barbell with at least one **full-size plate (450 mm across)** on the side
  facing the camera. It's the scale for measuring speed from the video. A strip
  of bright tape on the end of the bar sleeve helps the tracking.
- Safety bars or a spotter for squat and bench. Some sets go until the bar
  clearly slows.
- Ideally, **one athlete** as well as you, with a different build. Same
  protocol.
- If your gym or network has one: a **linear encoder or a bar-mounted VBT
  sensor**. It's a much better speed reference than video. Note its readings
  per rep.
- The log sheet at the end of this file, or a notes app.

## Step 1: download and install the probe

Same as the other probes:

1. On GitHub, open **Actions**, pick the newest green **Watch builds** run on
   the branch, and download the **watch-apps** artifact.
2. Inside is `XVelocityProbe_0.1.0.uapp`. Only use this GitHub-built copy.
3. Connect the watch by USB and wait for its drive to appear. On it, create
   `Apps/XVelocityProbe/` and copy the `.uapp` in.
4. Eject safely, unplug, then power the watch off and on.
5. Open **Velocity Probe**. The first screen shows the live sample rate, gaps
   and clips. Leave it on the table for ten seconds: it should read about
   **100 Hz, 0 gaps, 0 clips**.

## Step 2: calibrate (5 minutes, at a table)

Choose **Calibrate**. The watch shows each position and buzzes when it's time
to move to the next:

1. Face up, face down, and on each of its four edges, 5 s each, held still.
2. Flat on the table, turn it round once, steadily, back to where it started.
   Three times.

## Step 3: the lifts

For each exercise: choose **Lifts**, pick the exercise, and start filming
side-on. Then, for each set:

1. Press **R1**. The screen flashes white, which marks the start in the video.
2. **Tap the bar three times, sharply, with the watch hand.** The taps show in
   both the data and the video, which lines them up exactly.
3. Lift.
4. Press **R1** when the set's done (re-racked or put down).
5. Enter the **reps** you did (L1/L2, R1), then the **weight** (the last one is
   remembered, so usually just R1).

Rest as normal. One video per exercise is fine. Keep the camera running
between sets, or restart it before each set's taps.

**The session.** Warm up as you like first. Barbell lifts get three sets:
light × 8 moving fast, medium × 5, and a heavier set taken until the bar
clearly slows, stopping with one or two reps in the tank.

| Order | Exercise | Sets | Notes |
|---|---|---|---|
| 1 | Back squat | 3, then 1 messy | **Messy set:** unrack, walk out, 3 reps, 5 s pause standing, 3 reps, re-rack, re-grip, walk away |
| 2 | Bench press | 3, then 1 messy | Messy set: unrack, 3 reps, pause at lockout, 3 reps, re-rack |
| 3 | Deadlift | 3 | Touch-and-go on one set, reset each rep on another |
| 4 | Overhead press | 3 | |
| 5 | Wall balls | 2 × 15 | |
| 6 | Kettlebell swings | 2 × 15 | |
| 7 | Pull-ups | 2 | As many as is comfortable |
| 8 | Push-ups | 2 × 10 | |
| 9 | **Buzz test:** back squat, light | 1 × 8 | Choose **Buzz test** instead of Lifts. The watch buzzes every 2 s during the set |
| If time | Power clean | 3 × 3 | |
| If time | Dumbbell row | 1 × 10 each arm | Watch arm and other arm: the other arm should fail, and we want to see how |
| If time | Lunges holding dumbbells; burpees | 1 each | |

## Step 4: the jump test

Choose **Jumps**. Film side-on at 240 fps if your phone can, with your feet in
frame.

1. Press R1, tap your hip three times with the watch hand, then put your hands
   on your hips.
2. Three countermovement jumps, about 30 s apart. Land as you took off, legs
   straight.
3. Press R1.
4. Do this three times (nine jumps).
5. Then one more set of three **with arm swing**, to see how badly that
   confuses the wrist.

## Step 5: the 200 Hz trial

Choose **Rate test**. The probe asks for 200 samples a second for 30 seconds.
Shake the watch gently. The screen shows what rate actually arrived.

## Step 6: send the results back

1. Close the probe with **R2**.
2. Connect the watch by USB.
3. Copy the whole `Apps/XVelocityProbe/Sessions/` folder, which holds a `.bin`
   per session and a `.txt` summary.
4. Put the session files, the videos and the log sheet (or a photo of it) in a
   **Google Drive folder** and send me the link. I can read Drive from here.
   Videos are too big for the repository.
5. Tell me which wrist, the watch strap fit, and the phone's frame rate.

## Step 7: remove the probe (optional)

Delete `Apps/XVelocityProbe/` over USB.

## What happens next (my side)

1. **The data checks** (U1 to U4, U8) come straight from the files: scale,
   range, rate, gaps, lost blocks, and whether buzzing changed anything.
2. **The video reference:** a small script tracks the bar end against the
   450 mm plate, frame by frame. That gives the bar's speed through each rep
   and each set's true count. Jump flight times come from the frames.
3. **First-pass algorithms** run on the recordings (the start of V1's core):
   counting, speed per rep, velocity loss, jump height. They are scored
   against the reference, per exercise.
4. **A V0 report** in `NOTES.md`, with a verdict per exercise.

## Gate V0: what counts as GO

These targets are proposals (brief, decision 3).

| Check | GO if |
|---|---|
| Sample rate | At least 99.9 % of samples arrive at 100 Hz; no gap longer than 50 ms all session |
| Range | No clipped samples during lifts (clipping on jump landings is acceptable if the edges are clear) |
| Scale | Accelerometer and gyro scale measured to within 1 % |
| Logging | The whole session logged, no lost blocks |
| Counting | For each candidate exercise: exact on at least 95 % of sets, never more than one rep out |
| Speed (squat, bench, deadlift, press) | Average difference from the reference within 0.05 m/s; 95 % of reps within 0.10 m/s; velocity loss within 5 percentage points |
| Jump height | Within 2 cm of the video, jump by jump |
| Buzz test | The same count as the video, and speeds no noisier than a normal set |

**Outcomes:**
- **GO:** everything passes for the core lifts.
- **GO with a narrower scope:** the exercise list shrinks to what passes.
  Counting may pass where speed doesn't; then speed becomes relative (velocity
  loss only) or is dropped for those lifts.
- **NO-GO:** the data itself is unusable (gaps, clipping everywhere, an
  unknowable scale). Ask UNA (brief, section 12) before going further.

## If it is not GO

- **Gaps or lost blocks:** try a longer batching latency, bigger write blocks,
  or writing less (accelerometer only for the jump test).
- **Clipping:** ask UNA for the range, or whether an app can set it. Jump
  landings can clip harmlessly; lifts can't.
- **Speed off, counts fine:** ship the counter and velocity loss, with no m/s.
  That's a useful product on its own.
- **Counting off on one lift:** drop that lift from the v1 list. Record more
  sets of it and retry in V1.

---

## Log sheet

| Set | Exercise | Weight (kg) | Reps | Notes (messy, partial, buzz test, failed rep) |
|---|---|---|---|---|
| 1 | | | | |
| 2 | | | | |
| 3 | | | | |

---

## For developers

- **Layout:** `x-velocity/Tools/Probe`, its own CMake project, as Trail's
  probe is. Built by CI into the `watch-apps` artifact as
  `XVelocityProbe_0.1.0.uapp`.
- **Service:**
  - `FUSION_RAW` at a 10 ms period with 100 ms latency, as RunLVGL does;
  - `ACCELEROMETER` (float g) alongside it during calibration only, for U1;
  - the 200 Hz trial asks for a 5 ms period and records what arrives.
- **File format** (`Sessions/Snnn.bin`, little-endian):
  - a header: magic, version, the period asked for, the build;
  - sample records: a timestamp delta in ms, then six `int16`;
  - marker records: calibration step, set start or end, exercise id, reps,
    weight, buzz fired, rate-test start or end.
  About 1.4 KB a second, so about 5 MB an hour. Written from a double buffer
  once a second; the 100 Hz path never allocates.
- **Summary** (`Sessions/Snnn.txt`): sample count, actual rate, gaps over
  20 ms, the largest value per axis, clips, lost blocks, write time per block
  (worst and average), and the sets.
- **GUI:** a main menu (Calibrate, Lifts, Buzz test, Jumps, Rate test), an
  exercise picker, a set screen (elapsed time, live rate, a white flash on
  start), and wheels for reps and weight. The live rate, gaps and clips are
  shown on the first screen as a quick verdict.
- **Host tools** (`x-velocity/Tools/Analysis`): a reader for the `.bin`
  format, the set splitter that makes host-test fixtures, and the video bar
  tracker.
