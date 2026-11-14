# HybridX Training: the pace model and the three plans (version 2)

**Status:** version 2, 14 November 2026, after the independent review (`REVIEW.md`; its
finding numbers, C1-C14, are cited below). Still a draft for Jon to correct: the content is
a coach's to own. Version 1 is in the history (`PLANS_DRAFT.md`, commit `0e1b710`).
Read with `HYBRIDX_TRAINING_BRIEF.md`.

Marks, as in `hybridx-run/docs/RACE_PREDICTIONS.md`:
- **[computed]**: worked out here from a published formula;
- **[secondary]**: from web-search summaries of secondary sources (study notes of Daniels'
  book, calculators); agreed between sources unless said otherwise; not yet read in the book;
- **[convention]**: common coaching practice without strong evidence; Jon's call.

The plans are original: built from the principles below, not copied from a book or an app.

---

## 1. Principles

1. **Train at today's fitness.** Paces come from a recent result, never a goal time, and
   move when a new result says so (2.6).
2. **Dose by time.** Quality is minutes at an intensity, so a 20:00 and a 32:00 5K runner
   get the same session (C1).
3. **Mostly easy, and easy means easy.** About four fifths of the running is easy (Seiler
   and Kjerland 2006; Seiler 2010; section 10). On an easy day the watch only ever says
   "too fast" (C3).
4. **Hard days are spaced.** Never two hard days running. A week holds two quality
   sessions, or one and a long run with a race-pace finish (C8).
5. **Phases:** base (easy running, strides, hills, a little threshold), build (intervals and
   threshold), specific (race pace), taper.
6. **Every fourth week is a cut-back** with about a quarter less running, and ends in a time
   trial or tune-up race, run fresh (C4) [convention].
7. **Limits on quality** per session, by level (3.3), close to Daniels' (C2).
8. **Level 1 is its own plan,** not a smaller level 2: no repetition sessions, short
   intervals (C6).
9. **Taper:** about 60% of the peak in the last week (10K), 75% then 50% in the last two
   (half), keeping some race pace (Bosquet et al. 2007; section 10).
10. **Missed sessions are not made up** by stacking; the key session and the long run have
    priority (4.4).
11. **Not medical advice.** Stop with pain, skip when ill; said once at set-up and in the
    store text.

## 2. The pace model

### 2.1 Fitness from a race

The Daniels–Gilbert equations (Daniels and Gilbert 1979), as in RACE_PREDICTIONS §4.3
(v in m/min, t in min):

    VO2 cost      = −4.60 + 0.182258·v + 0.000104·v²
    fraction held = 0.8 + 0.1894393·e^(−0.012778·t) + 0.2989558·e^(−0.1932605·t)
    fitness       = VO2 cost(D / t) / fraction held(t)

A race time at another distance is the t that gives the same fitness (bisection, done once
per change, never in the 1 Hz loop).

The fitness figure is **internal**. The runner sees it as a 5K equivalent and as predicted
times, never as a number beside X Run's VO2max, which is on another scale (review E5).

### 2.2 Training paces

| Pace | Code | Worked out as | Checked |
|---|---|---|---|
| Easy | E | A range: the speeds costing **62.5%** and **72.5%** of the fitness | Table rows for fitness 51 (4:52-5:29 /km) and 40 (5:59-6:44) [secondary]; the model gives 4:54-5:31 and 5:57-6:41 [computed] |
| Marathon | M | The race equation at 42.195 km. Shown, not used | RACE_PREDICTIONS §4.3 |
| Half marathon | HM | The race equation at 21.0975 km, or the runner's own half (2.4) | Riegel agrees within 15 s, 18:00 to 36:00 [computed] |
| Threshold | T | **88%** of the fitness | Fitness 40, 45 and 51: 5:06, 4:38, 4:11 [secondary]; the model gives the same to the second [computed] |
| 10K | 10K | The race equation at 10 km | Riegel agrees within 15 s [computed] |
| Interval | I | **97.5%** of the fitness | Fitness 51: 3:51 [secondary]; the model 3:51 [computed] |
| Repetition | R | Interval pace less 6 s per 400 m (15 s/km) | Fitness 40, 45, 51: 108, 96, 86 s per 400 m [secondary]; the model 107, 96, 86 [computed] |

