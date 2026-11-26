# X Energy: notes

**Name (Jon, 24 November 2026): X Energy**, to sound more optimistic than X Sleep, Body
or Battery. The folder was `x-sleep/` for the desk study. The final watch name is still
open (at most 15 bytes); the Gate SL0 probe is **Energy Probe** (`docs/PROBE.md`).

**The app:** automatic sleep and nap tracking, plus an all-day energy score of the kind
Garmin calls Body Battery. It also covers rest, stress and exercise. It is independent of
the other apps, but it would read the family's training load and share its results.

This file is the **scoping desk study** (24 November 2026): what the UNA Watch lets an
app measure, which sleep and energy metrics can be worked out from that, how far each
can be trusted, and what must be tested on a watch before anything is built. Nothing
here has run on a watch yet.

## The idea (Jon, 24 November 2026)

"A combined sleep tracker and body battery measure, as with Garmin devices and others.
The sleep tracker needs to be automatic, ideally to sense sleep automatically, naps,
rest alongside tracking body stresses, anxiety and exercise."

## The short answer

1. **Automatic tracking is possible in principle.**
   - A service started with `APP_AUTOSTART` runs from boot with no screen. It keeps its
     sensor subscriptions while other apps are open (service-lifecycle.md §3.2, §4.1).
   - The SDK's Alarm example already runs this way.
   - Whether a service can **afford** to watch you all night and day is the first
     unknown: the battery cost has never been measured. That is Gate SL0's first question.
2. **The core of sleep tracking works from motion and heart rate alone, by published
   methods.** Both are proven on Jon's watch in our apps.
   - What works: when you slept and woke, time asleep, wakings, restlessness, sleeping
     heart rate, regularity and sleep debt.
   - Naps and rest periods use the same detector, but are less certain.
3. **Garmin-style stress and sleep stages need beat-to-beat heart-rate variability
   (HRV).** This is the biggest open question.
   - The SDK lists a `HEART_BEAT` sensor ("beat peak event"), but gives it no parser,
     no simulator model and no example. Nobody, including us, has used it.
   - If it gives beat times, overnight HRV and a real stress score become possible.
   - If it doesn't, stress becomes a weaker proxy: a raised heart rate while you are
     still. Sleep stages should not be shown either way in a first version.
4. **An energy score (Garmin's Body Battery idea) can be built as our own open model.**
   - Sleep and rest charge it; activity and a raised heart rate at rest drain it.
   - It would use the family's own load score (Edwards' TRIMP).
   - Its numbers are ours to tune, not a validated measure.
   - "Body Battery" is Garmin's trademark, so it needs our own name.
5. **Anxiety cannot be measured from the wrist, and the app must not claim to.**
   - The honest version is a three-second self check-in on the watch, shown beside the
     measured numbers.
6. **Ask UNA first.**
   - Intervals was retired when UNA built structured workouts themselves
     (`hybridx-race/docs/NOTES.md` 5.25).
   - Nothing in the SDK hints at native sleep tracking: no sleep sensor type, no sleep
     FIT message, nothing in the docs. Even so, the same question should go to UNA
     before work starts, with the `HEART_BEAT`, SpO2 and power questions (see
     "Questions for UNA").

---

## Desk study: what the watch gives an app

Every claim cites the SDK at the pinned commit (`una-sdk/`, `sdk-v1.6.2`) or our own
notes.

### Running all night and all day

- **Autostart.** `APP_AUTOSTART On` starts the app's service at boot, "and only the
  service", with nothing on screen. It starts again on leaving USB mass storage
  (`Docs/service-lifecycle.md` §3.2).
  - The Alarm example is a Utility app built this way
    (`Docs/Examples/Alarm-Architecture.md`, "APP_AUTOSTART On").
  - `APP_TYPE` is one of Activity, Utility, Glance or Clockface (`Docs/sdk-setup.md:323`).
    This app would be a **Utility** with autostart.
- **What a resident service still gets.** Its sensor data arrives at the rate it asked
  for, "ungated". Other apps opening, the launcher, the glances screen and the screen
  timing out do not stop it (§4.1, §7).
- **Nothing wakes it on a timer.** The service waits for messages with a time limit
  (`getMessage(msg, kTickMs)`) and does its timed work when the wait runs out. It reads
  elapsed time from `sys.getTimeMs()` and never counts its own wake-ups (§4.2).
  - Asking for sensor data in batches (the `latency` argument of
    `SDK::Sensor::Connection`) should let it wake about once a minute. Whether batching
    saves power is for the probe.
- **Save as it goes.** A reboot, a firmware update's reboot, a watchdog reset or power
  loss give "no notice at all" (§7). Every few minutes of data must be written when it
  is made.
- **Not promised.** "That an idle service is never reclaimed", and "that a service
  exceeding its resource budget is left alone" (§1.1). A later kernel could limit
  full-time services, which is a store and future risk (Questions for UNA, 6).
- **Memory.** A resident service holds its RAM.
  - If later apps don't fit, "the launch fails". An already-running service is not
    removed to make room (§8).
  - So the logger must be small, and the probe must check that the family's largest
    apps (X Training, Race) still open while it runs.
- **It can take the screen.** An autostarted service can bring up its own screen
  (`REQUEST_APP_RUN_GUI`), as Alarm does to ring (§3.2). A smart alarm needs this.

### The sensors

The SDK's hardware diagram names the parts: a **PixArt PAH8316LS** optical heart-rate
sensor, a **BMI270** motion sensor and an **MS5837** barometer
(`Docs/development-workflow.md:435-437`). We found no public datasheet for the
PAH8316LS.

Status:
- **Proven** means one of our apps has read it on Jon's watch.
- **Documented** means the SDK gives a parser, but none of our apps has run it.
- **Unknown** means the SDK lists the type with no parser and no description of its data.

