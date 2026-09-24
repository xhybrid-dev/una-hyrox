# HybridX Streak — Plan

A weekly-target streak tracker for the UNA Watch. This plan turns Jon's brief
(`UNA_STREAK_TRACKER_BRIEF.md`) into something buildable against what the SDK
actually offers. The evidence for every platform claim is in `NOTES.md` §E.1-E.8.

**Status: proposal, 24 September 2026. Nothing is built.** Decisions for Jon
are in §10. Nothing starts until he has made them, and HybridX Race's Gate 6
hardware testing is not disturbed by any of it.

---

## 0. In one screen

- **Same repository, separate app folder** (`hybridx-streak/`, beside
  `hybridx-race/`). One SDK checkout, and one shared file format for the day
  Race sessions count towards a streak. Split the repository only if the two
  apps end up on different licences (§1).
- **Two packages.** `HybridX Streak`, a `Utility` app with an LVGL GUI, and
  `Streak Glance`, a `Glance` app that shows this week's progress on the
  glances screen. The SDK does not let one app reliably be both (NOTES E.1).
- **Lazy evaluation everywhere.** The watch cannot wake an app on a schedule
  (NOTES E.2), so every period boundary is worked out when the app or glance
  is opened. The brief assumed this; the SDK confirms it is the intended
  design.
- **MVP = Tier A:** log a session in three button presses, a weekly target
  ring, the streak count, grace weeks offered at the boundary, a bead chain,
  badges and a personal-best week. No timing, no reminders.
- **Tier B, re-planned.** First P1 item: HybridX Race drops a one-line record
  into the SDK's shared folder whenever a race or sim is saved, and the streak
  counts it automatically. That uses a mechanism the SDK itself uses. Reading
  *other* apps' files is a research spike behind a hardware probe (§6).

---

## 1. Same repository or a second one?

**Recommendation: same repository, new top-level folder.** This is also the
brief's own default (S4).

| | Same repo | Separate repo |
|---|---|---|
| SDK checkout and pin | **One.** `una-sdk/` is cloned once, pinned once, bumped once (`NOTES.md` 5.18 in Race shows what a bump involves) | Two clones to keep in step; two sets of setup steps for Jon |
| Race ↔ Streak data contract (§6, B1) | **One header, compiled into both apps**, so it cannot drift | Duplicated or submoduled; drift is caught only on the watch |
| Brand assets, LVGL scaffolding, packaging script, host-test harness | Shared directly | Copied |
| `CLAUDE.md` standing rules | One file, a section per app | Two files to keep aligned |
| Version tags | `apps-v*` is repo-wide (`una-app.cmake:149`). **Fix:** Streak uses its own `streak-v*` tags and passes `BUILD_VERSION` in, which `una-app.cmake:141` supports. A few lines of our own CMake, no SDK edit | Independent tags for free |
| Licence and publishing route | Root `LICENSE` is MIT and covers the whole repo. If one app goes closed-source (D5), per-folder licensing gets awkward | Independent |
| Repo name | `una-hyrox` no longer fits. GitHub renames keep redirects, so this is optional and cosmetic | Clean name |

UNA made the same call for its own apps: it merged `una-apps` into `una-sdk`
(NOTES E.7).

**When to split instead:** if D5 lands differently for the two apps, say Race
open-source under MIT and Streak closed via the portal. Moving
`hybridx-streak/` out later is a `git subtree split`, so choosing the shared
repo now closes nothing off.

Proposed layout:

```
CLAUDE.md                     gains a "HybridX Streak" section
README.md                     becomes an index of both apps
hybridx-race/                 unchanged
hybridx-streak/
  Resources/                  icons, app-manifest.json (x2)
  Software/
    Libs/                     StreakModel (pure C++), Service, persistence
    Apps/HybridXStreak-CMake/        main app (Utility)
    Apps/HybridXStreakGlance-CMake/  glance (Glance), same Libs
    Apps/LVGL-GUI/            GUI + simulator
  Tests/Host/
  docs/                       brief, NOTES, PLAN, later ARCHITECTURE
shared/
  include/HybridX/SessionLedger.hpp   the Race → Streak contract (P1)
  brand/                      Jon's mark, source + sized icons
una-sdk/                      not committed, one clone for both
```

`shared/` holds contracts and assets only. Race's GUI code is **copied** into
Streak as a starting point rather than turned into a shared library. Sharing UI
code would tie the two apps' release cycles together for no gain yet.

---

## 2. The product, and what the MVP leaves out

