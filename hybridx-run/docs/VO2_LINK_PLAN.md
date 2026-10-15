# VO2max from Run and Trail: plan (L0, 15 October 2026)

Status: approved by Jon on 15 October 2026 (decisions below). **L1 done**
(15 October 2026, see NOTES.md "L1"); L2 next. Phases are L0-L5 (not the
R-phases in NOTES.md). Each phase stops at its gate.

## Decisions (Jon, 15 October 2026)

1. **Cost model: keep ACSM.** Whether Minetti is needed is a later question,
   after real hilly data. Nothing from the paper is in the code.
2. **Trail runs are treated exactly like Run's.** Many Trail runs will be flat
   road runs (Trail's point is the map navigation), so no special hill logic:
   get the functionality in, then build on it. A hilly test run is for later.
3. **HR range limit 12 bpm** (was 10), and grade judged per window (L1).
4. **A copy of the window builder in each app**, with a CI identity check.
5. **Max HR wording on the VO2 face:** yes (L3).

Average grade-adjusted pace (L3) therefore uses ACSM's own grade term: the
equivalent flat speed `(cost - 3.5) / 0.2` from each second's speed and grade,
so it adjusts for uphill only until a downhill model exists.

## Goal (Jon, 15 October 2026)

Trail's runs should count towards the VO2max that Run shows, so the figure is
built from every run on the watch. Trail writes the data Run needs into the
shared data folder; Run picks it up when it starts, recalculates, and adds it
to its history. Not every Trail run is representative, which is why the shown
value averages recent runs. Also: use grade-adjusted pace (GAP) across the
board, in both apps, and add an average-GAP summary to a run.

## What is known

- **Handoff route.** An app can open `../SharedData/...` (write, read, remove
  proven on the watch) and nothing else outside its own folder (Streak Gate 0,
  28 September). So Run cannot read Trail's FIT; Trail must write a file.
  Whether `../SharedData` can be *listed* is not proven: do not depend on it.
- **Neither app has GAP.** Both read the SDK's barometric GRADE sensor
  (percent, usable only when its valid flag is set).
- **Run's grade handling** (`Vo2Run.cpp`): ACSM cost with downhill taken as 0;
  *any single second* outside -3% to +10% rejects the whole 60 s window
  (`mTooSteep`), even if the window's mean grade is about level.
- **Trail has the same inputs** (GPS speed, HR with trust, grade) and the same
  1 Hz track tick, because both start from RunLVGL.

## Evidence: Jon's run of 14 October (`vo2_20261014T131357.csv`)

47 windows, 47 minutes, max HR 182 (age), resting 41, result **45.2**.
Replay: `docs/experiments/vo2_log_whatif.py <csv>`.

| Finding | Detail | Consequence |
|---|---|---|
| It was nearly flat | kept windows: mean grade +0.7%, range -1.2% to +4.5% | This run cannot test GAP; it tests the pipeline only. A hilly run is needed (L5) |
| Wider rules barely move the answer | grade judged on the window mean: 26 windows, 45.2. Plus HR range 12: 34 windows, 45.1 | Accepting more windows gives more data without shifting the figure on flat ground |
| Grade rejections | 8 of 47 windows. Two (6, 10) have a mean grade of -0.5 and -0.1, so a one-second sensor spike rejected them (inferred from the code; the log has window means only). Others are real descents (-5.5, -3.3) | Judge grade on the window, not on one second. Real descents need a downhill cost model |
| HR range | All 9 "unsteady" windows failed on HR range, none on speed (all under the 8% spread limit). Eight missed the 10 bpm limit by 1-3 bpm (11-13); one was 19 | The 10 bpm limit costs the most windows. A tuning decision, not a method change |
| Drift | first half of the kept windows 46.0, second half 44.7 | Matches the known cardiac drift bias in NOTES.md |
| **Max HR dominates** | 175: 43.2, 179 (this run's own max): 44.3, 182: 45.2, 190: 47.6, 195: 49.1 | A measured max HR matters more than any grade work. Say so in the UI wording |
| Resting HR | 41 vs 50: 45.2 vs 45.7 | Small |

## Design (proposed)

**1. One window record, built the same way in both apps.** A small
`WindowBuilder` (a copy in each app, with a CI check that the two copies are
byte-identical, as the config-field check does) turns each 60 s of track into
a record: good seconds, mean speed, spread, mean grade, mean/min/max HR. Only
Run keeps the judging and estimating code. So a Run run and a Trail run take
the same path into the same acceptance rules, which is the consistency Jon asked
for, and Run can re-tune acceptance later without touching Trail.

**2. The shared file.** `../SharedData/HybridX/trail_windows.json`, written only
by Trail (one writer, so no races), crash-safe (`SafeFile`), integers only,
`"v":1`. It holds the last few runs (proposed 3), each with its start UTC
(the run ID) and its post-warm-up windows with 40+ good seconds, capped per run
(proposed 150 windows, about 7 KB per run). One fixed filename, so no folder
listing is needed.

**3. Run's ingest.** When Run opens (and after each of its own runs), it reads
the file with the SDK JSON reader, bounds-checks every value (int64, then range:
the SDK reader does no range check), skips any run ID already in `vo2.json`,
applies its own profile and acceptance rules, and adds the run to the history
ring tagged with its source (Run or Trail). A missing, corrupt or newer-version
file is read as empty and never overwritten. The tag lets Jon compare sources
later; both count equally for now.

**3a. Profile.** Stays in Run (birth date, max HR, resting HR). Trail gets no
new fields.

**4. Grade-adjusted cost (GAP).** Replace "downhill = 0" with a published
cost-versus-grade curve. Method: a grade factor
`f(grade) = cost(grade) / cost(level)`, an equivalent flat speed
`v_eq = v x f`, and the oxygen cost
`3.5 + 0.2 x v_eq`, which keeps ACSM's level calibration. The candidate curve is
Minetti et al. 2002 (J Appl Physiol 93:1039-1046; runs -45% to +45%).
**Its coefficients are not in this repo and I could not confirm them from search
results: they must be read from the paper before any code uses them.** ACSM's
own uphill term and Minetti's curve are different, and may disagree noticeably
at steep grades, so the code keeps both as variants and the host tests compare
them on logs.

**5. Average GAP.** Time-weighted mean of `v_eq` over active seconds with valid
speed and grade, shown as a pace on both summaries (Run's face list, Trail's
summary) and in the tuning log. Not in the FIT file (no confirmed field).
Open: smooth grade over about 10 s before applying `f`, because the barometric
grade is noisy.

**6. Trail shows the figure.** Optional: read Run's existing
`SharedData/HybridX/vo2max.json` and show "VO2max 45.2" on Trail's start screen.

## Phases (each its own plan-mode start and gate; host tests; watch target and simulator built after each change)

| Phase | Work | Gate |
|---|---|---|
| **L0** | This plan, Jon's decisions below | Jon approves |
| **L1** Run core, host only. **Done** | Grade judged per window; `WindowBuilder`, the window record and the file codec; Jon's 14 October log as a host-test fixture. No GAP variants (ACSM kept) | Jon reviews the result (NOTES.md "L1") |
| **L2** Run ingest | Reads `trail_windows.json`, dedupes, tags sources; a synthetic Trail file in the simulator | Simulator shows a Trail run counted once |
| **L3** Run GAP | Average grade-adjusted pace (ACSM-based, above) on the summary; the max-HR wording; GAP columns in the tuning log | Screens captured |
| **L4** Trail | Copy of `WindowBuilder` and the CI identity check; feed it each second; write the file on save; GAP summary; memory and size checked | Simulator run writes a file that Run's L2 reads |
| **L5** On the watch | Both apps from CI; a flat run and a hilly run in each app, with the Garmin on the other wrist; tune acceptance and the cost variant | Jon's field test |

Releases: `run-vX.Y.Z` and `trail-vX.Y.Z`. L2 and L4 ship together, or Run first
(Run alone with no file just reads empty).

## Decisions for Jon

1. **Cost model.** Keep ACSM and compare Minetti on logs (recommended), or go
   straight to Minetti?
2. **A hilly run.** One run on a hill route in Run, with the Garmin, for L5:
   can you do this soon? It is the only way to learn whether GAP helps.
3. **HR range limit.** 10 bpm now. Per-window grade plus a 12 bpm limit together give
   34 windows instead of 22 on this run, with the figure unchanged (45.1).
   Agree to test it in L1?
4. **Copy or share.** A copy of `WindowBuilder` in each app with a CI identity
   check (recommended, matches how the apps are kept independent), or one
   shared library?
5. **Max HR.** It outweighs everything above. Should Run say on the VO2 face
   when the max HR is the age estimate rather than a measured one?

## Risks

- Barometric grade noise: smoothing and the per-window rule are the mitigation;
  the real size of the noise is unknown until a per-second grade trace exists
  (L1 can add one to the tuning log).
- Walking and hiking: below Run's slowest pace (134 m/min) windows are still
  rejected. Trail runs with long hikes may supply few windows. Accepted.
- Memory: both apps are tight in LVGL's pool; L1-L4 add core code only, not
  screens, except one summary face each. Each phase re-measures the pool.
- Concurrency: only Trail writes the file and only Run reads it, so no locking.
  This assumes two activities are not recorded at once, which I have not checked in the SDK; L2 reads the file only at start-up and after a save, so it holds either way.