| Sensor (`SDK::Sensor::Type`) | Gives | Status | What it is for here |
|---|---|---|---|
| `HEART_RATE` (0x41) | bpm and a trust level 0-3 | **Proven** (Race, Run, Sauna, ...; Race's gate `bpm > 20 && trust 1-3`, Race NOTES:324) | Sleeping HR, rest, effort, energy |
| `HEART_RATE_EX` (0x43) | as above, plus the raw optical bpm, which is "0 when absent/stale/off-wrist" (`SensorDataParserHeartRateEx.hpp:33`) | **Proven** (Race) | Off-wrist detection |
| `HEART_RATE_METRICS_DAILY` (0x42) | the day's average and **resting** HR (`SensorTypes.hpp:41`) | Documented (the SDK's GlanceARHR example) | Resting HR without our own sums; also a sign the watch already samples HR all day |
| `HEART_BEAT` (0x40) | "Beat peak event" (`SensorTypes.hpp:39`) | **Unknown**: no parser (`Docs/SensorsLayer.md` table), no simulator model, used by no example and none of our apps | **The key to HRV**: beat times give the intervals between beats |
| `PPG` (0xF0) | "Photoplethysmogram data" | **Unknown**: no parser, no layout | Our own beat detection, if `HEART_BEAT` fails (heavy work) |
| `SPO2` (0xF1) | saturation % and trust | Documented, but whether this sensor has the red or infrared light SpO2 needs is not stated | Overnight oxygen trend |
| `ECG` (0x100) | "Electrocardiogram data" | Listed only; the hardware diagram shows no ECG part | Nothing |
| `ACCELEROMETER` / `FUSION_RAW` | 3 or 6 axes | **Proven** at 100 Hz during activities (RunLVGL, Workout; Velocity NOTES) | Arm angle and movement: the backbone of sleep detection |
| `MOTION_DETECT` (0xB0) | events: no motion, motion, significant motion | Documented | A cheaper motion signal than raw samples, if it is fine enough |
| `ACTIVITY_RECOGNITION` (0xC0) | still, walking, running, with confidence % | Documented | Rest versus moving about |
| `WRIST_MOTION` (0xA0) | wrist-raise events | **Proven** (Sauna's backlight) | A sign of being awake (looking at the watch) |
| `TOUCH_DETECT` (0x140) | "worn / unworn" (`SensorTypes.hpp:125`) | Documented | **Essential**: a watch on the bedside table is perfectly still and would score as deep sleep |
| `AMBIENT_TEMPERATURE` (0x70) | °C | **Proven** (Sauna): a temperature *inside* the watch with a long lag, which "settles near the skin under it plus a little" (Sauna NOTES:34, :654) | Overnight wrist-temperature change from your normal, where the lag does no harm |
| `STEP_COUNTER_DAILY`, `ACTIVITY_TIME_DAILY`, `FLOOR_COUNTER_DAILY` | today's steps, active minutes, floors | Documented (the SDK's GlanceSteps example) | Daytime activity context |
| `BATTERY_LEVEL`, `BATTERY_CHARGING` | % and on-charger | Documented | Not worn while charging; the probe's battery-drain measurement |

**Not in the SDK:**
- no breathing rate;
- no stress, sleep-state or HRV output;
- no history of past sensor readings that an app can read later.

The watch's own resting HR (above) is the only figure the watch keeps for the day.
Everything else an app wants about last night, it must have recorded itself while it
happened.

### Profile, clock and settings

- **The profile gives:**
  - height and weight (`RequestSystemSettings`, `CommandMessages.hpp:218-219`);
  - the watch's **heart-rate zone thresholds** (`ISettings::getHrThresholds()`), the
    same zones the family's training load uses (`hybridx-link/docs/ACTIVITY_LINK.md:67`);
  - daily goals.
- **It has no birth date, sex or time zone** (Weigh-in NOTES:55). Age-based figures
  would need phone settings, as Weigh-in and Run have.
- **Local time:** `time(nullptr)` is UTC; `localtime_r` gives local time in the zone *as
  it is now* (Streak NOTES E.5).
  - A night that crosses a time-zone change (travel, or the clocks going back) must be
    stored in UTC and shown in local time.

### Output, storage and sync

- **Haptics:** named effects and patterns (`IVibro.hpp`). The buzzer, and the Alarm
  pattern of waking the screen to ring.
- **The glance:** a 240 × 60 strip (`Docs/sdk-overview.md:66`), as Streak, Run,
  Training and Weigh-in already use.
- **Storage:**
  - the app's own folder;
  - `../SharedData/` for anything other apps should read (`Docs/shared-data.md`);
  - the family's `../SharedData/HybridX/`.
  - There is no free-space call (`IFileSystem.hpp`), and how much flash an app may use
    is not documented. X Velocity asked UNA the same question (Velocity NOTES, open
    question 5).
- **Sync:**
  - only activity FIT files reach the phone. The SDK's FIT profile has no sleep or
    monitoring messages (`Libs/Header/SDK/Fit/FitProfile.hpp`).
  - So, like X Weigh-in, **sleep data would stay on the watch**: no UNA app, no Garmin
    Connect, no Strava.

### The battery

- **What the documents say:**
  - The SDK's power diagram says a 400 mAh LiPo (`Docs/architecture-deep-dive.md:1713`).
    It is a generic diagram, so treat it as unverified.
  - A retailer's page (not UNA's) says about 10 days in everyday use and 20 hours with
    GPS.
- **What that implies:** if both are right, the whole watch averages about
  400 mAh / 240 h ≈ **1.7 mA**.
