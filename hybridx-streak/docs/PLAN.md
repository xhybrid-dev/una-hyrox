# HybridX Streak — Plan

A weekly-target streak tracker for the UNA Watch that counts **every activity
the watch records, from any app, automatically**. This plan turns Jon's brief
(`UNA_STREAK_TRACKER_BRIEF.md`), plus his clarification of 24 September 2026
(NOTES, "Jon's clarification"), into something buildable against what the SDK
actually offers. The evidence for every platform claim is in `NOTES.md`,
§E.1-E.9.

**Status: proposal, revised 24 September 2026. Nothing is built.** Decisions
for Jon are in §12. The project starts with a probe on Jon's watch (§3), because
one unverified platform behaviour decides whether it can exist. HybridX Race's
Gate 6 hardware testing is not touched by any of this.

---

## 0. In one screen

- **Automatic first.** When the app or its glance opens, the watch's
  activity folders are scanned for new activities, each one is read for its
  sport, date and duration, and every qualifying session counts towards the
  week. Running, cycling, hiking, a workout, a HYROX sim: no logging needed.
  **Manual logging stays, as the fallback** for training the watch didn't
  record.
- **Independent of HybridX Race.** Race is just one more app whose activities
  get counted. The two share nothing they need. What they *can* share is a
  bonus in each direction (§9).
- **It hinges on one hardware fact.** Can an app read other apps' activity
  files on a real watch? The SDK's evidence leans yes (NOTES E.9), but nothing
  proves it. **S0 is a go/no-go probe on Jon's watch before any feature
  work** (§3).
- **Two packages**, for the reason in NOTES E.1. `HybridX Streak` is a
  `Utility` with an LVGL GUI; `Streak Glance` is a `Glance` that shows "2/3 this
  week · 7-week streak".
- **No background process.** Nothing can wake an app on a schedule (NOTES
  E.2). Scans happen when the app or glance opens. Checking the glances screen
  is enough to bring the week up to date.
- **Same repository**, in a new `hybridx-streak/` folder (§1).

---

## 1. Same repository or a second one?

**Recommendation: same repository, new top-level folder** (the brief's S4
default). Jon's clarification removes the one *hard* link I had proposed
between the apps: Race no longer needs to write a ledger for Streak, because
Streak reads Race's activities like anyone else's. The case rests on shared
infrastructure instead:

| | Same repo | Separate repo |
|---|---|---|
| SDK checkout and pin | **One** clone, one pin, one bump | Two, kept in step by hand |
| FIT expertise and test rig | Streak's FIT *reader* is tested against Race's FIT *writer* rig (`docs/experiments/fit_race_sample.cpp`) and `fitdecode`, with no copying | Copied tooling |
| Brand assets, LVGL scaffolding, packaging script, host-test harness | Shared directly | Copied |
| Optional Race ↔ Streak extras (§9) | A single small JSON format, in one place | Duplicated |
| `CLAUDE.md` standing rules | One file, a section per app | Two to keep aligned |
| Version tags | `apps-v*` is repo-wide (`una-app.cmake:149`). Streak uses `streak-v*` and passes `BUILD_VERSION` in (`una-app.cmake:141`). Our CMake only, no SDK edit | Independent |
| Licence | Root MIT covers everything. Awkward only if one app goes closed-source | Independent |

UNA made the same call for its own apps (NOTES E.7). **Split only if the two
apps end up on different licences** (S12). `git subtree split` moves the folder
out later without losing history.

```
CLAUDE.md                     gains a "HybridX Streak" section
README.md                     becomes an index of both apps
hybridx-race/                 unchanged
hybridx-streak/
  Resources/                  icons, app-manifest.json (x2)
  Software/
    Libs/                     ActivityScanner, FitSessionReader, StreakModel,
                              Service, persistence
    Apps/HybridXStreak-CMake/        main app (Utility)
    Apps/HybridXStreakGlance-CMake/  glance (Glance), same Libs
    Apps/LVGL-GUI/            GUI + simulator
  Tests/Host/
  docs/
shared/brand/                 Jon's mark, source + sized icons
una-sdk/                      not committed, one clone for both
```