The athlete sets a weekly target (default 3 sessions, week starting Monday).
Hitting it achieves the week. The streak is a count of consecutive achieved
weeks. Grace weeks, earned by achieving, can cover a missed week, but only when
the athlete chooses to spend one. Lifetime totals never reset.

| In the MVP (Tier A) | Deferred (P1 or later) |
|---|---|
| Log a session: type, when (today / yesterday), confirm | Timing a session, and a minimum-duration threshold, **together** (NOTES E.8) |
| One goal: target 1–7, week start, scope (any, or one type) | Multiple goals; rolling 7-day periods |
| Weekly ring, streak count, grace stock, bead chain of the last 8 weeks | Reminders (need a resident service, §8) |
| Boundary screen: week achieved, or "use a grace week?" | Auto-logging from HybridX Race (§6 B1), then from other apps (§6 B2) |
| Badges (weeks and sessions), personal-best week, trophy case | Phone-side history view |
| Undo the last log | |
| Glance (second package) | |
| Settings editable on the watch and from the phone (AppConfig) | |

Why no timer in the MVP: with no duration threshold, timing only produces a
number nobody uses, and it adds a screen and a running state. Both arrive
together in P1. That keeps the first version to "log, see progress, celebrate".

---

## 3. Architecture

```
            ┌──────────────── HybridX Streak (.uapp, Utility) ───────────────┐
 buttons ─► │ GUI (LVGL) ◄── messages ──► Service ─► StreakModel (pure C++)  │
            │                               │  writes (single writer)        │
            └───────────────────────────────┼────────────────────────────────┘
                                            ▼
             /Apps/HybridXStreak/state.json         private: full state + history
             /Apps/SharedData/HybridX/streak.json   public: small summary for the glance
                                            ▲
            ┌──────── Streak Glance (.uapp, Glance) ─┼───────────────────────────┐
 glances ─► │ Service: read summary ─► StreakModel::project(now) ─► glance form   │
 screen     │ (read-only, never writes)                                            │
            └──────────────────────────────────────────────────────────────────────┘
```

- **Single writer.** Only the main app's service writes state. The glance reads
  the public summary and never writes, so the two can never race.