Daniels gives E as 59-74% of VO2max, T as 86-88% (some sources 83-88%) and I as 95-100%
[secondary]; the fractions above are the points in those ranges that reproduce his table.
**Still to do:** ten rows of the table read from the book into a fixture file (REVIEW §5).
The host tests then hold the model to them, within ±3 s/km.

### 2.3 Examples [computed]

Per kilometre, from the 5K alone:

| 5K | Easy | Marathon | Half | Threshold | 10K | Interval | Repetition | Predicted 10K | Predicted half |
|---|---|---|---|---|---|---|---|---|---|
| 18:00 | 4:31-5:06 | 4:05 | 3:55 | 3:52 | 3:44 | 3:33 | 3:18 | 37:20 | 1:22:33 |
| 20:00 | 4:59-5:37 | 4:32 | 4:21 | 4:16 | 4:09 | 3:56 | 3:41 | 41:28 | 1:31:50 |
| 22:00 | 5:27-6:08 | 4:59 | 4:48 | 4:40 | 4:34 | 4:18 | 4:03 | 45:37 | 1:41:07 |
| 25:00 | 6:09-6:55 | 5:38 | 5:27 | 5:16 | 5:11 | 4:51 | 4:36 | 51:53 | 1:55:04 |
| 28:00 | 6:50-7:40 | 6:17 | 6:07 | 5:52 | 5:49 | 5:25 | 5:10 | 58:11 | 2:08:57 |
| 32:00 | 7:45-8:40 | 7:09 | 6:59 | 6:40 | 6:40 | 6:09 | 5:54 | 1:06:35 | 2:27:20 |

- **For slower runners threshold meets 10K pace** (32:00: both 6:40), because their 10K
  lasts over an hour and threshold is roughly one-hour race pace. The Paces screen says so.
- The equations were fitted to trained runners. Over about 30 minutes for 5K they are less
  tested, which is one more reason level 1's quality is short.

### 2.4 The half-marathon time, if given

Example: 5K 22:00 and a half of 1:45:00, which is the fitness of a 22:50 5K [computed].
- **Half race pace** comes from the half itself: 4:59 /km, not the 5K's 4:48.
- **Easy and long-run paces** come from the half's fitness: 5:39-6:21, not 5:27-6:08.
- **Threshold, 10K, interval and repetition** stay with the 5K.
- **The gap** is shown in words on the Paces screen: "Your half matches a 22:50 5K. Your
  endurance is behind your speed: the long runs matter most for you." Proposed threshold:
  a 5K equivalent more than 4% slower than the 5K entered. Whether the plan itself should
  change is still open (section 9).
- A half entered more than 6 months ago is ignored, with a word saying so [convention].

### 2.5 Bands and alerts

The SDK's `SpeedGauge` has a 3 s/km deadband (`TargetGauge.hpp`), so bands are not narrower
than ±5 s/km.

| Step | Band | Alerts | Example (22:00 5K) |
|---|---|---|---|
| Easy, long, warm-up, cool-down, recovery | The E range | **Too fast only** (C3) | 5:27-6:08 |
| HM, 10K, T | ±5 s/km | Both ways | T 4:35-4:45 |
| I | ±6 s/km | Both ways | 4:12-4:24 |
| R, strides, 30-30s | ±8 s/km, shown; too short for live alerts | Judged at the end of the rep (brief 8.2) | 400 m in 1:34-1:40 |
| Hills, time trials | None | None | — |

With "Easy runs by heart rate" on, the first row's target is the watch's own zone 2
(`HeartRateTarget::Zone`), still with "too high" only.

### 2.6 Recalibration