Race's GUI scaffolding is **copied** into Streak as a starting point, not made
into a shared library, so the two apps' release cycles stay independent.

---

## 2. The product

The athlete sets a weekly target (default 3 sessions, week starting Monday).
Every activity they record on the watch counts automatically, if it meets the
qualifying rules (long enough, the right type). Hitting the target achieves
the week. The streak is a count of consecutive achieved weeks. Grace weeks,
earned by achieving, can cover a missed week, but only when the athlete chooses
to spend one. Lifetime totals never reset.

| MVP | Later (P1+) |
|---|---|
| **Automatic counting from every app's recorded activities** | An opt-in background scanner, **only** if S0 shows the phone deletes files after sync (§5.6) |
| Qualifying rules: minimum duration (default 10 min), scope (any, or one type) | Excluding chosen apps; per-type minimums |
| Manual log for unrecorded training: type, today or yesterday | Timing a manual session on the watch |
| One goal: target 1–7, week start | Multiple goals; rolling 7-day periods |
| Weekly ring, streak, grace stock, bead chain; "this week" list showing where each session came from | Phone-side history |
| Boundary screen: week achieved, or "use a grace week?" | Reminders (§10) |
| Badges, personal-best week, trophy case | |
| Undo a manual log; exclude a mis-counted activity | |
| Glance (second package) | |
| Settings on the watch and from the phone (AppConfig) | |

The minimum duration is in the MVP now, where the brief had it optional. With
automatic counting, a 40-second recording started by accident would otherwise
count as a session.

---

## 3. S0: the go/no-go probe

Before any feature work, a throwaway `Utility` app. Jon installs it, opens it
once, and copies one log file back over USB. It records:

| # | Question | Why it matters |
|---|---|---|
| 1 | Can `../<OtherApp>/Activity/` be **listed** and a `.fit` in it **opened for reading**? | **The whole product.** Also tries `../SharedData/` as a control |
| 2 | What is in `/Apps/`: which apps, and do the built-in ones use `Activity/YYYYMM/activity_*.fit`? | Confirms the layout the scanner expects on shipping firmware |
| 3 | How long does parsing the `session` out of a real ~1-hour `.fit` take? | Sizes the scan for the glance, which must feel instant |
| 4 | After a phone sync, are the synced `.fit` files still on the watch? | Decides whether "scan when opened" can miss activities (§5.6) |
| 5 | What `width`, `height` and `maxControls` does `RequestGlanceConfig` report? | Glance layout (§8) |
| 6 | Does `rename` over an existing file fail, as FatFs's does? | Confirms the save sequence (§6.5) |

**Gate 0.** If 1 passes, build as planned. If 1 fails, the watch firmware
blocks what the product needs, and there are three honest options, none of
which is a workaround:

- Ask UNA for read access, or for an activity-list API. The kernel already
  "auto-registers the .fit the moment its FileGuard::close() fires"
  (`hybridx-race/Software/Libs/Sources/ActivityWriter.cpp:383-384`, a comment
  inherited from the SDK template), so exposing that list would be enough.
- Ship manual-only, plus automatic counting of HybridX apps that opt in by
  writing to `../SharedData/` (proven to work, NOTES E.3).
- Pause the app.

The simulator allows cross-app reads regardless (NOTES E.9), so S1 and S2 can
run in parallel with waiting for Jon, **as long as nothing is shipped before
Gate 0.**

---

## 4. Architecture

```
            ┌─────────────── HybridX Streak (.uapp, Utility) ──────────────────┐
 buttons ─► │ GUI (LVGL) ◄─ msgs ─► Service                                     │
            │                        ├─ ActivityScanner ─► FitSessionReader     │
            │                        ├─ Classifier (sport, then app folder)     │
            │                        └─ StreakModel (pure C++) ─ single writer ─┼─► state.json (private)
            └───────────────────────────────────────────────────────────────────┘   streak.json (SharedData, public)
                     reads                                                          ▲
   /Apps/*/Activity/YYYYMM/activity_*.fit  ◄──── Running, Cycling, Hiking, Workout, │
                                                  HybridX Race, any SDK-template app│
            ┌─────────────── Streak Glance (.uapp, Glance) ────────────────────┐    │
 glances ─► │ read streak.json ─► scan only files newer than it ─► project to now ├──┘
 screen     │ (same Libs; read-only; never writes)                              │
            └──────────────────────────────────────────────────────────────────┘
```

