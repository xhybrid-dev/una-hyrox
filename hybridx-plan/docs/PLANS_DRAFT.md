# HybridX Plan: the pace model and the three draft plans

**Status:** draft for Jon, 14 November 2026. Nothing here is decided. The coaching content
(sections 1, 3-7) is Jon's to correct; the pace model (section 2) uses published equations,
but the training fractions and the quality limits are **provisional until read in the source
at TP0**. Read with `HYBRIDX_PLAN_BRIEF.md`.

Marks used, as in `hybridx-run/docs/RACE_PREDICTIONS.md`:
- **[computed]**: worked out here from a published formula;
- **[from memory]**: what I recall of a published source, to be checked before any code;
- **[convention]**: common coaching practice without strong evidence; Jon's call.

All plans here are original: built from the principles below, not copied from a book or
another app (brief, section 16).

---

## 1. Principles (draft)

1. **Train at today's fitness.** Paces come from a recent result, never from a goal time.
   They move when a new result says so (2.7).
2. **Mostly easy.** About four fifths of running time easy, the rest at threshold or faster:
   what well-trained endurance athletes are observed to do (Seiler and Kjerland 2006; Seiler
   2010) [from memory]. At 3-4 runs a week the quality share is a little higher, because two
   sessions of 4 are quality.
3. **One thing at a time.** Each plan moves through phases: **base** (easy running, strides,
   short repetitions, a little threshold), **build** (intervals and threshold), **specific**
   (race-pace work; the half and the VO2max block), **taper**.
4. **Cut-back weeks.** Every fourth week has about a quarter less running [convention]. Week 4
   holds a 5K time trial, so the paces follow the runner.
5. **Gradual growth.** The long run grows by about 5 minutes a week, weekly time by roughly a
   tenth at most [convention: the "10% rule" did not reduce injuries in novice runners in a
   trial, Buist et al. 2008 [from memory], so it is a guide, not a safety rule].
6. **Quality has limits** per session, as a share of the week (3.3).
7. **Taper** before a race: about half the volume over the last one (10K) or two (half) weeks,
   keeping some intensity (Bosquet et al. 2007, meta-analysis) [from memory].
8. **Never make up a missed session** by stacking it onto another day (4.4).
9. **The plan is not medical advice.** Stop with pain; skip when ill. The app says so once, at
   set-up, and the store text says it too.

## 2. The pace model

### 2.1 Fitness from a race

The Daniels–Gilbert equations (Daniels and Gilbert 1979), as quoted and checked in
RACE_PREDICTIONS §4.3 (v speed in m/min, t race time in min):

    VO2 cost      = −4.60 + 0.182258·v + 0.000104·v²
    fraction held = 0.8 + 0.1894393·e^(−0.012778·t) + 0.2989558·e^(−0.1932605·t)
    fitness       = VO2 cost(D / t) / fraction held(t)

The fitness of a 5K is a direct calculation. A race time at another distance is found by
solving the same equation for t (bisection, about 40 steps, once per change, not in the 1 Hz
loop). Checked again here [computed]: fitness 50 gives 19:56, 41:20, 1:31:31 and 3:10:40, the
same as §4.3 and within 10 s of Daniels' published table.

**The name.** Daniels calls the figure VDOT, which is also a brand (VDOT O2). The app should
call it something of its own: "Run fitness" (decision for Jon).

### 2.2 Training paces from the fitness

