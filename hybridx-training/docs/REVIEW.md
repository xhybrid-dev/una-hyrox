# HybridX Training: independent review of the brief and the plans

**Date:** 14 November 2026. **Reviewed:** the first brief and `PLANS_DRAFT.md` (commit
`0e1b710`, where the app was "HybridX Plan"). **By:** a second pass with a different brief:
take nothing in the first documents on trust, check each claim against the SDK or a source,
and read the plans as a running coach would.

**Result.** The architecture stands. Three engineering points and eleven coaching points
changed the documents; the biggest are in bold below. Everything accepted is already in
`HYBRIDX_TRAINING_BRIEF.md` and `PLANS.md` (version 2). Jon's decisions of 14 November are
recorded in `NOTES.md`.

---

## 1. What was checked, and how

| Check | Result |
|---|---|
| Training-pace fractions against Daniels' published table | **Partly wrong in the draft, now fixed.** The journal and book sites were blocked again, so this used web-search summaries of secondary sources (study notes of the 2021 edition and several calculators). Three table rows were found: fitness 51 (easy 4:52-5:29 /km, threshold 4:11, interval 3:51, repetition 86 s per 400 m), 40 (threshold 5:06, repetition 108 s, easy 5:59-6:44) and 45 (threshold 4:38, repetition 96 s). Threshold at 88% and interval at 97.5% reproduce all of them to the second. Easy was not 65-74% but **62.5-72.5%**, and repetition is better as **interval pace less 6 s per 400 m** than as mile pace. With those, every value found is within 3 s/km [computed]. The draft's "fitness 50" memory was fitness 51's row |
| Race predictions | Riegel (b = 1.06), which RACE_PREDICTIONS §5 recommends as calibrated on recreational runners up to the half, and the Daniels–Gilbert equations agree within **15 s at 10 km and at the half for every 5K from 18:00 to 36:00** [computed]. Two independent published methods, one answer: the race paces are sound |
| Daniels' quality limits | Confirmed by secondary sources: interval work the lesser of 10 km or 8% of weekly distance; repetition the lesser of 8 km or 5%; threshold 10% (up to 30 minutes if broken up); the long run 30% of the week under 40 miles a week, 25% or 150 minutes above. Still to be read in the book |
| How RunLVGL starts a workout | `Service::startWorkout()` is three calls once `mProgram` is filled (`mEngine.start`, `mLeadIn.stepStarted`, `startWorkoutStep`); the details screen is sent `&mProgram`. A plan session needs only a second entry point that fills `mProgram` from the plan instead of a file |
| What makes a Program valid | From `FitWorkoutReader.hpp`: at least one step; at most 100; a repeat points to an earlier step; blocks nest (never overlap) and at most 4 deep; counts 1-99. Units: ms, cm, mm/s; a heart-rate custom value over 100 is bpm + 100. The builder's tests assert each |
| Phone fields | `SDK::AppConfig` has `has`, `getInt`, `getString`, `setInt`, `setString`, `save`. **Patterns must not use `^` or `$`** (`Docs/app-config-fields.md` §3.2: implicit full match), and the watch-side reader has no pattern check at all, so the watch must validate every string |
| The link's limits | The `app` name is 1-15 characters: `HybridXTraining` is exactly 15. `kSlotCount` is 5 of `kMaxSlots` 8. **`StreakNow` sizes an array by `kSlotCount`**, so a sixth slot changes the link library in every app (finding E3) |
| The build environment | X Run's host tests build and pass here (200 tests), so the TP1 scaffold can copy its CMake as it is |

## 2. Engineering findings