- **Single writer.** Only the main app's service writes state. The glance runs
  the same scanner and model to *project* the week to now, but commits
  nothing, so the two can never race over a file.
- **No resident service, no sensors.** The main service exits once its GUI
  has gone (`service-lifecycle.md` §5.2). The glance exits on
  `EVENT_GLANCE_STOP`. Nothing runs while the watch is idle.
- **Race's embedded rules carry over:** fixed-size messages under the
  256-byte pool, no heap churn, bounds checks, integer formatting, and fixed
  buffers in the FIT reader.

---

## 5. Automatic detection

### 5.1 Discovery (`ActivityScanner`)

1. List `..` (that is, `/Apps/`). For each folder except our own and
   `SharedData`, look for `Activity/`.
2. In each `Activity/`, list only the `YYYYMM` folders at or after the
   **high-water mark**, the newest activity already processed. That's usually
   one folder, two at a month end.
3. Take `activity_YYYYMMDDTHHMMSS.fit` files newer than the high-water mark.
   Skip the file named in that folder's `.recording` marker (NOTES E.9); it's
   still being recorded.
4. Bounded: at most N files per scan (say 32), oldest first, and the rest the
   next time. A first-ever scan only looks back to the start of the current
   week. **History before the app was installed is not back-counted** (S15).

### 5.2 Reading (`FitSessionReader`)

A streaming parse of one file with a fixed 512-byte buffer. It checks the
header, keeps a definition table for the 16 local message types, skips every
data message except `session`, and extracts StartTime, Sport, SubSport and
TotalTimerTime, all SDK-declared field numbers (NOTES E.9). It checks the CRC.
A file with no `session`, a bad CRC or a short read is **rejected, not
counted**, and retried on the next scan in case it was mid-write. It's
host-tested against real output from Race's writer rig, fixtures from each SDK
example writer, deliberately truncated and corrupted files, and `fitdecode`
reading the same bytes.

### 5.3 Classification

| FIT sport (SDK enum) | Streak type |
|---|---|
| Running (any sub-sport, Treadmill included) | Run |
| Cycling | Ride |
| Walking, Hiking | Walk / Hike |
| Training | Strength |
| Generic | by app folder: `HybridXRace` → **HYROX**, `Workout` → Workout, otherwise Other |
| Anything outside the SDK enum | Other |

The app folder beats the sport for apps we know. Race writes Running/Generic
(D2), but a HYROX sim is shown as HYROX.

### 5.4 Qualifying, and the date that counts

- **Qualifies if** TotalTimerTime is at least the minimum duration (default
  10 min, S13) and the type is within the goal's scope.
- **The date is the local date in the filename.** It is the local start time
  as it was when the activity was recorded (NOTES E.3), so a later time-zone
  change cannot move it into another week. StartTime from the `session` is the
  cross-check; if the two disagree by more than a day, the file is skipped and
  logged.
- **Several a day all count** (the brief counts sessions), unless the athlete
  turns on "count one per day" (S14).

### 5.5 Deduplication

Each activity's key is `<app folder>/<filename>`. That's unique, because the
filename is the start time to the second, and the folder is the app. Keys
counted in the current and previous week are kept in a bounded ring (64
entries). Together with the high-water mark, a file is never counted twice,
even after a crash between "counted" and "saved": the high-water mark and the
ring are saved in the same atomic write as the counts.

### 5.6 Retention: what if the phone deletes synced files?

If S0 question 4 shows synced files vanish from the watch, an activity
recorded and synced between two looks at the streak is never seen. Options, in
order:

1. Nothing, if files stay after a sync. Unknown until S0 checks it.
2. The glances screen is looked at often, which shrinks the window but doesn't
   close it.