| Pace | Code | What it is for | How it is worked out (provisional) |
|---|---|---|---|
| Easy | E | Easy and long runs, warm-ups, recoveries | A range: the speeds whose cost is 65% and 74% of the fitness [from memory: Daniels gives E as about 59-74% of VO2max; the table's E range for fitness 50, about 4:52-5:29 /km, is matched by 74% and 65%] |
| Marathon | M | Not used by these plans, shown on the Paces page | The race equation at 42.195 km |
| Half marathon | HM | The half plan's race-pace work | The race equation at 21.0975 km, or the runner's own half time (2.4) |
| Threshold | T | Tempo runs and cruise intervals | 88% of the fitness [from memory: T is about 83-88% of VO2max, "a pace you could race for about an hour"; the fraction held for 60 min is 0.888 by the equation above [computed], which agrees] |
| 10K | 10K | The 10K plan's race-pace work | The race equation at 10 km |
| Interval | I | VO2max intervals | 97.5% of the fitness [from memory: I is 95-100% of VO2max, a pace you could race for 10-12 minutes; 97.5% reproduces the 3:55 /km I recall for fitness 50] |
| Repetition | R | Short fast reps, strides | The race equation's mile pace [from memory: Daniels' R is about current mile race pace] |

**Checked against the published table for fitness 50, as far as memory goes** [computed]:
T 4:15 /km, I 3:55 /km (97.5%; 3:50 at 100%), E 4:54-5:26 /km, R 87 s per 400 m. Daniels'
table should be read at TP0 for a fixture set (fitness 30 to 70 in steps of 5), and the
fractions adjusted until every pace is within ±2 s/km. Those fixtures then become host tests.

### 2.3 Examples [computed]

Per kilometre, from the 5K alone, at the provisional fractions:

| 5K | Fitness | Easy | Marathon | Half | Threshold | 10K | Interval | Repetition | I 400 m | R 200 m |
|---|---|---|---|---|---|---|---|---|---|---|
| 18:00 | 56.3 | 4:27-4:56 | 4:05 | 3:55 | 3:52 | 3:44 | 3:33 | 3:15 | 85 s | 39 s |
| 20:00 | 49.8 | 4:54-5:27 | 4:32 | 4:21 | 4:16 | 4:09 | 3:56 | 3:38 | 94 s | 44 s |
| 22:00 | 44.5 | 5:22-5:57 | 4:59 | 4:48 | 4:40 | 4:34 | 4:18 | 4:01 | 103 s | 48 s |
| 25:00 | 38.3 | 6:03-6:42 | 5:38 | 5:27 | 5:16 | 5:11 | 4:51 | 4:36 | 117 s | 55 s |
| 28:00 | 33.5 | 6:44-7:27 | 6:17 | 6:07 | 5:52 | 5:49 | 5:25 | 5:11 | 130 s | 62 s |
| 32:00 | 28.5 | 7:37-8:25 | 7:09 | 6:59 | 6:40 | 6:40 | 6:09 | 5:57 | 148 s | 71 s |

Two things the table shows:
- **For slower runners, threshold reaches 10K pace** (32:00: both 6:40). That follows from T's
  definition: their 10K lasts over an hour. Expected, but worth a word on the Paces page
  ("Threshold is about your 10K pace") rather than looking like a fault.
