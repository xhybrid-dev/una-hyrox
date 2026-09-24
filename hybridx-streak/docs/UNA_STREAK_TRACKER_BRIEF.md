# HybridX Streak Tracker for UNA Watch: Concept and SDK Exploration Brief

**Owner:** Jon
**Audience:** Claude Code, exploring alongside (or after) the Hyrox race app's SDK work
**Status:** Early-stage concept, not yet a gated build plan. The goal of this document is to give Claude Code a model to prototype against and a set of SDK questions to answer while it's already in the documentation, not to start building screens yet.

---

## 1. The core idea

Most streak trackers demand a qualifying activity every single calendar day, which suits committed daily runners but is unachievable, and arguably unhealthy, for most athletes training 3 to 5 times a week.

Jon's idea: the athlete sets their own target, for example 3 sessions a week, rather than every day. Hitting the target in a given period (default: a calendar week) counts as that period achieved. The streak is then a count of **consecutive periods achieved**, not consecutive days. The qualifying activity can be scoped to any activity, or to a specific type (running only, for example).

This fits HybridX's own positioning better than a daily streak would: it works equally well for a beginner doing two sessions a week and a competitive athlete doing six, using the same mechanic with a different personal number, and it doesn't create pressure to train through illness or injury just to protect a daily chain.

---

## 2. Core model to prototype

### 2.1 Goal definition (one per user in MVP)
- `targetCount`: integer, qualifying sessions required in the period. Default 3.
- `period`: default `WEEK`, a calendar week with a configurable start day (default Monday). Document `ROLLING_7_DAY` as a possible v2 option, but recommend the fixed calendar week for MVP: it gives a clean, celebratable "did I hit it" moment at each boundary, where a rolling window is fuzzier to display and to feel good about.
- `activityScope`: `ANY`, or a specific type from a short fixed list (see 2.2).
- Optional per-session qualifying threshold (minimum duration or distance), off by default.
- MVP supports one active goal. Multiple simultaneous goals (a running target and a separate strength target, say) is a natural P1, not MVP.

### 2.2 Logging a qualifying session
Two tiers, deliberately separated by how much they depend on unanswered SDK questions:

- **Tier A, MVP, buildable regardless of what Phase 0 finds:** the athlete logs a session manually on the watch. Pick a type from a short list (Run, Strength, Row/Erg, Walk, Other) via a wheel menu, optionally time it with a simple start/stop, confirm. This needs nothing from any other app.
- **Tier B, stretch, gated on SDK findings (see Section 4):** auto-detect qualifying sessions from activities already recorded by other apps on the watch, including a built-in Running app or the Hyrox app, by reading their saved files. This removes all manual logging, which would be a real usability win, but only works if app storage isn't sandboxed.

### 2.3 Streak state machine
- Track sessions-so-far against the target within the current period.
- At each period boundary: if the target was met, the weeks-streak increments; if not, either a grace week is spent automatically (if available, see 3) or the streak resets to zero. Lifetime totals (sessions logged, periods attempted, longest streak) persist regardless of resets.
- The watch won't reliably be running exactly at a Monday midnight boundary, so evaluate lazily: whenever the app is opened or a session is logged, compute which period "now" falls into from the stored local date and catch up any boundaries crossed since last opened. The Hyrox app's brief handles a related problem (the kernel's wrapping millisecond clock); this is the calendar equivalent, watch for the same class of edge case.

---

## 3. Gamification design

Carried over from the general streak-tracker research, adapted to a weekly rather than daily unit:

- **Visual:** a row of period cells (a chain of beads or flames, one per week, similar in spirit to a GitHub contribution row) rather than a single daily flame, since the unit of success is now the week. Within the current week, a small ring or progress bar shows sessions logged against target, for example 2 of 3.
- **Grace weeks** instead of daily freezes: earn one grace week for every few successful weeks (for example, one per 4 weeks achieved), capped at a small stockpile. Spending one to cover a missed week should be an explicit, visible choice offered at the boundary, not a silent save, so it still feels like a resource rather than the target being quietly softened.
- **Milestone badges** on weeks achieved (4, 12, 26, 52) and on lifetime session counts (10, 50, 100, 250), shown on a trophy-case screen. Reuse the Hyrox app's paged-list pattern for this rather than designing a new one.
- **Personal-best week** callout: if the current week's count beats the best week on record, celebrate that separately from the streak mechanic itself, so a week that misses the target can still feel like a win.

---

## 4. Open SDK questions to fold into exploration

Some of these overlap with the Hyrox app's Phase 0 and can be answered in the same pass; the last three are specific to this app.

1. **(shared)** Does a watch-face complication, widget, or glance surface exist, so the current week's progress or the streak count could be visible without opening the app? This is the single biggest factor in how good this app can feel.
2. **(shared)** Can a background service wake on a schedule (daily, or at the period boundary) to check status and nudge the athlete, or must everything be evaluated lazily on open?
3. **(new)** Is on-watch app storage sandboxed per app, or can one app read files saved by another (its own Running app, a third-party activity app, the Hyrox app)? This alone decides whether Tier B auto-detection is possible.
4. **(new)** What is the actual per-app data file size limit for a settings or history file, separate from the roughly 8 KB-class limit already noted for AppConfig values in the Hyrox brief. This app's own weekly-history file is a different file with potentially different limits.
5. **(new)** How does the kernel expose local calendar date and time zone, particularly relevant if the athlete travels, so period rollover is computed on local date rather than UTC.
6. **(new)** Does the manifest's `type` field support anything other than `activity` (a lighter "utility" type, say) that might suit a mostly-logging app better than the full activity-recording template the Hyrox app uses. Worth checking alongside the complications question.

---

## 5. Suggested approach for Claude Code

This is an exploration pass, not the gated build plan the Hyrox app has. Suggested shape:

- Read the same SDK areas already planned for the Hyrox app's Phase 0 (`Docs/`, RunLVGL, service lifecycle, AppConfig), and specifically hunt for: any complications, widget, or tile concept; any scheduled or background wake API; any indication of shared versus sandboxed app storage; and the full list of valid manifest `type` values.
- Record findings against each of the six questions in Section 4, in this app's own `docs/NOTES.md` (or a clearly separated section of the Hyrox app's, if working from the same SDK clone).
- Sketch, without building, the weekly data model (2.1 to 2.3) and the Tier A logging flow against what's actually confirmed, and flag anywhere this brief's assumptions turn out to be wrong.
- Report back before committing to Tier A-only versus Tier A-plus-B scope for an MVP.

---

## 6. Open decisions for Jon

| ID | Decision | Default until decided |
|---|---|---|
| S1 | Default target count and period | 3 sessions per calendar week, starting Monday |
| S2 | Activity type list for MVP | Run, Strength, Row/Erg, Walk, Other |
| S3 | Grace weeks in MVP, or a fast-follow | In MVP |
| S4 | Same workspace and SDK clone as the Hyrox app, or a separate repo | Same workspace, separate app folder |
| S5 | App name | Working name: HybridX Streak |