| Source | What happens |
|---|---|
| A time trial or tune-up race in the plan | Faster: new paces from the next run. Slower by more than 2%: asked ("Keep your paces, or update?"), because one bad day should not drag the plan down |
| A faster 5K found inside a plan run (X Run's best-split code; outdoors only) | Offered on the Saved screen, never automatic |
| A new 5K or half on the phone | Used from the next session, dated the day it was first seen |
| The race | Kept as the fitness for whatever comes next |

A jump of more than about 6% in one step (1:20 on a 22:00 5K) is asked about, not applied
[convention].

## 3. The sessions

### 3.1 Recipes

Each becomes one `SDK::Workout::Program`. "min" is minutes at the intensity; the builder
turns reps into steps by time (or, with "Reps by distance" on, by distance rounded to 100 m).

| Code | On the watch | Steps | Purpose line |
|---|---|---|---|
| `E t` | Easy run | t min easy | "Easy: you should be able to talk" |
| `E+S t` | Easy run, strides | t min easy, then 6 x (20 s fast, 60 s easy) | "Easy, with strides for form" |
| `L t` | Long run | t min easy | "Builds your endurance" |
| `L t (x HM)` | Long run, fast finish | t − x min easy, then x min at HM. **A hard day** | "Endurance, finishing at race pace" |
| `TEMPO t` | Tempo run | WU, t min at T, CD | "Raises your threshold" |
| `CRUISE n x t` | Cruise intervals | WU, n x (t min at T), jogs of 1 min per 5 min of rep, CD [secondary: Daniels' mile reps with 1 min] | "Threshold in pieces" |
| `INT n x t` | Intervals | WU, n x (t min at I), jogs of about 60% of the rep unless the table says otherwise, CD | "Builds your VO2max" |
| `30/30 s x n` | 30-30s | WU, s sets of n x (30 s at I, 30 s easy), 3 min easy between sets, CD (Billat 2001; section 10) | "VO2max in short bites" |
| `HILLS n x t` | Hill reps | WU, n x (t s uphill, strong; jog back down, ended by a lap press), CD | "Strength and form" |
| `REPS n x d` | Repetitions | WU, n x (d at R, an equal distance jogged), CD. Levels 2 and 3 only | "Speed and form" |
| `RP10 n x t` | 10K pace | WU, n x (t min at 10K pace), jogs, CD | "Race pace" |
| `RPHM n x t` | Half pace | WU, n x (t min at HM pace), jogs, CD | "Race pace" |
| `TT5`, `TT10` | 5K (10K) time trial | Warm-up ended by a lap press (so it works at an event), the distance flat out with time and distance shown, an open cool-down | "Sets your paces" |
| `SHAKE` | Shake-out | 20 min easy, 4 strides | "Loosen up for race day" |
| `RACE d` | Race | Warm-up ended by a lap press, then d with the race-pace band (alerts as in Settings) | "Good luck" |

### 3.2 Warm-ups, cool-downs, recoveries

- Warm-up 15 min (level 1: 10 min), cool-down 10 min, easy, "too fast" alerts only.
  Level 3's warm-up for I, R and time trials ends with 4 strides.
- Recoveries are easy jogs by time, `Rest` steps.
- A time trial's result is the lap of its distance step: the SDK's engine ends a distance
  step exactly at the distance, interpolating between two seconds (`WorkoutEngine.hpp`).

### 3.3 Limits per session

Daniels' limits [secondary]: I the lesser of 10 km or 8% of the week; R the lesser of 8 km or
5%; T 10% (to 30 minutes if broken up); the long run 30% of the week under 40 miles a week,
else 25% or 150 minutes. Turned into minutes at each level's typical week (C2):

| | Level 1 | Level 2 | Level 3 |
|---|---|---|---|
| Interval work, total | 12 min, reps to 3 min | 18 min, reps to 4 min | 24 min, reps to 5 min |
| Tempo, continuous | 15 min | 20 min | 25 min |
| Cruise intervals, total | 20 min | 30 min | 40 min |
| Repetitions, total | none | 2.4 km | 3.2 km |
| 10K pace, total | 16 min | 27 min | 32 min |
| Half pace, total | 25 min | 40 min | 50 min |
| Fast finish of a long run | 10-15 min | 20-30 min | 30-40 min |
| Long run | 150 min at most, at every level |

`PlanBuilder` enforces the caps whatever the plan file says. At level 2 a 22:00 runner's
18 min of intervals is about 9% of a 40-47 km week, and the long run about a third of it at
4 runs a week [computed]: just over Daniels' figures, which assume more runs.

## 4. Levels and the week

### 4.1 Levels

| | Level 1 | Level 2 | Level 3 |
|---|---|---|---|
| Running now | up to 20 km (12 mi) a week | about 20-40 km (12-25 mi) | over 40 km (25 mi) |
| Easy run | 25-35 min | 35-45 min | 45-60 min |
| Long run, 10K (start → peak) | 45 → 70 min | 60 → 85 min | 70 → 100 min |
| Long run, half | 60 → 105 min | 70 → 115 min | 80 → 130 min |
| Half: the peak long run is at least | 16 km | 18 km | 20 km |
| Long run, VO2max block | 45 → 65 min | 60 → 80 min | 70 → 95 min |
| Quality | Two thirds of the tables' minutes, within 3.3; `REPS` becomes `HILLS`; `INT` reps of 2-3 min | As in the tables | One more rep or a quarter more minutes, within 3.3 |

**The half's distance floor (C9).** If the peak long run at the runner's middle easy pace
falls short of the floor, every long run in the plan is scaled up by the same factor, to
150 min at most. A 32:00 runner at level 1 (16 km is 131 min [computed]) gets a peak of
131 min, not 105.

The tables in sections 5-7 are **level 2, four runs a week**.

### 4.2 Placing the week (C12)

The runner chooses the days they can run (seven phone switches, 3 to 6 of them on) and the
long-run day. The tables have four slots: first quality (the Tuesday column), easy, second
quality (Friday) and long (Sunday). The rule that places them:

1. The long run on its day (L).
2. Hard days (the long run and quality) are never next to each other. Quality is never the
   day after the long run; the day before it only when nothing else is possible (a Saturday
   session before a Sunday long run is common enough to allow).
3. With 4 or more run days: the two quality days are the pair of run days that keeps hard
   days furthest apart (the largest smallest gap round the week between L and the two; a tie
   goes to the pair whose first day comes soonest after L). The earlier is the first-quality
   slot. A pair never uses the day before L. If no pair fits, there is one quality day
   (rule 4) and the second quality becomes an easy run with strides.
4. With 3 run days: one quality day, the allowed run day furthest from L. It gets the week's
   key session (★), whichever column that is in; the other run is easy with strides.
5. The remaining days are easy. Extra days (5 or 6 runs) are easy runs of the level's
   shorter length.

Checked by brute force in a scratch script for every choice of 3 to 6 days and long-run day
(392 cases): every one gets at least one quality day, none has quality the day after the
long run, and Tuesday, Wednesday, Friday and Sunday (the default) give the tables' layout.
The host tests repeat the check in TP1.

### 4.3 Missed sessions and weeks (C13, C14)

| Case | What the plan does |
|---|---|
| A missed easy run | Dropped |
| A missed key session, with another quality still to come this week | The key session takes that place |
| A missed long run | Offered once the next day, if that day is easy or rest |
| "Do tomorrow's session today" / "Move today to tomorrow" | Allowed when it keeps hard days apart; otherwise the nearer quality becomes easy |
| A week less than half done | Repeated. With a race date it takes the place of the next week, and never touches the last two weeks (10K) or three (half); without one the plan grows a week, twice at most |
| A week with nothing done | The next week is the last cut-back week again |
| Paused 8-14 days | The last completed week is repeated |
| Paused over 14 days | Easy running until a time trial, then the plan resumes |
| Race too close for the whole plan | Base weeks are dropped first (10K to 6 weeks, half to 8); closer than that, only the final weeks are offered |
| The 5K is not recent (C5) | Week 1 becomes a time-trial week: `E+S`, `E`, `E`, `TT5` |

## 5. The 10K plan (10 weeks)

Level 2, four runs (Tuesday, Wednesday, Friday, Sunday). ★ is the key session.

| Week | Phase | Tue | Wed | Fri | Sun |
|---|---|---|---|---|---|
| 1 | Base | `E+S 40` | `E 35` | `CRUISE 3 x 6` ★ | `L 60` |
| 2 | Base | `HILLS 8 x 45 s` | `E+S 40` | `CRUISE 4 x 6` ★ | `L 65` |
| 3 | Base | `REPS 6 x 400 m` | `E+S 40` | `TEMPO 20` ★ | `L 70` |
| 4 | Cut-back | `E+S 35` | `E 35` | `E 30` | `TT5` ★ |
| 5 | Build | `INT 5 x 3` ★ | `E+S 40` | `CRUISE 3 x 8` | `L 75` |
| 6 | Build | `INT 4 x 4` ★ | `E+S 45` | `CRUISE 3 x 10` | `L 80` |
| 7 | Build | `INT 6 x 3` | `E+S 45` | `RP10 4 x 5`, 90 s jogs ★ | `L 85` |
| 8 | Cut-back | `E+S 35` | `E 35` | `E 30` | `TT5` ★ (tune-up, two weeks out) |
| 9 | Peak | `INT 4 x 4` | `E+S 40` | `RP10 3 x 9`, 2 min jogs ★ | `L 70` |
| 10 | Race week | `RP10 4 x 4`, 2 min jogs ★ | `E+S 30` | `SHAKE` | `RACE 10 km` |

Week 6 for a 22:00 runner is about 40 km [computed].

## 6. The half-marathon plan (12 weeks)

| Week | Phase | Tue | Wed | Fri | Sun |
|---|---|---|---|---|---|
| 1 | Base | `E+S 40` | `E 35` | `CRUISE 3 x 6` ★ | `L 70` |
| 2 | Base | `HILLS 8 x 45 s` | `E+S 40` | `CRUISE 4 x 6` ★ | `L 75` |
| 3 | Base | `INT 5 x 3` | `E+S 40` | `TEMPO 20` ★ | `L 85` |
| 4 | Cut-back | `E+S 35` | `E 35` | `E 30` | `TT5` ★ |
| 5 | Build | `INT 4 x 4` | `E+S 45` | `CRUISE 3 x 10` ★ | `L 95` |
| 6 | Build | `CRUISE 2 x 15` ★ | `E 45` | `E+S 40` | `L 100 (20 HM)` |
| 7 | Build | `INT 6 x 3` | `E+S 45` | `RPHM 3 x 10`, 2 min jogs ★ | `L 105` |
| 8 | Cut-back | `E+S 35` | `E 40` | `E 30` | `TT10` ★ (tune-up, four weeks out) |
| 9 | Specific | `CRUISE 3 x 10` | `E+S 45` | `RPHM 2 x 20`, 3 min jog ★ | `L 115` |
| 10 | Specific | `TEMPO 20` ★ | `E 45` | `E+S 40` | `L 100 (30 HM)` |
| 11 | Taper | `CRUISE 3 x 8` ★ | `E+S 40` | `RPHM 2 x 10`, 2 min jog | `L 75` |
| 12 | Race week | `RPHM 3 x 8`, 2 min jogs ★ | `E+S 30` | `SHAKE` | `RACE 21.1 km` |

- Weeks 6 and 10 have a hard long run, so only one other quality session (C8).
- The longest run is three weeks out, the last race-pace long run two weeks out.
- The 10K tune-up updates the half's pace: a 10K says more about a half than a 5K does.

## 7. The VO2max block (8 weeks)

For a runner already running regularly, between races or before a 10K plan. One long-interval
session a week (the key), one lighter one (C11). It opens and closes with a 5K time trial,
its **main measure**. X Run's VO2max, where the runner has X Run, is the second.

| Week | Phase | Tue | Wed | Fri | Sun |
|---|---|---|---|---|---|
| 1 | Baseline | `E+S 40` | `E 35` | `30/30 2 x 6` | `TT5` ★ |
| 2 | Build | `INT 5 x 3` ★ | `E+S 40` | `30/30 2 x 8` | `L 65` |
| 3 | Build | `INT 4 x 4` ★ (Helgerud et al. 2007) | `E+S 40` | `HILLS 8 x 60 s` | `L 70` |
| 4 | Cut-back | `INT 4 x 3` ★ | `E 35` | `E+S 35` | `L 55` |
| 5 | Build | `INT 6 x 3` ★ | `E+S 45` | `30/30 2 x 10` | `L 75` |
| 6 | Build | `INT 4 x 4`, 2 min jogs ★ | `E+S 45` | `HILLS 10 x 60 s` | `L 75` |
| 7 | Peak | `INT 6 x 3`, 90 s jogs ★ | `E+S 45` | `30/30 3 x 8` | `L 80` |
| 8 | Retest | `INT 3 x 3` | `E 30` | `E 25` | `TT5` ★ |

- With a recent 5K, week 1 is `INT 5 x 2` ★, `E+S 40`, `30/30 2 x 6`, `L 60` instead.
- The progression is in density (shorter jogs), not only volume, within level 2's 18 minutes.
- At the end the Plan screen shows, for example (made-up results): "5K 22:00 → 21:20.
  VO2max (X Run) 45.1 → 46.0".
- Level 1: `INT` of 4-5 x 2 min rising to 4 x 3 min, `HILLS` in place of the 30-30s in
  alternate weeks, and a note at set-up that the block suits runners with a few months of
  regular running behind them.

## 8. One week in full

A 22:00 5K runner, level 2, running Tuesday, Wednesday, Friday and Sunday, in week 7 of the
half plan [computed]:

| Day | Today screen | Steps | About |
|---|---|---|---|
| Mon | REST DAY. Next: Tue, Intervals | — | — |
| Tue | INTERVALS, 6 x 3 min @ 4:12-4:24 /km, about 700 m each. "Builds your VO2max" | Warm up 15 min; 6 x 3 min @ 4:12-4:24 with 2 min jogs; cool down 10 min | 10.0 km, 53 min |
| Wed | EASY RUN, STRIDES, 45 min, easy: slower than 5:27 /km. "Easy, with strides for form" | 45 min easy; 6 x 20 s strides | 7.8 km, 45 min |
| Thu | REST DAY. Next: Fri, Half pace | — | — |
| Fri | HALF PACE ★, 3 x 10 min @ 4:43-4:53 /km, about 2.1 km each. "Race pace" | Warm up 15 min; 3 x 10 min @ 4:43-4:53 with 2 min jogs; cool down 10 min | 11.2 km, 59 min |
| Sat | REST DAY. Next: Sun, Long run | — | — |
| Sun | LONG RUN, 105 min, easy: slower than 5:27 /km. "Builds your endurance" | 105 min easy | 18.1 km, 105 min |

The week is about 47 km: 18 minutes at interval pace (8.9% of the distance) and 30 at half
pace, the rest easy.

## 9. Open questions on the content (for Jon)

None blocks TP1: every number above lives in a plan text file and is cheap to change.

1. The shape of each plan: lengths (10, 12, 8 weeks) and the session mix.
2. The level caps (3.3) and the level table (4.1).
3. "Reps by distance" as an option, time as the default?
4. The half's gap (2.4): words only, or should the plan change (longer long runs)?
5. Race day: run it in X Training with a race-pace band, as drafted?
6. After a plan: suggest a recovery week and then the VO2max block, or leave the choice?
7. Anything the HybridX coaching platform already does that these should match: session
   names, pace names, how levels are described.

## 10. Sources

- Daniels J, Gilbert J. *Oxygen Power: Performance Tables for Distance Runners*. 1979.
- Daniels J. *Daniels' Running Formula* (3rd ed. 2014; 4th ed. 2021), by way of secondary
  sources: the training-pace table rows in 2.2, the intensity ranges, and the limits in 3.3.
  **To be read in the book** (REVIEW §5).
- Riegel PS. Athletic records and human endurance. *American Scientist* 1981;69(3):285-290.
- Seiler KS, Kjerland GØ. *Scand J Med Sci Sports* 2006;16(1):49-56. Seiler S. *Int J Sports
  Physiol Perform* 2010;5(3):276-291.
- Helgerud J et al. Aerobic high-intensity intervals improve VO2max more than moderate
  training. *Med Sci Sports Exerc* 2007;39(4):665-671.
- Billat LV. Interval training for performance, part 1. *Sports Med* 2001;31(1):13-31.
- Bosquet L et al. Effects of tapering on performance: a meta-analysis. *Med Sci Sports
  Exerc* 2007;39(8):1358-1365.

The last four are cited from memory of the literature and have not been re-read here; none
supplies a number the code uses.