- **The glance still handles week boundaries.** If a week has rolled over since
  the app was last opened, the stored summary is stale. The glance runs the same
  pure `StreakModel` code to *project* the summary to now ("new week, 0 of 3,
  7-week streak at stake") without committing anything. The commit happens the
  next time the main app opens.
- **No resident service.** The main service exits once its GUI has gone, with
  the startup grace the lifecycle doc requires (`service-lifecycle.md` §5.2).
  The glance service exits on `EVENT_GLANCE_STOP`, as GlanceHR does. Neither
  holds a thread, a sensor or RAM while the watch is idle.
- **No sensors at all** in the MVP.
- **Race's embedded rules carry over:** fixed-size message structs under the
  256-byte pool, integer formatting, no heap churn, and bounds-checking
  everywhere.

---

## 4. The core model: `StreakModel`

Pure C++, with no SDK types. It takes "now" as an argument and is covered by
host tests, like `RaceModel`. Both packages link it.

### 4.1 Dates

- Every event is stamped with a **local day number**: days since 1970-01-01
  in the local calendar, from `localtime_r` at the moment of the event and
  converted with an integer days-from-civil function. We never store a local
  date as UTC and convert it back, so later zone changes cannot move a logged
  session into a different week.
- **Period index** for a week starting on weekday `s` (0 = Sunday … 6 =
  Saturday):

  ```
  period = floorDiv(localDay + 4 - s, 7)      // 1970-01-01 was a Thursday (4)
  ```

  Integer only, with no calendar library and no DST arithmetic. The host tests
  pin it against known dates across year ends and leap years, for all seven
  start days.
- **Clock guard:** if UTC is earlier than a sanity floor (1 January 2026), the
  watch has lost its time. Logging and boundary evaluation refuse, and the
  screen says "Set the time from your phone" (NOTES E.5).
- **Monotonic periods:** `lastEvaluatedPeriod` only ever increases. If "now"
  is in an earlier period (time went backwards), the session is credited to
  `lastEvaluatedPeriod` and nothing is un-rolled.

### 4.2 Goal

```
struct Goal {
    uint8_t  targetCount;   // 1..7, default 3                          (S1)
    uint8_t  weekStart;     // 0..6, default 1 = Monday                  (S1)
    uint8_t  scope;         // Any, or one SessionType                   (S2)
};
enum class SessionType : uint8_t { Run, Strength, RowErg, Walk, Other };  // S2
```

Goal changes take effect **from the next week**, and the screen says so ("from
Monday"). That stops a target being lowered on Sunday night to rescue a week.
One exception: changing the week-start day closes the current week early.
It counts as achieved if the target was already met, and otherwise as *void*,
which neither extends nor breaks the streak (S8).

### 4.3 State

```
struct StreakState {
    int32_t  currentPeriod;        // period being counted
    uint8_t  sessionsThisPeriod;   // qualifying only
    uint8_t  loggedThisPeriod;     // all types, for display
    uint16_t streakWeeks;
    uint8_t  graceWeeks;           // 0..kGraceCap
    uint8_t  achievedSinceGrant;   // towards the next grace week
    uint16_t longestStreak;
    uint32_t lifetimeSessions;
    uint16_t periodsAttempted;
    uint16_t weeksAchievedTotal;
    uint8_t  bestWeekCount;
    uint32_t badges;               // bitmask
    PendingDecision pending;       // grace offer waiting on the athlete
    PeriodRecord history[52];      // ring: count + {Achieved, Graced, Missed, Void, Trial}
    LastLog  lastLog;              // for undo: type, day, period
};
```

### 4.4 The boundary rule

`evaluate(now)` runs on every open and every log. For each period between
`currentPeriod` and `period(now)`:

1. **Target met** → Achieved. Streak +1, `weeksAchievedTotal` +1, and one grace
   week earned per 4 achieved weeks, up to a cap of 2 (S3). Badges are checked.
2. **First week of a new goal** (the athlete set it mid-week) → **Trial**.
   Counts as achieved if met; a miss does not break the streak.
3. **Missed**, with the streak at 0 → Missed. No prompt, nothing to lose.
4. **Missed**, with the streak above 0 → collected into one pending decision
   covering every missed week in this catch-up:
   - enough grace weeks to cover them all → **ask once**: "You missed 2 weeks.
     Use 2 grace weeks to keep your 9-week streak?" Yes → Graced (the streak
     holds, doesn't grow, and graced weeks don't earn grace). No → reset.
   - not enough grace → the streak resets, and the screen explains why rather
     than offering a choice that cannot save it (S6).

While a decision is pending, the home screen shows it first and the glance
shows "Streak at risk". Nothing about the streak changes until the athlete
answers, which is what the brief means by "an explicit, visible choice".

### 4.5 Logging

- **When:** today, or yesterday while yesterday is still in the current week.
  Back-filling across a boundary is refused, because that is how a streak gets
  gamed (S7). Forgetting to log is the most common failure of a manual tracker,
  and this covers it.
- A session outside the goal's scope is logged and shown, but marked "doesn't
  count towards your goal". The screen says so *before* confirming.
- **Undo:** the most recent log, while its week is still current.
- **Personal-best week:** celebrated once, the first time this week's count
  goes past `bestWeekCount`, separately from the target.
- **Badges** (S5): cumulative weeks achieved at 4, 12, 26 and 52; lifetime
  sessions at 10, 50, 100 and 250. Recommendation: cumulative weeks rather than
  streak length, so a bout of illness costs the streak but not the badge
  progress. `longestStreak` is shown separately.

### 4.6 Host tests (Gate 1)

Period math for all seven start days across a year end and 29 February. The
clock guard. Time going backwards. A catch-up of 0, 1 and 5 missed weeks with
0, 1 and 2 grace weeks. The trial week. A goal change mid-week. A week-start
change. Undo across the boundary refused. Grace earning and its cap. Every
badge threshold. History ring wrap-around at 52. Persistence round-trip,
including a truncated file and a corrupt one.

---

## 5. Screens and buttons (Tier A)

Buttons match HybridX Race (L1 up, L2 down, R1 select, R2 back/exit), so an
athlete with both apps never has to relearn.

| Screen | Shows | Buttons |
|---|---|---|
| **Home** | Arc ring "2 / 3", "this week", streak "7 weeks", grace shields, bead row of the last 8 weeks | R1 log · L1/L2 menu · R2 exit |
| **Boundary** (only after a roll-over) | "Last week 3/3 ✓ — 8-week streak", or the grace offer from §4.4 | R1 use grace / continue · R2 let it go |
| **Log: type** | Wheel: Run, Strength, Row/Erg, Walk, Other; the goal's scope type first | L1/L2 · R1 next · R2 back |
| **Log: when** | Today / Yesterday (only if still this week) | R1 confirm |
| **Logged** | Tick, haptic, "2 of 3"; or "Week done!"; or "Best week yet"; or "Badge: 12 weeks" | auto-dismiss after 2 s, like Race's Saved screen |
| **Menu** | Trophy case · History · Undo last · Settings | |
| **Trophy case / History** | Paged lists, reusing Race's summary pager | L1/L2 page · R2 back |
| **Settings** | Target, week start, counts (any / one type), reset streak (hold to confirm) | |

First run is the Settings screen with defaults already in place, so a single
R1 press starts. Settings are also phone-editable through AppConfig, the same
pattern Race uses (`targetCount`, `weekStart`, `scope`). A phone edit is still
a goal change, so §4.2's "from next week" rule applies to it too.

**The LVGL pool, from Race.** Race peaked at 91% of the SDK's fixed 40 KB pool
mid-switch (Race NOTES 5.6). The bead row is 8 small objects, and the ring is
one arc. The peak is measured in the simulator at Gate 3, not assumed.

---

## 6. Tier B — auto-detecting sessions, re-planned

Three routes, in the order to take them:

**B1 — HybridX ledger. Recommended first P1 item; nothing unverified.**
HybridX Race appends one record per saved race or sim to
`../SharedData/HybridX/sessions.log`: local day, type, duration, and a unique
id. It uses the same shared-folder mechanism as the SDK's own stride data
(NOTES E.3). The streak reads records newer than its last-read id and logs
them as sessions of the right type. The format lives in
`shared/include/HybridX/SessionLedger.hpp`, compiled into both apps and tested
once. Any future HybridX app joins by appending. Cost: a small Race change,
**after** Race v0.1.0, so its hardware testing is untouched. The file is capped
and rolled over like `StrideLut`'s store.

**B2 — scan other apps' activity folders. Research spike, gated on a
probe.** Apps built on the SDK template write
`Activity/YYYYMM/activity_YYYYMMDDTHHMMSS.fit` with the local start time in the
name (NOTES E.3). A directory walk over `../*/Activity/` could date every
session from every such app without parsing a byte of FIT. Three unknowns,
each answerable on the watch in one sitting:

1. Does the kernel allow `..` into another app's folder, or only into
   `SharedData`?
2. Do UNA's built-in apps (not the SDK examples) use this layout?
3. Type comes only from the folder name (Running → Run); duration would need
   a FIT parser we would have to write. Is folder-name typing good enough?

If all three answers are good, B2 turns on as an opt-in "count sessions from
other apps" setting. If not, B1 still stands.

**B3 — the watch's daily active minutes. Rejected.** The value is only
available live, for today. An app that recomputes on open cannot see
yesterday's (NOTES E.7).

---

## 7. The glance

Built from the GlanceSteps / GlanceActivity pattern: a service only,
`APP_USE_ICONS Off`, and exit on `EVENT_GLANCE_STOP`.

- On `EVENT_GLANCE_START`: `RequestGlanceConfig` for width, height and
  `maxControls`, then read `../SharedData/HybridX/streak.json`, project it to
  now (§3), and draw.
- **Full layout** (if the control budget allows): Jon's X mark · "2/3 this
  week" · "7-week streak" · 8 rectangles as beads, teal for achieved, grey for
  missed, outlined for the current week.
- **Fallback** (small budget): text only, "2/3 · 7 wk". GlanceSteps checks
  `maxControls >= 3` the same way.
- **No summary yet** (streak app never opened): "Open HybridX Streak to set a
  goal".
- Colours are limited to the glance's 16 (`GlanceControl.h:46-61`), so the
  palette is picked from those, not from the LVGL app's.

---

## 8. Reminders — P1, opt-in, and what they cost

The only way to buzz "one session to go" on a Sunday evening is Alarm's
pattern: `APP_AUTOSTART On`, a service resident from boot that waits with a
bounded timeout, and `REQUEST_APP_RUN_GUI` when it is time (NOTES E.2). The
price is a permanent thread and its RAM, a wake-up every period, and residency
the kernel does not promise to honour. If it ships, it is off by default, wakes
at most hourly, and keeps no sensors. It stays out of the MVP until the MVP has
shown the lazy design is enough.

---

## 9. Build phases and gates

The same gated shape as Race: plan mode at the start of each phase, host tests
and **both** builds (watch and simulator) after every change, and a stop at
each gate.

| Phase | Work | Gate — how it is verified |
|---|---|---|
| **S0** Scaffold and probes | `hybridx-streak/` skeleton from Race's LVGL scaffolding; both CMake projects; `streak-v*` versioning; `CLAUDE.md` section. A throwaway **probe app** that writes to a log file: whether `..` reaches another app's folder, the built-in apps' folder layout, and the real glance `width` / `height` / `maxControls` | Both packages build and install; **Jon runs the probe on his watch** and sends back the log |
| **S1** `StreakModel` | §4, pure C++, every rule in §4.6 | Host tests green |
| **S2** Service and persistence | State file written atomically (below), public summary export, AppConfig fields | Round-trip and corruption tests; simulator open → log → close → reopen |
| **S3** GUI | §5 screens; simulator screenshots of every screen; LVGL pool peak measured | Jon reviews screenshots and the flow |
| **S4** Glance | §7, both layouts, the stale-summary projection | Simulator (if it runs glances; otherwise hardware) |
| **S5** Packaging and docs | Two manifests, two store zips, README, ARCHITECTURE, CHANGELOG | Both manifests validate; both zips match `deploy.md` |
| **S6** Hardware and release | On-watch checklist: boundaries across a real Monday, a time-zone change, a flat battery, glance staleness | Jon's hardware pass; tag `streak-v0.1.0` |
| **P1** | B1 ledger (a Race change after Race v0.1.0); timed sessions with a minimum duration; B2 if S0's probe allows; reminders if wanted | Per item |

**Atomic writes on FatFs.** FatFs's rename refuses to overwrite an existing
file (`FR_EXIST`; to be confirmed on the watch at S2). So a save is: write
`state.json.tmp` → remove `state.json.bak` → rename `state.json` →
`state.json.bak` → rename the tmp file → `state.json`. The reader tries
`state.json`, then `.bak`. A reboot at any step leaves a readable file, which
matters because a reboot gives no warning (`service-lifecycle.md` §7). The
public summary for the glance uses the same sequence.

---

## 10. Decisions for Jon, and questions for UNA

Brief decisions, with recommendations:

| ID | Decision | Recommendation |
|---|---|---|
| S1 | Default target and period | 3 per calendar week, Monday start, as the brief says. Week start is our own setting, since the watch has none (NOTES E.5) |
| S2 | MVP activity types | Run, Strength, Row/Erg, Walk, Other, as the brief says. Consider **HYROX / Hybrid** as a sixth, since that is the audience |
| S3 | Grace weeks in MVP | Yes: 1 earned per 4 achieved weeks, cap 2 |
| S4 | Same repo or separate | **Same repo, `hybridx-streak/` folder** (§1) |
| S5 | App name | "HybridX Streak", and "Streak" for the glance's label |

New decisions this plan raises:

| ID | Decision | Recommendation |
|---|---|---|
| S6 | Several weeks missed, not enough grace to cover them all | Streak resets and the screen explains. No partial rescue |
| S7 | Back-filling a forgotten session | Today or yesterday, within the current week only |
| S8 | Goal changes | Take effect next week. A week-start change closes this week early (achieved or void) |
| S9 | First week of a new goal | Trial week: counts if met, cannot break the streak |
| S10 | Badges on weeks | Cumulative weeks achieved, not streak length |
| S11 | Timing sessions | P1, together with the minimum-duration threshold |
| S12 | Licence for Streak | Decides whether §1's same-repo recommendation holds. MIT keeps it simple |

Questions only UNA can answer (to send with the Race findings already queued
for them):

1. Can one store listing install two `.uapp`s, a main app plus its glance?
2. Is `..` access to another app's folder allowed by design, forbidden, or
   undefined? Does their answer differ for `SharedData`?
3. Where do the built-in apps write their activity files?
4. Is a clockface complication API planned? It would beat the glance for a
   streak.

---

## 11. Risks

| Risk | Mitigation |
|---|---|
| Clock lost after a flat battery, before a phone sync | Sanity floor; refuse and explain (§4.1) |
| Travel moves the local date | Day numbers fixed at log time; monotonic periods (§4.1) |
| Reboot mid-write | tmp → bak → rename sequence (§9) |
| The glance shows a stale week | It projects to now with the same model (§3) |
| Two installs confuse users | Ask UNA (§10 Q1). The glance's empty state tells the user to open the app |
| LVGL pool pressure | Few objects; measured at S3 |
| Manual logging is a chore without reminders | "Yesterday" back-fill; auto-logging through B1 as the first P1 |