| # | Finding | What changed |
|---|---|---|
| **E1** | **Start from X Run, not from RunLVGL.** The brief copied RunLVGL and ported X Run's additions back. But X Run's service is 3,043 lines against RunLVGL's 2,016, and X Training needs most of the difference: the activity link, training load, the Streak line, the zone and best-split statistics (which find a 5K inside a run), cadence, and the window builder for the Run link. Jon's own words were "the same as run". Removing X Run's VO2max history, re-scoring, Trail ingest, VO2 screens and glance is less work, and less risk, than re-adding everything else | Brief §11: a verbatim copy of `hybridx-run/Software` first, in its own commit, then the removals, then the plan. Later X Run fixes merge three ways, as Run R6a did with RunLVGL |
| **E2** | **Plans as text files, not C++ tables.** The plans will be tuned by a coach for months. Jon should be able to change "4 x 4 min" to "5 x 4 min" without touching C++ | `Tools/Plans/*.plan` (plain text) → `gen_plans.py` → a generated header, never edited by hand (as Trail Quest's sprites). `plan_print`'s output for fixed runners is kept as golden files, so any content change shows up as a readable diff in review |
| **E3** | **A new Streak slot is a family change.** The copies of the link library are byte-identical and `StreakNow` (the Streak line in every app) is sized by `kSlotCount`. Adding `act_training.json` means: a Streak release (or it does not count the runs), and a rebuild of the others (or their Streak line misses X Training's sessions until they are next released) | Brief §10.1 and phase TP4 say so; nothing in TP1-TP3 depends on it |
| E4 | Patterns and validation (section 1 above) | Brief §5.1: patterns without anchors; the watch checks every value |
| E5 | The draft showed a "Run fitness" figure (44.5) beside X Run's VO2max (45.1). Two similar numbers on different scales invite exactly the confusion RACE_PREDICTIONS §4.3 warns about | The fitness figure is internal. The runner sees it as a **5K equivalent** and predicted race times |
| E6 | Easy-run alerts (see C3) need no SDK change: the app decides what an alert plays (`playZoneAlert` is app code) | Brief §8.2 |
| E7 | The draft used "×" and "·" in on-watch text. RunLVGL folds workout text to what its fonts can draw (`WorkoutUi.hpp`) | On-watch text is ASCII ("4 x 4 min") until the text-fit audit proves a glyph |
| E8 | Not specified: a plan set up mid-week, the clock not set, a run across midnight | Brief §7: set up Monday to Wednesday starts this week (earlier days are "before the start", not missed), otherwise next Monday; no "today" without a valid date; a session belongs to the day it started |
| E9 | Product, not blocking: X Training does everything X Run does except own the VO2max. Two running apps of ours beside UNA's is a question for the store text, and later for whether X Run stays the VO2max engine only | Brief §14, for later |

## 3. Coaching findings

Read as a coach would read an athlete's plan: is the dose right for every runner it will be
given to, is the hard work placed where the runner is fresh, and what happens when life gets
in the way.

| # | Finding | What changed |
|---|---|---|
| **C1** | **Dose by time, not distance.** The draft's sessions were written in metres, which is fair only to the runner they were written for. For a 32:00 5K runner, `INT 4 x 1200 m` is 7:23 a rep (interval reps should stay under about 5 minutes), `RPHM 3 x 3 km` is 63 minutes at race pace, and `RP10 3 x 2 km` is 40 minutes: far more than the 22:00 runner's 5:09, 43 and 27 [computed] | Every quality session is now minutes at an intensity ("4 x 4 min"), so every runner gets the same stimulus. The Today card shows what a rep is in metres ("about 930 m each"), and a watch setting can run reps by distance, rounded to 100 m, for track users. Recoveries are a share of the rep, not fixed minutes |
| **C2** | **Quality limits by level, in minutes.** The draft found its own sessions broke Daniels' limits at 3-4 runs a week and left it open. The limits are now confirmed (section 1) | Caps per session by level (PLANS §3.3), set close to Daniels' at each level's typical week: for level 2, 18 min of interval work (about 9% of a 40-47 km week [computed]), 20 min of tempo (10%), and 30 min of cruise intervals, which is Daniels' own allowance for threshold work broken into bouts. The long run runs to about a third of the week at 4 runs, which Daniels allows under 40 miles a week (30%), with his 150-minute cap |
| **C3** | **There is no "too slow" on an easy day.** The draft gave easy runs a pace band with alerts both ways. Being buzzed to speed up on a recovery run teaches the wrong thing | Easy and long runs, warm-ups, cool-downs and recoveries alert only for **too fast**. The band still draws the gauge |
| **C4** | **Time trials when fresh, and as tune-up races.** The draft put the 5K time trial on a Tuesday, two days after a long run, and dropped the second one | The time trial takes the long-run slot of each cut-back week (the weekend, when 5K events are; "do tomorrow's session today" moves it to a Saturday event). The 10K plan has a second 5K two weeks before the race; the half has a **10K** tune-up four weeks out, which sets the half's pace better than a 5K can |
| **C5** | **The entered 5K may be a personal best from years ago.** It is the commonest mistake with pace calculators, and it makes three weeks of sessions too fast | A phone switch, "5K run in the last 6 weeks". If off, week 1 becomes a time-trial week before any paced quality |
| **C6** | **Level 1 is not a smaller level 2.** Repetitions at mile pace and long intervals are where new runners get hurt | Level 1 has no repetition sessions (strides and short hills instead), interval reps of at most 3 minutes, and smaller caps |
| C7 | Strides appeared in a few weeks only | One easy run a week carries strides, every normal week |
| **C8** | **A long run with a race-pace finish is a hard day.** The draft's half-marathon weeks 7 and 9 had two quality sessions and a fast-finish long run: three hard days in a four-run week | A week has two quality sessions, or one and a hard long run. The half's weeks are re-cut to match (PLANS §6) |
| **C9** | **Long runs by time short-change slower runners in the half.** The draft's peak, 115 minutes, is 19.8 km for a 22:00 runner but 14.0 km for a 32:00 runner, whose race will last 2:27 [computed] | The half's long runs are scaled so the peak reaches a distance floor (16, 18 or 20 km by level), capped at 150 minutes |
| C10 | The 10K's week 9 held its two biggest sessions and an 80-minute long run, a week before race week | Lighter: the interval session back to an earlier dose, the long run to 70 minutes, the race-pace session kept as the key, 9 days out |
| **C11** | **The VO2max block was two hard interval days a week for seven weeks**, including a week with 4 x 4 min and 6 x 800 m. That is half of a four-run week at VO2max intensity | One long-interval session a week (the key) and one lighter one (30-30s, hills). It also needs a base: level 1 gets a gentler version, and set-up says the block is for runners already running regularly |
| C12 | Sessions sat on fixed days relative to the long run; real runners have fixed days they can run | Seven phone switches, one per day, and a placement rule checked for every combination of days (PLANS §4.2) |
| C13 | A missed key session was simply dropped, even when a lesser quality session was still to come that week | Priority: the long run, then the key session. If the key was missed and a non-key quality is next, the key takes its place |
| C14 | "Repeat the week" was not defined against a fixed race date | With a race date, a repeated week takes the place of the next one (the plan never moves the race or the taper); without one, the plan grows by a week, twice at most |

## 4. Looked at and left alone

- **The equations and the "train at current fitness" rule.** Sound, and now cross-checked.
- **Cut-back every fourth week.** A convention, but a safe one. Every third week for older
  or newer runners is a possible later setting.
- **No heart-rate targets for quality.** Right: heart rate lags reps of a few minutes.
- **No goal time.** Right for v1. The Paces screen shows what current fitness predicts.
- **Outdoors only.** Time-dosed sessions make a treadmill version easy later (show the belt
  speed in km/h), but it is not v1.

## 5. Not done, and why

- **Daniels' book itself was not read.** `test.knowt.com` and `cdbaca.github.io` are outside
  this session's allowed network, as the journal sites were for RACE_PREDICTIONS. The values
  above are from search summaries of secondary sources, which agreed with each other on
  threshold and interval and less well on easy. Before the paces ship, someone with the book
  (Jon?) should read ten rows of the table into the fixture file; the tests then hold the
  model to them.
- **No sports-medicine review.** The plans are conservative by construction, but they are
  not medical advice, and the app and store text must say so.
- **Ideas kept for later:** strength days on rest days through X Workout (HybridX's athletes
  are hybrid athletes), a recovery week after a race, heat and hill adjustments.