3. An **opt-in** background scanner: autostart, hourly bounded wait, no
   sensors. It costs what NOTES E.2 describes. P1, and only if needed.
4. Ask UNA for a retention setting or an activity-list API (§12).

### 5.7 Manual logging (the fallback)

For training the watch didn't record: pick a type (Run, Strength, Row/Erg,
Walk, HYROX, Other) and today or yesterday (within this week). It counts the
same as an automatic session, marked "manual" in the week list. Undo is
available while the week is current. Duplicates are avoided in two ways: a
manual log is never matched against automatic ones (they are different
sessions by definition), and the week list shows both, so a mistaken manual
entry is easy to spot and undo.

---

## 6. The core model: `StreakModel`

Pure C++, with no SDK types. It takes "now" as an argument and is covered by
host tests. Both packages link it.

### 6.1 Dates

- Every session carries a **local day number** (days since 1970-01-01, local
  calendar), from the filename for automatic sessions and from `localtime_r`
  at log time for manual ones.
- Period for a week starting on weekday `s` (0 = Sunday … 6 = Saturday):
  `period = floorDiv(localDay + 4 - s, 7)`. That's integer only, because
  1970-01-01 was a Thursday. **Verified**: for every one of the 7 start days,
  across 900 days spanning 29 February 2024 and two year ends, the period
  changes exactly on the chosen weekday and nowhere else.
- **Clock guard:** with UTC earlier than 1 January 2026, the watch has lost its
  time. Refuse to evaluate, and say "Set the time from your phone". Scanning
  still works, because filenames carry their own dates.
- **Monotonic:** `lastEvaluatedPeriod` never goes backwards.

### 6.2 Order of work on every open

**Scan → credit each session to its own week → evaluate boundaries.** A Sunday
run found on Tuesday counts towards *last* week before last week is judged.
Committed weeks are final. The only way a session can arrive after its week
has been committed is a crash-recovered recording, which is rare enough to
record in history without reopening the week.

### 6.3 Goal and state

```
struct Goal {
    uint8_t  targetCount;      // 1..7, default 3                     (S1)
    uint8_t  weekStart;        // 0..6, default 1 = Monday             (S1)
    uint8_t  scope;            // Any, or one StreakType               (S2)
    uint8_t  minMinutes;       // default 10; 0 = off                  (S13)
    bool     onePerDay;        // default false                        (S14)
};
```

The state is as in the first draft: current period, counts, streak, grace,
longest, lifetime totals, best week, badges, a pending grace decision and a
52-week history ring. Added now: the scan high-water mark, the dedup ring,
and this week's session list (type, source app, minutes, auto/manual), capped
at 16.

### 6.4 The boundary rules

1. Target met → **Achieved**: streak +1; one grace week per 4 achieved weeks,
   capped at 2 (S3).
2. The first week of a new goal → **Trial**: counts if met; a miss doesn't
   break the streak (S9).
3. Missed with streak 0 → **Missed**, and no prompt.
4. Missed with streak > 0 → one pending decision covering every missed week in
   the catch-up. With enough grace weeks: "Use 2 grace weeks to keep your
   9-week streak?" Without: the streak resets, and the screen explains why
   (S6).

Goal changes apply from next week. A week-start change closes the current week
early, as achieved or void (S8). Badges are on cumulative weeks achieved (4,
12, 26, 52) and lifetime sessions (10, 50, 100, 250) (S10). The personal-best
week is celebrated once per week.

### 6.5 Persistence

A JSON state file under 4 KB, saved with the SDK's own crash-safe sequence:
write `.tmp`, flush, rotate to `.bak`, rename into place
(`RecordingMarker.cpp:70-110`). The reader falls back to `.bak`. The public
`../SharedData/HybridX/streak.json` for the glance (and Race, §9) is written
the same way.

### 6.6 Host tests (Gate 1)