- The equations were fitted to trained runners (RACE_PREDICTIONS §4.3, "Where Daniels breaks
  down"). For 5K times over about 30 minutes, the paces are less tested. Level 1 keeps the
  quality short for that reason (4.1).

### 2.4 The half-marathon time, if given

Example: 5K 22:00 (fitness 44.5) and a half in 1:45:00 (fitness 42.6) [computed].
- **Half race pace** comes from the half itself: 4:59 /km, not the 5K's 4:48. It is the
  better guide to the race, being the same distance.
- **Easy and long-run paces** come from the half's fitness too: 5:34-6:10 rather than
  5:22-5:57. Slower easy running is the safe side.
- **Threshold, 10K, interval and repetition** stay with the 5K: they are short efforts, nearer
  the 5K.
- **The gap** (here 1.9) is shown on the Paces page. A gap of more than about 3 (provisional)
  says endurance is behind speed, and the half plan says so at set-up: "Your half is slower
  than your 5K suggests: the long runs matter most for you." Whether the plan should also
  change (longer long runs, more threshold) is a question for Jon (section 10).
- A half more than about 6 months old (the date it was entered, 5.1 of the brief) is ignored,
  with a word on the Paces page [convention; Jon's call].

### 2.5 Bands and rounding

The SDK's `SpeedGauge` judges a band with a 3 s/km deadband and a dwell (`TargetGauge.hpp`),
so a band narrower than about ±4 s/km would flicker. Proposed:

| Pace | Band | Example (22:00 5K) |
|---|---|---|
| E | the range itself | 5:22-5:57 |
| HM, 10K, T | ±5 s/km | T 4:35-4:45 |
| I | ±6 s/km | 4:12-4:24 |
| R | ±8 s/km, judged by rep time (brief 8.2) | 400 m in 1:33-1:40 |

Shown paces are rounded to the nearest second; the bands' edges to whole seconds. Miles when
the watch is set to imperial (`stepLabel()` already does both).

### 2.6 Easy by heart rate

With `easyByHr` on, easy and long runs (and warm-ups and recoveries, if they have a target at
all; section 10) get a heart-rate target of the watch's own **zone 2** (`HeartRateTarget::Zone`,
resolved against the watch's zones by the SDK), instead of the E pace band. Quality steps keep
their pace bands. This suits hills and hot days, and runners who drift too fast on easy days.

### 2.7 Recalibration

| Source | What happens (proposed; brief decision 14.8) |
|---|---|
| A 5K time trial in the plan (week 4, and the VO2max block's first and last weeks) | Faster: the new fitness from the next session, with "New paces from your next run". Slower by more than 1.0: asked ("Keep your paces, or update?"); a bad day should not drag the plan down |
| A faster 5K found inside any plan run (the best 5 km split, outdoors only, as X Run's `RunStats` finds it) | Offered on the Saved screen ("5 km in 21:41 inside this run: update your paces?"). Never automatic: it may have been downhill or a GPS fault |
| A new 5K or half time on the phone | Used from the next session, dated the day it was first seen |
| The race at the end of a plan | Saved as the new fitness for whatever comes next |

A rise of more than 3 in one step (about 1:30 on a 22:00 5K) is asked about, not applied
[convention].

### 2.8 Not used

- **X Run's VO2max:** a different scale (brief 10.3; RACE_PREDICTIONS §4.3).
- **A goal time:** decision 14.5.
- **Heart rate for quality sessions:** heart rate lags too much for reps of a few minutes.

## 3. The session library

### 3.1 Recipes

Every session is one of these recipes, filled in with numbers from the plan tables and the
level (4.1). Each becomes an `SDK::Workout::Program`: warm-up (`Intensity::Warmup`), active
steps (`Active`) with their pace band, recoveries (`Rest`), cool-down (`Cooldown`).

| Code | Name on the watch | Steps | Purpose line (draft) |
|---|---|---|---|
| `E t` | Easy run | t min at E | "Easy: you should be able to talk" |
| `E+S t` | Easy run and strides | t min at E, then 6 × (20 s fast, 60 s easy) | "Easy, with strides for form" |
| `L t` | Long run | t min at E | "Builds your endurance" |
| `L t (x HM)` | Long run, fast finish | t − x min at E, then x min at HM | "Endurance, finishing at race pace" |
| `TEMPO t` | Tempo run | WU, t min at T, CD | "Raises your threshold" |
| `CRUISE n × d` | Cruise intervals | WU, n × (d at T, jog), CD; the jog 1 min for every 5 min of rep, in 30 s steps [from memory: Daniels' cruise-interval rest] | "Threshold in pieces" |
| `INT n × d` | Intervals | WU, n × (d at I, jog r), CD | "Builds your VO2max" |
| `INT n × t min` | Intervals by time | WU, n × (t min at I, jog r), CD | "Builds your VO2max" |
| `30/30 s × n` | 30-30s | WU, s sets of n × (30 s at I or a little faster, 30 s easy), 3 min easy between sets, CD (Billat 2001) [from memory] | "Time at VO2max in short bites" |
| `HILLS n × t` | Hill reps | WU, n × (t s uphill hard, jog back down: ends on a lap press), CD. No pace target: pace means nothing uphill | "Strength and VO2max" |
| `REPS n × d` | Repetitions | WU, n × (d at R, jog d), CD | "Speed and form" |
| `RP10 n × d` | 10K pace | WU, n × (d at 10K, jog r), CD | "Race pace" |
| `RPHM n × d` | Half pace | WU, n × (d at HM, jog r), CD | "Race pace" |
| `TT5` | 5K time trial | WU 15 min with 4 strides, 5 km all-out with no target (time and distance shown), CD 10 min | "Sets your paces" |
| `SHAKE` | Shake-out | 20 min at E and 4 strides | "Loosen up for race day" |
| `RACE d` | Race | An open warm-up (ends on a lap press), then d at race pace (10K or HM band), alerts as in Settings | "Good luck" |

### 3.2 Warm-ups, cool-downs and recoveries

- **Warm-up** 15 min (level 1: 10 min), **cool-down** 10 min, both easy. Proposed **without a
  pace target**, shown as pace only, so a slow first mile is not nagged (decision for Jon).
- **Recoveries** are jogs by time (or by distance for repetitions), `Rest`, no target.
- **Strides** and **30/30s** are by time with their target shown but, being under 20 s of
  quiet start, never alerted (brief 8.2).

### 3.3 Quality limits

From Daniels [from memory, check at TP0]:

| Quality | Limit per session |
|---|---|
| T | 10% of the week's distance |
| I | 8% of the week's distance, and at most 10 km |
| R | 5% of the week's distance, and at most 8 km |
| Long run | 25-30% of the week's distance, and at most 150 min |

`PlanBuilder` can enforce them from the plan's own weekly distance (the week's minutes at each
pace), trimming reps. **But they clash with these drafts at 3-4 runs a week**:
- a 22:00 runner in the 10K plan's week 6 (level 2, 4 days) runs about 43 km [computed]; 8% is
  3.4 km of I, so the draft's 5 × 1000 m would become 4 × 800 m;
- at 4 runs a week the long run is about a third of the week, over the 25-30%.

Daniels' limits assume 6-7 runs a week. Options for Jon: enforce them; scale them for 3-5 runs
(for example I up to 12%, the long run up to 35%); or use the table's numbers as written, with
the levels doing the scaling. `plan_print` (TP1) will show each option's effect.

## 4. Levels and the week

### 4.1 Levels

| | Level 1 | Level 2 | Level 3 |
|---|---|---|---|
| Running now | up to 20 km (12 mi) a week | about 20-40 km (12-25 mi) | over 40 km (25 mi) |
| Runs a week | 3-4 (5 allowed) | 3-5 | 4-5 |
| Easy run | 25-35 min | 35-50 min | 45-60 min |
| Long run, 10K plan (start → peak) | 45 → 70 min | 60 → 85 min | 70 → 100 min |
| Long run, half plan | 60 → 105 min | 70 → 115 min | 80 → 130 min |
| Long run, VO2max block | 45 → 65 min | 60 → 80 min | 70 → 95 min |
| Reps | the table's count less 1-2 (at least 3), or the next shorter rep | as in the tables | 1 more, or the next longer rep |
| Tempo and threshold minutes | two thirds | as in the tables | a quarter more |
| Warm-up | 10 min | 15 min | 15 min and 4 strides |

The tables below are written for **level 2, 4 runs a week, long run on Sunday**; the levels and
the placement rules make the rest. `plan_print` (TP1) prints every combination for review.

### 4.2 Placing the sessions

Relative to the long-run day L (Sunday in the tables):

| Runs a week | L+1 | L+2 | L+3 | L+4 | L+5 | L+6 | L |
|---|---|---|---|---|---|---|---|
| 3 | rest | **key** quality | rest | easy + strides | rest | rest | long |
| 4 | rest | Q1 | easy | rest | Q2 | rest | long |
| 5 | rest | Q1 | easy | rest | Q2 | easy | long |

- At 3 runs, the week's **key** session (★ in the tables) is kept and the other quality becomes
  the easy run with strides.
- Never two quality days within 48 hours, whatever is moved (4.4).

### 4.3 Cut-back weeks and the taper

- **Cut-back weeks** (10K weeks 4 and 8; half weeks 4 and 8; VO2max week 4): about a quarter
  less, with the 5K time trial in week 4.
- **Taper:** 10K, the last week at about 60% of the peak; half, the last two at about 75% and
  50%; VO2max, the last week before the retest at about 60%.

### 4.4 Missed sessions and weeks (draft rules)

| Case | What the plan does |
|---|---|
| A missed easy run or quality session | Dropped. Nothing is added elsewhere |
| A missed long run | Offered once the next day, if that day is easy or rest; the two swap |
| The runner moves today to tomorrow | Allowed when tomorrow is rest or easy and it keeps quality 48 h apart; otherwise tomorrow's quality becomes easy |
| A week less than half done | Repeated, taken from the base weeks first; never moves the taper or the race |
| A week with nothing done | The next week is its cut-back version |
| Paused 8-14 days | The last completed week is repeated |
| Paused over 14 days | Easy running only until a 5K time trial, then the plan resumes at the matching week |
| Race date too close | The first base weeks are dropped (10K to 6 weeks, half to 8); closer than that, only the last weeks are offered |

## 5. The 10K plan (10 weeks)

Level 2, 4 runs a week, long run Sunday. ★ the key session (kept at 3 runs a week). At 5
runs, an easy run is added on Saturday (35-45 min).

| Week | Phase | Tue (Q1) | Wed | Fri (Q2) | Sun |
|---|---|---|---|---|---|
| 1 | Base | `E+S 40` | `E 40` | `CRUISE 3 × 5 min` ★ | `L 60` |
| 2 | Base | `REPS 8 × 200 m` | `E 40` | `CRUISE 4 × 5 min` ★ | `L 65` |
| 3 | Base | `REPS 6 × 400 m` | `E 45` | `TEMPO 20` ★ | `L 70` |
| 4 | Cut-back | `TT5` ★ | `E 35` | `E+S 35` | `L 55` |
| 5 | Build | `INT 5 × 800 m`, 2 min jog ★ | `E 45` | `CRUISE 3 × 1.6 km` | `L 75` |
| 6 | Build | `INT 5 × 1000 m`, 2:30 jog ★ | `E 45` | `TEMPO 25` | `L 80` |
| 7 | Build | `INT 4 × 1200 m`, 3 min jog | `E 45` | `RP10 3 × 2 km`, 2 min jog ★ | `L 85` |
| 8 | Cut-back | `INT 4 × 1000 m`, 2:30 jog ★ | `E 40` | `TEMPO 20` | `L 65` |
| 9 | Peak | `INT 6 × 1000 m`, 2:30 jog | `E 45` | `RP10 4 × 2 km`, 2 min jog ★ | `L 80` |
| 10 | Taper and race | `RP10 4 × 1 km`, 2 min jog ★ | — | Thu `E+S 30`, Sat `SHAKE` | `RACE 10 km` |

Week 8's time trial is left out on purpose (it is two weeks before the race); add it back if
Jon prefers a check there.

## 6. The half-marathon plan (12 weeks)

| Week | Phase | Tue (Q1) | Wed | Fri (Q2) | Sun |
|---|---|---|---|---|---|
| 1 | Base | `E+S 40` | `E 40` | `CRUISE 3 × 6 min` ★ | `L 70` |
| 2 | Base | `REPS 8 × 200 m` | `E 45` | `CRUISE 4 × 6 min` ★ | `L 75` |
| 3 | Base | `INT 5 × 800 m`, 2 min jog | `E 45` | `TEMPO 20` ★ | `L 80` |
| 4 | Cut-back | `TT5` ★ | `E 35` | `E+S 35` | `L 65` |
| 5 | Build | `INT 5 × 1000 m`, 2:30 jog | `E 45` | `CRUISE 3 × 1.6 km` ★ | `L 90` |
| 6 | Build | `CRUISE 4 × 1.6 km` ★ | `E 45` | `RPHM 2 × 3 km`, 3 min jog | `L 95` |
| 7 | Build | `INT 4 × 1200 m`, 3 min jog | `E 50` | `TEMPO 30` ★ | `L 100 (15 HM)` |
| 8 | Cut-back | `CRUISE 3 × 1.6 km` ★ | `E 40` | `RPHM 2 × 3 km`, 3 min jog | `L 75` |
| 9 | Specific | `INT 5 × 1000 m`, 2:30 jog | `E 50` | `RPHM 3 × 3 km`, 2 min jog ★ | `L 105 (20 HM)` |
| 10 | Specific | `CRUISE 2 × 15 min`, 3 min jog | `E 50` | `RPHM 8 km` ★ | `L 115` |
| 11 | Taper | `CRUISE 3 × 8 min`, 2 min jog | `E 40` | `RPHM 2 × 3 km`, 3 min jog ★ | `L 80` |
| 12 | Race week | `RPHM 3 × 1.6 km`, 2 min jog ★ | — | Thu `E+S 30`, Sat `SHAKE` | `RACE 21.1 km` |

For a 22:00 runner the peak long run, 115 min at the middle of the easy range, is about 20 km
[computed]; level 3's 130 min about 23 km.

## 7. The VO2max block (8 weeks)

A block to raise VO2max, between races or before a 10K plan. Two VO2max sessions a week, the
rest easy. It opens and closes with a 5K time trial, which is its **main measure**; X Run's
VO2max, if the runner uses X Run (and its Run link, brief 10.2), is the second.

| Week | Phase | Tue (Q1) | Wed | Fri (Q2) | Sun |
|---|---|---|---|---|---|
| 1 | Baseline | `TT5` ★ (or, with a 5K from the last 4 weeks, `INT 5 × 800 m`) | `E 40` | `30/30 2 × 8` | `L 60` |
| 2 | Build | `INT 5 × 1000 m`, 2:30 jog ★ | `E 40` | `30/30 2 × 10` | `L 65` |
| 3 | Build | `INT 4 × 4 min`, 3 min jog ★ (Helgerud et al. 2007) | `E 45` | `INT 6 × 800 m`, 2 min jog | `L 70` |
| 4 | Cut-back | `INT 4 × 1000 m`, 2:30 jog ★ | `E 35` | `E+S 35` | `L 55` |
| 5 | Build | `INT 5 × 1200 m`, 3 min jog ★ | `E 45` | `30/30 3 × 8` | `L 75` |
| 6 | Build | `INT 5 × 4 min`, 3 min jog ★ | `E 45` | `HILLS 8 × 60 s` | `L 75` |
| 7 | Peak | `INT 6 × 1000 m`, 2:30 jog ★ | `E 45` | `30/30 2 × 12` | `L 80` |
| 8 | Retest | `INT 4 × 800 m`, 2 min jog | `E 30` | `E+S 25` | `TT5` ★ |

- Interval reps are kept between about 2 and 5 minutes at I [from memory: Daniels' guidance
  for I reps].
- After week 8 the Plan screen shows the result, for example (made-up results): "5K 22:00 →
  21:20 · Run fitness 44.5 → 46.2 · VO2max (X Run) 45.1 → 46.0".
- Level 1 starts with `INT 4 × 600 m` and `30/30 2 × 6`, and has no hills week.

## 8. One week in full: what the watch shows

A runner with a 22:00 5K, level 2, 4 days, Sunday long runs, in week 6 of the half plan
[computed, provisional paces]:

| Day | Today card | Steps | About |
|---|---|---|---|
| Mon | REST DAY · Next: Tue Cruise intervals | — | — |
| Tue | CRUISE INTERVALS · 4 × 1.6 km · @ 4:35-4:45 /km · "Threshold in pieces" | Warm up 15 min · 4 × 1.6 km @ 4:35-4:45, 90 s jog between · Cool down 10 min | 11.5 km, 59 min |
| Wed | EASY RUN · 45 min · @ 5:22-5:57 /km · "Easy: you should be able to talk" | 45 min @ 5:22-5:57 | 8.0 km, 45 min |
| Thu | REST DAY · Next: Fri Half pace | — | — |
| Fri | HALF PACE · 2 × 3 km · @ 4:43-4:53 /km · "Race pace" | Warm up 15 min · 2 × 3 km @ 4:43-4:53, 3 min jog between · Cool down 10 min | 10.9 km, 57 min |
| Sat | REST DAY · Next: Sun Long run | — | — |
| Sun | LONG RUN · 95 min · @ 5:22-5:57 /km · "Builds your endurance" | 95 min @ 5:22-5:57 | 16.8 km, 95 min |

During Tuesday's second rep the step face reads "RUN 2/4", the gauge's pointer in the green
middle third at 4:39, "840 m to go"; at 5 s from the end the lead-in beeps start; the step
card then shows "JOG · 90 s" and, from the rep just run, "Last rep 7:26 · target 7:20-7:36".
On Sunday the Saved screen reads "Session done · Week 6: 4 of 4 done · Next: Tue Intervals",
then the Streak line.

## 9. Sources

The equations:
- Daniels J, Gilbert J. *Oxygen Power: Performance Tables for Distance Runners*. Tempe, AZ,
  1979 (equations as in RACE_PREDICTIONS §4.3).

[From memory: each to be checked before any figure from it is coded or put in store text]
- Daniels J. *Daniels' Running Formula*, 3rd ed. Human Kinetics, 2014: the E, M, T, I and R
  intensities, cruise-interval rests, I rep lengths, and the quality limits in 3.3.
- Seiler KS, Kjerland GØ. Quantifying training intensity distribution in elite endurance
  athletes: is there evidence for an "optimal" distribution? *Scand J Med Sci Sports*
  2006;16(1):49-56.
- Seiler S. What is best practice for training intensity and duration distribution in
  endurance athletes? *Int J Sports Physiol Perform* 2010;5(3):276-291.
- Helgerud J et al. Aerobic high-intensity intervals improve VO2max more than moderate
  training. *Med Sci Sports Exerc* 2007;39(4):665-671 (the 4 × 4 min).
- Billat LV. Interval training for performance: a scientific and empirical practice. Part 1:
  aerobic interval training. *Sports Med* 2001;31(1):13-31 (the 30-30s).
- Bosquet L, Montpetit J, Arvisais D, Mujika I. Effects of tapering on performance: a
  meta-analysis. *Med Sci Sports Exerc* 2007;39(8):1358-1365.
- Buist I et al. No effect of a graded training program on the number of running-related
  injuries in novice runners: a randomized controlled trial. *Am J Sports Med*
  2008;36(1):33-39.

## 10. Open questions on the content (for Jon)

1. The three plans' shape: lengths (10, 12, 8 weeks), phases, the session mix, the key sessions.
2. Levels: three, as in 4.1, and their numbers.
3. The quality limits and the clash in 3.3.
4. Warm-ups and cool-downs without a pace target (3.2)?
5. The half's gap rule (2.4): a word only, or a change to the plan?
6. Time trials: week 4 only, as drafted, or week 8 too (section 5)?
7. Race day: run the race in X Plan with a race-pace band, or just "Race day" and a free run?
8. What follows a plan: the VO2max block, a "keep fit" week on repeat, or nothing (the
   runner picks)?
9. Anything Jon's coaching platform already does (HybridX) that these plans should match:
   session names, pace names, the way levels are described.