- **What an extra drain would cost**, on the same two assumptions:

  | Extra average current from this app | Days of everyday use |
  |---|---|
  | none | 10 (the retailer's figure) |
  | 0.25 mA | 8.7 |
  | 0.5 mA | 7.7 |
  | 1 mA | 6.2 |
  | 2 mA | 4.5 |

- **What we don't know:**
  - what the optical sensor costs at 1 reading a second, or whether a longer period
    dims it;
  - whether the watch already samples HR all day. It must sample something, since it
    reports a daily resting HR.
- **How the probe answers it:** it logs `BATTERY_LEVEL` through whole nights and days
  in each set-up.

---

## What can be measured, worked out, or neither

**How each metric is labelled:**

- **Kind**:
  - *Measured*: a sensor gives it.
  - *Worked out*: a published method applied to sensor data.
  - *Our model*: our own formula, with numbers for Jon to tune.
  - *Logged*: the wearer enters it.
- **Status**:
  - **Ready**: its inputs are proven on Jon's watch and the method is published.
  - **Likely**: an input is documented but unproven; the probe confirms it.
  - **Unknown**: it depends on `HEART_BEAT`, `PPG` or `SPO2`; only the probe or UNA can
    say.
  - **No**: no sensor for it, or not something a wrist can honestly measure.
- Everything automatic also depends on **Gate SL0's first answer**: a resident service
  that lasts all night at an acceptable battery cost.

### Sleep

| # | Metric (label on the watch) | Kind | How | Status | How far to trust it |
|---|---|---|---|---|---|
| 1 | **Bedtime and wake time** ("Asleep 23:20, up 06:45") | Worked out | Finds the night's sleep window from how little the arm's angle changes, with no diary: van Hees et al. 2018 (HDCZA), using 5-second arm angles from the accelerometer | Ready | Against lab sleep studies, a C-statistic of 0.86 (clinic patients) and 0.83 (good sleepers); tested against diaries in 3,752 people |
| 2 | **Time asleep**, **awake in the night**, **wakings**, **efficiency** | Worked out | Minute by minute: asleep when the arm's angle has not changed by more than 5° for 5 minutes (van Hees 2015; Cole-Kripke is the classic alternative); HR rising marks a waking | Ready | Wrist motion finds sleep well but misses lying awake still. For Cole-Kripke against a sleep lab: sensitivity 0.97, **specificity 0.33** (Marino 2013). HR should claw some of that back; how much is for the probe |
| 3 | Time to fall asleep | Logged, optional | Needs a "lights out" moment, which a wrist can't know. A "going to bed" press would give it | **No** automatically | |
| 4 | **Restlessness** | Worked out | Movement minutes within the window | Ready | Good: it is what the sensor measures directly |
| 5 | Sleep stages (light, deep, REM) | Worked out | Needs HRV as well as motion | **No** on motion and bpm alone; Unknown with beats | Even the best consumer devices (Oura, Fitbit, with HRV and years of tuning) reach a kappa of 0.45-0.70 against a sleep lab on four stages; cheap trackers below 0.33. **Recommendation: no stages in v1.** Show restless and settled periods instead |
| 6 | **Sleep debt** ("2 h 40 short this week") | Worked out | Time asleep against a target. Adults 18-60 need "7 or more hours" regularly (AASM and SRS consensus, Watson 2015). The wearer sets their own target | Ready | As good as #2 |
| 7 | **Regularity** (the Sleep Regularity Index; bedtime spread; weekday-to-weekend shift) | Worked out | SRI: the chance of being in the same state, asleep or awake, at two times 24 hours apart, 0-100 (Phillips 2017); from 7 nights | Ready | Robust: it uses timing, which actigraphy gets right |
| 8 | **Sleep score** (0-100) | Our model | From #2, #4, #6, #7 and sleeping HR | Ready (as a design) | Ours: the weights are Jon's, and the screen should say what drove it |
| 9 | **Smart alarm**: buzz at a restless moment in the 20 to 30 minutes before the alarm | Our model | Movement, plus the Alarm example's way of taking the screen | Likely | An extra. The evidence that it helps is thin |

### Naps and rest

| # | Metric | Kind | How | Status | How far to trust it |
|---|---|---|---|---|---|
| 10 | **Naps** ("Nap 14:10, 25 min") | Worked out | The same detector outside the night, counted only if HR drops towards sleeping levels; shortest nap is Jon's call | Ready (inputs) | **The weakest of the automatic sleep figures.** Lying on the sofa watching television looks like a nap to motion; the HR rule is what has to tell them apart. Expect false naps until tuned on real days |
| 11 | **Rest** (still, awake, HR near resting) | Worked out | Motion still, HR within your resting band, not asleep | Ready | Good as a description. It is what charges the energy score in the day |

### Heart and body

| # | Metric | Kind | How | Status | How far to trust it |
|---|---|---|---|---|---|
| 12 | **Resting HR** (the watch's own, today) | Measured | `HEART_RATE_METRICS_DAILY` | Likely | UNA's figure; how it is worked out is not documented |
| 13 | **Sleeping HR** (lowest, average, lowest 30 minutes) | Worked out | Trusted HR within the sleep window | Ready | Good: the wrist reads HR best when still. A 2-week normal makes "4 above your normal" meaningful |
| 14 | **Overnight HRV** (RMSSD) | Worked out | Intervals between beats from `HEART_BEAT` | **Unknown**: the biggest single question | If beats arrive, wrist optical RMSSD at night agrees well with a chest ECG (Polar Vantage V2: ICC 0.93). For WHOOP the error was close to the smallest meaningful change, so show **trends over days, not one night against the last** |
| 15 | Breathing rate | Worked out | From beat-to-beat intervals or raw PPG | Unknown, and hard | Not for v1 |
| 16 | Blood oxygen overnight | Measured | `SPO2` | Unknown | If it works: a trend only, never a breathing-disorder claim |
| 17 | **Wrist temperature change** ("+0.4° on your normal") | Worked out | Night's average of the watch's temperature against a 2-week normal | Likely | Proven to settle near skin temperature (Sauna). Overnight the lag doesn't matter. Its stability over a night is for the probe. Room temperature and bedding move it too |
| 18 | **Worn or not** | Measured | `TOUCH_DETECT`, the raw optical bpm 0 off the wrist, on charge | Likely | Essential, not shown: no figure counts minutes when the watch was off |
| 19 | Steps, active minutes, floors today | Measured | The `*_DAILY` sensors | Likely | UNA's own counts |

### Stress, anxiety and exercise

| # | Metric | Kind | How | Status | How far to trust it |
|---|---|---|---|---|---|
| 20 | **Exercise sessions and their load** | Measured (by the family) | Each X app's sessions and Edwards loads in `act_*.json`; fitness, fatigue and form in `streak_now.json` | Ready | Proven by the family check. Only our own apps' sessions, not UNA's built-in ones (`hybridx-link/docs/ACTIVITY_LINK.md`) |
| 21 | **Effort outside sessions** (stairs, walking, gardening) | Worked out | Minutes in the watch's HR zones, weighted 1 to 5 as the family's Edwards load is | Ready | As good as wrist HR on the move, which is weaker than at rest |
| 22 | Stress, Garmin-style (0-100 from HRV) | Worked out | Needs beats: Firstbeat's method, which Garmin uses, reads stress and recovery from 24-hour HRV | Unknown | The real thing, if `HEART_BEAT` works |
| 23 | **Raised heart rate at rest** ("HR up 18 while still, 40 min") | Our model | HR above your resting band while motion says still and you are not asleep | Ready | **Not "stress".** It cannot tell worry from coffee, heat, illness, a meal or alcohol. Label it for what it is |
| 24 | Anxiety | **No** | No wrist sensor measures anxiety; claiming to is a medical claim | **No** | |
| 24a | **How you feel** (a check-in: calm to very stressed, 1 to 5; optional tags such as caffeine, alcohol, late meal) | Logged | Three seconds on the watch, any time | Ready | Honest, and trackable. Shown beside #13, #17 and #23, it lets the wearer see their own patterns without the app claiming to detect them |

### Energy and readiness

| # | Metric | Kind | How | Status | How far to trust it |
|---|---|---|---|---|---|
| 25 | **Energy** (0-100 all day: our own Body Battery) | Our model | Charges during sleep and rest, faster the further HR sits below your normal (or by HRV, if beats work). Drains with effort (#20, #21 on the family's zone weights), with raised HR at rest (#23), and slowly while awake | Ready (inputs) | **Ours, and validated against nothing.** It can be made sensible and explainable, and compared with Jon's Garmin over the probe weeks, but it is not a measurement. Its constants are provisional, for Jon to tune, as Streak's form words were. **Needs its own name**: "Body Battery" is Garmin's trademark |
| 26 | **Readiness** (each morning) | Our model | Sleep against need (#2, #6), sleeping HR against normal (#13), form from Streak (#20), temperature change (#17), HRV if available (#14) | Ready | As #25. It fits the family: X Training could offer to ease a session after a short night (TP5 already makes offers) |
| 27 | **"Something's up"** | Our model | Sleeping HR and temperature both above normal for two nights or more | Likely | Information only: "your body is working harder than usual". Never "you are getting ill" |

**What v1 could honestly ship** (if Gate SL0 passes):
- #1, #2, #4, #6, #7, #8: automatic sleep;
- #10, #11: naps and rest, tuned on real days;
- #12, #13: resting and sleeping HR;
- #17, if the probe shows the temperature is steady overnight;
- #20, #21: exercise from the family, and effort outside it;
- #23: raised HR at rest;
- #24a: the check-in;
- #25, #26: energy and readiness.

If `HEART_BEAT` gives beat times, a v2 adds:
- #14, overnight HRV;
- #22, a true HRV stress score;
- HRV-based charging for #25.

---

## How it would work (outline, for discussion)

- **Three parts:** a resident logger (the service), the app's screens, and a glance.
- **The logger:**
  - A Utility with `APP_AUTOSTART On`, closing a **one-minute epoch** each minute.
  - Each epoch holds:
    - the arm-angle changes from 5-second mean angles;
    - a movement count;
    - HR minimum, mean, maximum and trusted seconds;
    - temperature;
    - flags: worn, on charge, in a recorded activity.
  - About 12 bytes a minute, so a day is 1,440 × 12 ≈ 17 KB.
  - Today's epochs stay in RAM and are appended to a day file every 10 minutes. A
    reboot then loses at most 10 minutes (service-lifecycle §7).
  - Fourteen days of minutes is about 242 KB. A night's summary is about 64 bytes, so a
    year of them is about 23 KB.
- **Working out the night:**
  - When the morning's sustained movement closes the window, the logger works out the
    night once.
  - It writes its own `nights.json`, and `../SharedData/HybridX/sleep_now.json` for the
    glance and the family.
- **Energy:** updated every minute from the epoch (cheap integer arithmetic) and stored
  with it.
- **The pure core:**
  - the window finder, minute scoring, naps, energy and readiness;
  - an injected clock, as the standing rules ask;
  - host-tested on the probe's recorded nights. As X Velocity's rule has it, **every
    recorded night becomes a test**.
- **Screens:**
  - Last night;
  - Energy now (with today's curve);
  - Trends (7 and 28 days: sleep, sleeping HR, regularity);
  - Check-in;
  - Settings.
- **Glance:** energy now and last night's sleep (for example "Energy 72 · slept 7h12").
- **Family:**
  - Streak could show readiness next to form;
  - X Training could take a short night into its offers;
  - the logger reads `act_*.json` to know which minutes were recorded sessions.
- **Fallback if a full-time service is not possible:** a "Sleep" activity the wearer
  starts at bedtime and stops on waking, recording like the SDK's HRMonitor example.
  - Still possible: bedtime, wake, time asleep, wakings, sleeping HR, regularity.
  - Lost: the automatic start, naps, all-day rest and the energy score.

## Risks

1. **Battery.** The whole idea rests on a cost the wearer accepts. The probe measures
   each set-up, from motion only to HR every second.
2. **The bedside table.** A watch taken off is perfectly still. Off-wrist detection
   (#18) must work, or every evening the watch is off counts as deep sleep.
3. **Awake but still** (reading in bed, television). Actigraphy's known weakness (#2).
   HR helps; the probe measures how much.
4. **Naps against sitting still.** The same problem in daylight (#10).
5. **Memory.** A resident logger could make the family's larger apps
   fail to open (service-lifecycle §8).
6. **The future kernel.** Full-time services are allowed but not promised
   (service-lifecycle §1.1). UNA might also restrict them in the store.
7. **No sync.** Sleep data never leaves the watch unless UNA adds a way, as with
   Weigh-in.
8. **Claims.**
   - It must stay a wellness app: no anxiety, sleep apnoea, heart-rhythm or illness
     claims.
   - Software that diagnoses can count as a medical device (in the UK, under the MHRA).
     Check the wording before the store listing.
9. **Names.** "Body Battery" (Garmin) cannot be used for the energy score.
10. **UNA builds it themselves.** The Intervals lesson.

## Gate SL0: the probe

**The build.**
- A small autostart Utility, `SleepProbe`, that logs a CSV line a minute:
  - motion (arm-angle change, movement count);
  - HR (min, mean, max, trusted seconds);
  - temperature;
  - the touch, motion and activity-recognition events;
  - battery % and charging.
- At the start of each night it also tries `HEART_BEAT`, `PPG` and `SPO2` for
  10 minutes each, logging every field it gets and the sensor's description text
  (`RequestGetDesc`).
- A phone setting chooses one of four set-ups:
  - **A**, motion only;
  - **B**, A plus HR every second;
  - **C**, A plus HR for one minute in every five;
  - **D**, B plus temperature and the event sensors.

**What it must answer:**

1. Does the service run all night and all day, through:
   - the screen going off;
   - other apps opening;
   - a run recorded in X Run;
   - USB?
2. What each set-up costs: battery % an hour, by night and by day.
3. **`HEART_BEAT`**:
   - Does it deliver anything, with what fields, how often, and with what timestamps?
   - Do the intervals agree with the HR?
4. `PPG`, `SPO2`, `ECG`: does anything arrive, and in what layout?
5. `TOUCH_DETECT`, `MOTION_DETECT`, `ACTIVITY_RECOGNITION`:
   - When do they fire?
   - Does touch say "not worn" on the bedside table?
6. `HEART_RATE_METRICS_DAILY`: when does resting HR change, and does it follow the night?
7. The temperature through a night.
8. With the probe running, do X Training and Race still open and record?
9. In set-up C, how long after reconnecting does the first trusted HR arrive?

**Jon's test:**
- **Seven nights**, the UNA on one wrist and his Garmin on the other.
- Include:
  - at least one nap;
  - an evening with the UNA off on the bedside table for an hour.
- Keep a short diary each day:
  - in bed, lights out, woke, out of bed;
  - naps;
  - caffeine and alcohol.
- Send:
  - the CSVs;
  - the Garmin's sleep, resting HR, HRV status and Body Battery for each day
    (screenshots or an export).
- These nights become the core's first tests.

## Proposed phases

| Phase | Work | Gate |
|---|---|---|
| **SL0** | Decisions; UNA asked; the probe built (CI) | Jon's seven nights; the answers above |
| SL1 | The pure core: window, minute scoring, naps, energy, readiness; host tests on the recorded nights, compared with the diary and the Garmin | Jon agrees the numbers and the energy model's shape |
| SL2 | The resident service (logger, files, the battery budget) and the screens | Two weeks on the wrist |
| SL3 | The glance and the family (`sleep_now.json`; Streak; X Training) | The family check extended |
| SL4 | HRV, stress and HRV-based charging, **only if `HEART_BEAT` delivers** | Against a chest strap overnight |
| SL5 | Store release | As the others |

## Questions for Jon

1. **The name** on the watch, at most 15 bytes (X Energy, Jon's lead, 24 November), and a name
   for the energy score (not "Body Battery"). Some options: Energy, Charge, Reserve,
   Tank.
2. **The battery.** What cost is acceptable for automatic tracking? For example, 10 days
   down to 8 (about 0.25 mA, see "The battery").
3. **The fallback.** If a full-time service is too costly, is a start-at-bedtime Sleep
   activity worth having?
4. **The check-in** (#24a): yes, and which tags?
5. **Ground truth.**
   - Is the Garmin you compared X Run against still available for the probe nights?
   - Does it show sleep, HRV status and Body Battery?
   - Is there a chest strap for an HRV check later (Ride's probe used one)?
6. **UNA first?** Send the questions below before building the probe, or build the
   probe alongside?

## Questions for UNA

1. Are you building sleep tracking, all-day stress or an energy score into the watch?
2. `HEART_BEAT`:
   - Is it delivered on the watch?
   - What is its data layout?
   - Does it give beat times good enough for HRV during sleep?
3. `PPG`:
   - What is its layout and rate?
   - Which lights does the PAH8316LS have? Is `SPO2` real on this hardware?
4. Power:
   - What does optical HR cost at a 1-second period?
   - Does a longer period dim or duty-cycle the light?
   - Does the watch already sample HR all day (for its resting HR), and could an app
     read that history?
5. `TOUCH_DETECT`: is it the worn or unworn signal we should rely on?
6. Is a full-time `APP_AUTOSTART` service acceptable in the store, and will future
   kernels keep allowing it?
7. How much flash may an app use for logs?
8. Could the UNA app sync a non-activity file (a sleep log) to the phone?

## Sources

- **Sleep window, no diary:** van Hees VT et al. (2018) *Sci Rep* 8:12975, "Estimating
  sleep parameters using an accelerometer without sleep diary"
  ([nature.com](https://www.nature.com/articles/s41598-018-31266-z)).
- **Sustained inactivity (5° for 5 minutes):** van Hees VT et al. (2015) *PLoS ONE*
  ([PMC4646630](https://pmc.ncbi.nlm.nih.gov/articles/PMC4646630/)).
- **Actigraphy against PSG (Cole-Kripke):** Marino M et al. (2013) *Sleep*
  ([PubMed 24179309](https://pubmed.ncbi.nlm.nih.gov/24179309/)).
  - Also: GT3X+ and Actiwatch ([PMC5779275](https://pmc.ncbi.nlm.nih.gov/articles/PMC5779275)).
- **Consumer sleep stages against PSG** (2024)
  ([PubMed 38087674](https://pubmed.ncbi.nlm.nih.gov/38087674/)).
- **Sleep duration:** AASM and SRS consensus, Watson NF et al. (2015)
  ([PMC4434546](https://pmc.ncbi.nlm.nih.gov/articles/PMC4434546/)).
- **Sleep Regularity Index:** Phillips AJK et al. (2017); its properties
  ([PubMed 33864369](https://pubmed.ncbi.nlm.nih.gov/33864369/)).
- **Wrist HRV at night against a chest ECG:**
  - Polar Vantage V2 against H10, Nuuttila et al.
    ([University of Jyväskylä](https://jyx.jyu.fi/bitstream/123456789/79291/1/nuuttilaym.pdf));
  - WHOOP, Bellenger et al. (2021)
    ([ACU](https://acuresearchbank.acu.edu.au/download/70f2691b687e5a4e0997266c34d00cec8a8d37a2d38cbc0f8233bfe57a4c56d9/950087/OA_Bellenger_2021_Wrist_based_photoplethysmography_assessment_of_heart.pdf)).
- **Stress and recovery from HRV (the method behind Garmin's):** Firstbeat white paper
  ([firstbeat.com](https://www.firstbeat.com/wp-content/uploads/2015/10/Stress-and-recovery_white-paper_20145.pdf)).
- **Body Battery™ (Garmin's trademark), and that it uses Firstbeat's analytics:**
  ([garmin.com](https://www.garmin.com/en-US/blog/?p=29612)).
- **Battery life claim (a retailer's page, not UNA's):**
  ([heyupnow.com](https://heyupnow.com/collections/brand-una-watch.oembed)).

---

## Gate SL0: the Energy Probe (24 November 2026)

Jon asked for an initial app that records every parameter above, to confirm the
viability of the full app, with the battery drain recorded and some nights set to record
nothing, as a baseline. He starts testing tonight. Jon's steps: `PROBE.md`.

### What it is

- **Energy Probe** (`Tools/Probe`):
  - app folder `EnergyProbe`, a Utility;
  - development APP_ID `939CAA3975888E07` (md5 of the folder name);
  - version 0.1.0;
  - started from the Bike Probe (`hybridx-ride/Tools/Probe`).
- **`APP_AUTOSTART On`**, as the SDK's Alarm example. The service starts at boot and
  after USB mode, and never exits because its screen closed. Only `COMMAND_APP_STOP` ends
  it, after writing the part-row and the state.
- **Five modes** (`Core/Header/ProbeMode.hpp`):
  - **Off**: the battery only, a row every 10 minutes (the baseline);
  - **Full**: every sensor below;
  - **Motion**: the accelerometer only;
  - **Heart**: `HEART_RATE_EX` every second only;
  - **Heart 1 in 5**: heart rate for the first minute of every five.
- **The nights:**
  - The night window defaults to 22:00–08:00, with presets 21–07, 23–09 and 00–10.
  - A night belongs to its evening's date.
  - The rotation runs from the first night seen: **Full, Off, Full, Off, Motion, Heart,
    Heart 1 in 5, Off**, then repeats.
  - The Tonight page overrides one night.
  - Days are Full (a setting makes them Off), so naps and rest are recorded.
- **Full connects:**
  - `ACCELEROMETER` at 100 ms with 1 s batches (`ACCELEROMETER_RAW` if that fails);
  - `HEART_RATE_EX` 1000/1000;
  - `HEART_BEAT` (0/1000);
  - `AMBIENT_TEMPERATURE`, `TOUCH_DETECT`, `MOTION_DETECT`, `ACTIVITY_RECOGNITION` and
    `WRIST_MOTION` (0/0);
  - `HEART_RATE_METRICS_DAILY`, `STEP_COUNTER_DAILY`, `ACTIVITY_TIME_DAILY` and
    `FLOOR_COUNTER_DAILY`;
  - in every mode, `BATTERY_LEVEL`, `BATTERY_CHARGING` and `BATTERY_METRICS` at 60 s
    (10 minutes when Off).
  - Periods and latencies are in ms, as the Bike Probe's and RunLVGL's watch-proven
    connections.
- **The sensor check** (on demand, 2 minutes):
  - asks for each of 21 types: `RequestDefault`, `RequestList` and `RequestGetDesc`;
  - connects those the logger doesn't already hold, and leaves the logger's own alone;
  - counts messages, samples and fields, and keeps the first three samples' first four
    fields as floats and hex;
  - this is where `HEART_BEAT`, `PPG`, `SPO2` and `ECG` get their first answer.

### The files (`Apps/EnergyProbe/Logs/`)

- **`min_<yyyymmdd>.csv`** (`MinuteEpoch`), 60 columns, integers only:
  - battery % ×100, charging and USB, mV, mA ×100, average mA ×100, mAh ×10;
  - wakes (messages a row);
  - motion: accelerometer samples, mean magnitude ×100 (to settle the unit), ENMO in
    mg ×10, **twelve 5-second arm angles** ×10 (van Hees), the angle change;
  - heart rate: samples, trusted samples (Race's gate), min, mean ×10, max, mean trust,
    optical-zero count, source, seconds to the first trusted reading;
  - the watch's daily AHR, RHR, steps, active time and floors;
  - temperature;
  - touch and its changes, motion-detect counts, activity class, wrist raises;
  - beats: count, RR mean, RMSSD ×10, rejected RR.
- **`nights.csv`** (`NightSummary`), a row per night:
  - battery at both ends, hours, **drain %/h ×100**;
  - the mAh drop and the mean current from it;
  - the fuel gauge's mean average current;
  - rows, worn minutes, heart-rate minutes;
  - flags: charged, restarted, partial, no battery, mode changed (Tonight changed while the night was on).
- **`events.csv`**, **`check_<utc>.csv`**, **`beats_<yyyymmdd>.csv`** (raw beats, first
  10 minutes of each hour) and **`state.txt`** (`StateFile`: the settings, the first
  night, the override, the open night's sums, the last 7 nights).
- **Writing:**
  - Rows are buffered and appended every 10 rows (open, seek to the end, write, close, as
    X Weigh-in's LogStore).
  - The state is written to `state.new`, then swapped in.

### Sizes

- The watch build (local compile check, not for a watch) is 214 KB.
- The service has 31 KB of code and 16 KB of data and bss, plus its 10 KB stack. That is
  what stays resident.

### Verified (here)

- **Host tests:** 39 (`Tests/Host`): schedule, rotation and override, civil dates, the
  minute row, the night's drain, the state file, fixed-point text.
- **Simulator:** rows written each minute; absent sensors listed in `events.csv`; the
  pages captured; R1 changes the override, window and daytime; the sensor check writes
  its file and lists 21 lines.
  - The GUI's queue holds ten messages, so the check lines go five to a message.
  - The simulator has no accelerometer, beat, temperature, touch, event or daily sensor,
    so they read as absent.
  - Closing the simulator's screen ends the program, so "logging continues with the
    screen closed" is for the watch to show.
  - **A night, end to end** (`docs/experiments/night_check.sh`, the local time moved with
    `TZ`):
    - the night opened at 22:00;
    - a restart at 07:58 the next morning carried it on (flagged restarted);
    - 08:00 closed it into `nights.csv`, the state's history and the Nights page.
  - Screens: `docs/screens/sl0-*.png` (`capture_screens.sh`).
- **`probe_report.py`:** run on the simulator's logs, and on a synthetic folder (four
  nights, one charged, a 10-hour night of angles). It found the made-up sleep window,
  23:00 to 06:55, and left out the charged night.
- **Local watch compile check** (Trail Quest's recipe).
- **CI**'s build is the one to install.

### What Gate SL0 must answer (Jon's eight nights)

1. Does the service log all night, every night, with no screen?
2. Each mode's drain against the Off nights (`probe_report.py`).
3. The sensor check: `HEART_BEAT`, `PPG`, `SPO2`, `TOUCH_DETECT` and the daily figures.
4. How the watch's temperature, touch and motion events behave overnight, and off the
   wrist on a bedside table.
5. Whether X Training and Race still open with the probe resident.
6. The first look at each night's sleep window and lowest heart rate, against Jon's
   Garmin and his notes.


### First run on the watch (Jon, 24 November 2026, before the first night)

Jon installed the CI build, opened it, ran the sensor check, then plugged the watch in to
copy the files.
- The files are those of a single ten-minute session (`min_20261124.csv`, 10 rows), plus
  `events.csv`, `check_1795523327.csv` and `state.txt`.
- `probe_report.py` reads them.

**The probe itself works on the watch:**
- **It autostarts.**
  - "start 1" came 21 s after boot.
  - After USB, it started again by itself ("start 2" at 456 s of uptime). It was stopped
    with `COMMAND_APP_STOP` when the cable went in.
- **It stays resident.** The screen was closed and reopened four times with no new start:
  the same service carried on.
- **The rows are complete.** Every minute has accelerometer data (600 samples, 10 Hz), heart
  rate, temperature, touch and the battery gauge.
- **The service wakes 120 to 300 times a minute in Full.**

**"Sensors failed: 0x10" is the beat sensor, and it is absent, not failing:**
- `HEART_BEAT` has no handle on the watch (the check: handle 0).
- Nor do `PPG`, `SPO2`, `ECG` and `GESTURE_RECOGNITION`.
- **So this watch gives no beat-to-beat timing, no raw PPG and no SpO2 through the SDK.**
- That settles the desk study's biggest question for now:
  - #14 (overnight HRV), #15 (breathing rate), #16 (SpO2) and #22 (HRV stress) are **No**
    unless UNA adds them;
  - stress stays the heart-rate proxy (#23);
  - the energy score charges on heart rate, not HRV.
- It goes to UNA as a question (Questions for UNA, 2-3).

**What the sensors are** (from their descriptions):
- **Accelerometer:** "BMI270 accelerometer", in **m/s²**. It reads 9.78 at rest, as the
  current SDK documents.
  - `ACCELEROMETER_RAW` sends three fields that do not read as floats. Its parser reads
    int16s; we do not use it.
- **Temperature:** "Barometer temperature", the MS5837's own sensor inside the watch. That
  fits X Sauna's finding of a lagging internal temperature near the skin.
  - It reads 30.2 °C, falling to 28.3 °C over ten minutes off the charger.
  - It arrives at 1 Hz.
- **Heart rate:** optical (source 1), trust about 2.7, 52 to 56 bpm at rest.
  - The first trusted reading came 27 s after connecting (18 s after the restart).
- **The watch's daily figures** arrive when connected, then only on a change:
  - AHR 56.0 and **RHR 41.0** bpm;
  - 1812 steps, 11 active minutes, 4 floors.
- **Touch, motion and wrist:**
  - Touch said "on the wrist" (1).
  - `MOTION_DETECT` events arrive.
  - `ACTIVITY_RECOGNITION` and `WRIST_MOTION` sent nothing in ten still minutes.
- **The battery gauge** (`BATTERY_METRICS`) reports, each minute:
  - volts;
  - the current and its average (negative is discharge);
  - **remaining capacity in mAh**.
- **Battery level** arrives only on a change. It stayed at 66.53 % for six minutes.

**The battery is about 132 mAh, not 400.**
- The gauge read 88.0 mAh at 66.53 %, and 92.5 mAh at 69.89 % after charging. Both give
  132 mAh for 100 %.
- So the SDK diagram's "400 mAh" (desk study, "The battery") is wrong for this watch.
- With the retailer's 10 days, the whole watch averages about **0.55 mA**, not 1.7.
- Revised "what an extra drain would cost":

  | Extra average current | Days of everyday use (132 mAh, 0.55 mA baseline) |
  |---|---|
  | none | 10 |
  | 0.25 mA | 6.9 |
  | 0.5 mA | 5.2 |
  | 1 mA | 3.5 |
  | 2 mA | 2.2 |

- With the screen on and the check running, the gauge's average current read 2 to 8 mA.
  That includes the screen, so it is not the probe's cost: the first night, screen off,
  gives that.
- But the margin is far tighter than the desk study assumed.
- The mAh reading (0.5 mAh steps or finer) measures the drain much more finely than the
  %, and the Off nights' baseline matters even more.

**For the next build** (not before the first nights: this one works):
- show absent sensors as "not on this watch" rather than "failed";
- log the gauge's design capacity once;
- stop asking for `HEART_BEAT` (and leave it to the check).

### Nights 1 and 2 (Jon's logs, 26 November 2026)

Jon sent the Logs folder after two nights:
- Tue 24, **Full**;
- Wed 25, **Off**;
- the days of the 25th and 26th, Full.

Read with `probe_report.py`; its sleep look now takes each night whole, across midnight.

**The logger:**
- every row of both nights is there;
- the Off night wrote a row every 10 minutes with the battery only, as designed;
- the night summaries, the rotation (Full, then Off) and the Nights page all worked.
- **One unexplained reset at 22:32 on the 24th**:
  - "start 11" came 10 s after a boot, with no `stop` before it, so the watch reset rather
    than closing the app (or Jon restarted it; to be asked).
  - The six rows in the RAM buffer (22:26 to 22:31) were lost, as designed.
  - The night carried on, flagged restarted.
- The afternoon of the 24th has three more boots with no stop (starts 5, 6 and 8): Jon
  installing and restarting.

**Night 1 (Full), from the rows** (`probe_report.py`; to be set against the Garmin and
Jon's notes):

| Hour | Heart rate (mean) | Arm still | ENMO, mg | Watch temp, °C |
|---|---|---|---|---|
| 22:00 | 50.0 | 33 % | 20 | 29.1 |
| 23:00 | 45.4 | 85 % | 8.7 | 32.8 |
| 00:00 | 44.4 | 90 % | 3.9 | 31.0 |
| 01:00-04:00 | 45 to 52 | 87 to 92 % | 3.5 to 4.4 | 28.9 to 30.3 |
| 05:00-07:00 | 44.7 to 46.0 | 78 to 90 % | 5.6 to 6.2 | 30.5 to 31.7 |
| 08:00 | 74.6 | 15 % | 59 | 26.1 |

- **Asleep about 22:59 to 07:59** (the crude window), 87 % still inside.
  - 07:59 is the end of the night window, so the true wake may be later; the 08:00 hour is
    plainly up and about.
  - The lowest 30 minutes of heart rate averaged **42.3 bpm**. The watch's own RHR is 41.0.
- **Heart rate touched exactly 40 bpm in 185 of 594 minutes.** 40 is the most common
  minute minimum by far, which looks like **a floor in the watch's heart rate**. Jon's
  Garmin's overnight low will tell.
- **The night and the day separate cleanly on every channel:**
  - heart rate: 44 to 52 at night, 57 to 85 by day;
  - arm still: 78 to 92 % at night, 2 to 22 % by day;
  - ENMO: 3.5 to 6 mg at night, 20 to 65 mg by day;
  - significant-motion events: 0 to 4 an hour at night, 50 to 450 by day;
  - activity-recognition events: about 0 at night, 10 to 250 by day;
  - the watch's temperature: 29 to 33 °C at night, 23 to 28 °C by day.
- `WRIST_MOTION` fired 2 to 22 times an hour at night: turning over, perhaps a
  restlessness signal.
- Touch stayed "on" all night. It flickered once during the evening's activity on the 25th.

**The battery, so far:**
- **Full costs about 0.85 to 0.95 mA, whole watch, day or night at rest.**
  - Night 1: 8.5 mAh in 10 h (0.70 %/h).
  - The morning of the 26th: 5 mAh in about 5.3 h.
- **The Off night is not a clean baseline yet.** It came after a hard evening (18:00 to
  20:00 on the 25th):
  - heart rate 101 to 124 and 14.7 °C, so outdoors, almost certainly with GPS;
  - 14 % gone in two hours, at 6 to 14 mA.
  - The gauge's remaining capacity then *rose* from 81.0 to 83.0 mAh in the first hours of
    the Off night (recovery after the load), before falling to 79.0 by 08:00.
  - So "2.0 mAh in 10 h (0.20 mA)" understates the baseline. The second half alone gives
    about 0.6 mA.
  - **Full's extra cost is somewhere between 0.2 and 0.65 mA** until a clean Off night pins
    it down. Jon is asked to keep the evening before an Off night free of long GPS
    sessions, or to note them.
- **The gauge is coarse:**
  - % moves in whole steps;
  - mAh moves in 0.5 steps and drifts after a heavy load.
  - So: whole nights only, never an hour.
- **The capacity, from % and mAh together:** 122 to 125 mAh on the 24th to 26th, 132 on
  the first run. "About 125 mAh" is the working figure; the design-capacity field, logged
  in the next build, will settle it.
- **What it means, provisionally:** with Full on all the time, the watch at rest draws about
  0.9 mA, which is about 5.8 days on a charge with no other use. GPS sessions come on top.
  The Motion and Heart nights will show which part costs what.

**Jon's answers (26 November 2026):**
1. **The 22:32 reset was an app update** Jon installed before bed, not a crash. So the
   probe has had no unexplained reset.
2. **The 25th's 18:00 to 20:00 session was an interval run in X Training** (GPS).
   - **X Training opened, recorded and saved with the probe resident.** That answers
     Gate SL0's memory question (5) for X Training; Race is still to try.
   - The probe logged straight through it, with the same optical heart rate, 101 to 124
     bpm.
3. **No Garmin on these nights.** For now the sleep windows are checked against Jon's own
   bed and wake times, not a second device. A night or two with the Garmin is still worth
   having before SL1 tunes anything.
4. **Friday's long run moves earlier in the day**, so Friday's Off night should give a
   cleaner baseline.

**For the next build** (after the week; nothing changes on Jon's watch mid-test):
- **A row filed under 1970:** `stop()` writes the part-row without setting the buffer's
  date. It happened once: `min_19700101.csv`, one row.
- **Worn minutes on an Off night:** they count the touch state carried over from the day,
  though touch is not connected. Clear it when the events group disconnects.
- **Absent sensors** as "not on this watch", not "failed"; and stop asking for
  `HEART_BEAT`.
- **The gauge's design capacity**, logged once.
- **Flushing more often** at night (5 rows), since a reset loses what is buffered.