- Period math for all start days, over a year end and 29 February.
- The clock guard, and time going backwards.
- Scan → credit → evaluate ordering.
- Dedup across a crash between count and save.
- High-water marks across a month end.
- The `.recording` skip.
- The FIT reader against real writer output, truncated files, a bad CRC, no
  session, and every SDK example's sport.
- Every classification row in §5.3.
- Minimum duration and one-per-day.
- Catch-ups of 0, 1 and 5 missed weeks with 0–2 grace weeks.
- The trial week; goal and week-start changes; badges; the history wrap.
- Persistence: round trip, truncated file, corrupt file.

---

## 7. Screens

Buttons match HybridX Race (L1 up, L2 down, R1 select, R2 back/exit).

| Screen | Shows | Buttons |
|---|---|---|
| **Home** | Arc "2 / 3", streak "7 weeks", grace shields, bead row of the last 8 weeks. A one-line toast if new activities were just found: "+1 Run (Running, 42 min)" | R1 this week · L1/L2 menu · R2 exit |
| **This week** | Each session: type, minutes, source ("Running", "HybridX Race", "manual"), and anything seen that didn't qualify, greyed out with the reason ("6 min — under 10") | R1 on a manual entry: undo. On an automatic one: exclude |
| **Boundary** | "Last week 3/3 ✓ — 8-week streak", or the grace offer | R1 use / continue · R2 let it go |
| **Log manually** | Type wheel, then today / yesterday | R1 next / confirm · R2 back |
| **Celebration** | "Week done!", "Best week yet", "Badge: 12 weeks" | auto-dismiss after 2 s |
| **Menu** | This week · Log manually · Trophy case · History · Settings | |
| **Settings** | Target, week start, counts (any / one type), minimum minutes, one per day, reset streak (hold) | |

Showing *what didn't count and why* is what makes automatic counting feel
trustworthy rather than mysterious. The LVGL pool is measured at Gate 3, a
lesson from Race's 91% peak.

---

## 8. The glance

Service only, from the GlanceSteps / GlanceActivity pattern.

- On `EVENT_GLANCE_START`: `RequestGlanceConfig`, read `streak.json`, scan only
  files newer than its high-water mark (usually none, sometimes one), project
  to now, draw. Nothing is written.
- **Full layout** if the control budget allows: Jon's X mark · "2/3 this week"
  · "7-week streak" · 8 rectangles as beads (teal achieved, grey missed, outline
  for the current week). **Fallback:** text only, "2/3 · 7 wk".
- **Empty state:** "Open HybridX Streak to set a goal".
- Its colours come from the glance's 16 (`GlanceControl.h:46-61`).
- If S0 shows parsing a file is slow, the glance counts new files by filename
  and marks the total "provisional", and the app does the full check.

---

## 9. HybridX Race and Streak — the bonus, both ways

Neither app needs the other. Each extra is optional and does nothing when the
other app isn't installed.

- **Race → Streak: automatic, no Race change.** Race's activities sit in
  `/Apps/HybridXRace/Activity/` like any other app's, and the classifier labels
  them **HYROX** rather than Run (§5.3). A full race and a half both count as
  one session.
- **Streak → Race (after Race v0.1.0).** Race's Finished screen reads
  `../SharedData/HybridX/streak.json` if present and adds one line: "3 of 3
  this week · 8-week streak". It's a single file read and a few lines of Race
  code, done after its hardware release so Gate 6 isn't disturbed.
- **Shared engineering.** The FIT reader is tested against Race's writer rig,
  and both apps share the brand assets, SDK pin and build tooling.
- **Later, together with HybridX coaching:** the streak summary is a natural
  thing for the HybridX platform to show, via the Strava route in Race's brief
  §16.

---

## 10. Reminders — P1, opt-in

The only way to buzz "one to go" on a Sunday evening is Alarm's pattern:
autostart, a resident service with a bounded wait, then
`REQUEST_APP_RUN_GUI`. It costs a permanent thread and RAM, and the kernel
doesn't promise to keep it resident (NOTES E.2). If it ships, it is off by
default and wakes at most hourly. It would also do §5.6's background scan, if
S0 shows that is needed.

---

## 11. Build phases and gates

Race's gated shape applies: plan mode at the start of each phase, both builds
after every change, and a stop at each gate.

| Phase | Work | Gate |
|---|---|---|
| **S0** Probe and scaffold | The §3 probe; `hybridx-streak/` skeleton; both CMake projects; `streak-v*` versioning; `CLAUDE.md` section | **Go/no-go**: Jon runs the probe and sends the log |
| **S1** Pure core | `FitSessionReader`, `StreakModel`, the classifier, and the scanner over an injected file system. Can start before Gate 0 reports | Host tests (§6.6) green |
| **S2** Service | Scanning, persistence, the public summary, AppConfig fields | Simulator, with fixture app folders beside its sandbox holding real `.fit` files from Race's rig and the SDK writers: open Streak, see them counted, dated and classified |
| **S3** GUI | §7; screenshots of every screen; LVGL pool peak | Jon reviews |
| **S4** Glance | §8, both layouts, projection | Simulator if it runs glances, otherwise hardware |
| **S5** Packaging and docs | Two manifests, two zips, README, ARCHITECTURE, CHANGELOG | Both validate; zips match `deploy.md` |
| **S6** Hardware and release | On-watch: activities from each built-in app, a real Monday roll-over, a time-zone change, a flat battery, a phone sync | Jon's pass; tag `streak-v0.1.0` |
| **P1** | Race's streak line (§9); reminders / background scan if needed (§5.6, §10); excluded apps; multiple goals | Per item |

---

## 12. Decisions for Jon, and questions for UNA

| ID | Decision | Recommendation |
|---|---|---|
| S1 | Default target and period | 3 per calendar week, Monday start |
| S2 | Types | Automatic: Run, Ride, Walk/Hike, Strength, Workout, HYROX, Other (§5.3). Manual: Run, Strength, Row/Erg, Walk, HYROX, Other |
| S3 | Grace weeks in MVP | Yes: 1 per 4 achieved weeks, cap 2 |
| S4 | Same repo or separate | **Same repo, `hybridx-streak/`** (§1) |
| S5 | App name | "HybridX Streak"; the glance is labelled "Streak" |
| S6 | Several weeks missed, not enough grace | Streak resets, with an explanation |
| S7 | Manual back-fill | Today or yesterday, within the current week |
| S8 | Goal changes | Apply next week. A week-start change closes the current week early |
| S9 | First week of a goal | Trial week |
| S10 | Week badges | Cumulative weeks achieved, not streak length |
| S12 | Licence | Decides whether §1 holds. MIT keeps it simple |
| S13 | Minimum duration to count | **10 minutes**, adjustable, 0 = off |
| S14 | Several sessions in one day | All count by default; "one per day" as a setting |
| S15 | Activities from before install | Not counted; the first week counts from install (as a trial week) |

(S11, timing in the first draft, is dropped: automatic sessions carry their
own duration.)

**For UNA**, alongside Race's findings already queued:

1. Can an app read another app's `Activity/` folder: by design, forbidden, or
   undefined? If forbidden, could the kernel's activity registry be exposed?
2. Does the phone app delete activities from the watch after syncing?
3. Do the built-in activity apps use the SDK layout
   (`/Apps/<App>/Activity/YYYYMM/`)?
4. Can one store listing install two `.uapp`s, a main app plus its glance?
5. Is a clockface complication API planned?

---

## 13. Risks

| Risk | Mitigation |
|---|---|
| **The kernel blocks cross-app reads** | S0 before any shipping; the §3 fallbacks; ask UNA |
| The phone deletes synced files | S0 Q4; §5.6 |
| Built-in apps use a different layout | S0 Q2; the scanner is table-driven by folder |
| A third-party app writes non-standard FIT | Rejected safely, never mis-counted; "didn't count" shows it |
| A slow parse makes the glance laggy | S0 Q3; bounded scan; §8's provisional fallback |
| An accidental recording counts | Minimum duration, and exclude on the week screen |
| Clock loss, travel, reboot mid-write | §6.1, §6.5 |
| Two installs confuse users | UNA question 4; the glance's empty state |
