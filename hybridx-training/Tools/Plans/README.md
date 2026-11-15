# X Training: the plan files

The three plans live here as plain text, one file each: `10k.plan`, `half.plan` and
`vo2max.plan`. They are the source: the app's `Software/Libs/Core/Header/data/PlanData.hpp`
is generated from them and never edited by hand.

Each file is written for **level 2 and four runs a week** (the tables in `docs/PLANS.md`
sections 5 to 7). The app makes levels 1 and 3 from it, applies the caps per session, and
places the four slots on the runner's own days (`PLANS.md` 3.3, 4.1 and 4.2). So a file only
ever says what a level-2 runner does in each week.

## Changing a plan

1. Edit the `.plan` file.
2. Regenerate the header and the golden files, then read what changed:

   ```
   python3 hybridx-training/Tools/Plans/gen_plans.py
   cmake --build hybridx-training/build-tests
   XTRAINING_UPDATE_GOLDEN=1 hybridx-training/build-tests/hybridx-training-host-tests --gtest_filter='Golden.*'
   git diff hybridx-training/Tests/Host/golden/
   ```

   The golden files are the nine plans for the three gate runners (5K 20:00 at level 3 on
   five days; 25:00 at level 2 on four; 32:00 at level 1 on three), exactly as the watch
   would build them. The diff shows the effect of the edit on each.
3. Run all the host tests before committing (the command is in `CLAUDE.md`). The test
   `plan-data-current` fails if the header and the files disagree.

To see a plan for any other runner:

```
hybridx-training/build-tests/plan_print --plan half --5k 23:30 --half 1:52:00 \
    --level 2 --days mon,wed,fri,sun --long sun --race 2027-03-28 > half.md
```

(`--not-recent` for a 5K that is not from the last six weeks, `--hr` for easy runs by
heart rate, `--distance` for reps by distance.)

## The notation

Lines starting `#` are comments. A file has a header, then one `week` line per week:

```
plan     10k             # a short name, for the generated identifiers
id       1               # 1 10K, 2 half marathon, 3 VO2max: the phone's "plan" field
name     10K             # shown on the watch, up to 20 letters, digits and spaces
race     10000           # the race distance in metres; 0 for a block without a race
minweeks 6               # the fewest weeks the plan may be cut to before a race date
long     1 45 70         # level 1's long runs: from 45 minutes at the start to 70 at the peak
long     2 60 85         # level 2's (the file's own long runs must peak at this)
long     3 70 100
floor    16000 18000 20000   # optional: the peak long run reaches at least this far, by level
trialweek cutback | E+S 35 | E 35 | E 30 | *TT5     # optional: week 1 when the 5K is not recent
recentweek ...                                     # optional: week 1 when it is

week <phase> | <first quality> | <easy> | <second quality> | <long>
```

The phases are `base`, `build`, `peak`, `specific`, `taper`, `race`, `cutback`,
`baseline` and `retest`. Exactly one session a week is marked `*`: the **key** session, the
one kept when the runner has only three days, and the one given priority when sessions are
missed (`PLANS.md` 4.3; that rule is phase TP5's).

| Write | Session | Numbers |
|---|---|---|
| `E 35` | Easy run | minutes |
| `E+S 40` | Easy run, then 6 x 20 s strides | minutes of easy running |
| `L 70` | Long run | minutes |
| `L 100 HM 20` | Long run with the last 20 minutes at half-marathon pace (a hard day) | minutes in all, minutes at half pace |
| `TEMPO 20` | Tempo run at threshold | minutes |
| `CRUISE 3x10` | Cruise intervals at threshold | reps x minutes |
| `INT 4x4` | Intervals at interval pace | reps x minutes |
| `30/30 2x8` | 30-30s | sets x pairs (30 s fast, 30 s easy) |
| `HILLS 8x45s` | Hill reps, jog down ended by a lap press | reps x seconds |
| `REPS 6x400m` | Repetitions at repetition pace, equal jog | reps x metres |
| `RP10 4x5` | Reps at 10K pace | reps x minutes |
| `RPHM 3x10` | Reps at half-marathon pace | reps x minutes |
| `TT5`, `TT10` | 5K or 10K time trial (or a race used as one) | |
| `SHAKE` | 20 min easy and 4 strides, before a race | |
| `RACE` | The race (only in the last week, in the long slot) | |

A rep length can also be `4:30` (minutes and seconds) or `90s`. After `CRUISE`, `INT`,
`RP10` and `RPHM`, an optional `jog 2:00` (or `jog 90s`) sets the recovery; without it the
app uses its rule (`PLANS.md` 3.1): cruise intervals 1 minute per 5 of rep, intervals about
60% of the rep, race pace 2 minutes.

Warm-ups (15 minutes, 10 at level 1) and cool-downs (10 minutes) are added by the app, so a
file never writes them.

## What the generator refuses

`gen_plans.py` stops with the file and line of anything that breaks a rule, so an edit
cannot produce a plan the app would place badly:

- a week without exactly one key session, or a key session that is not quality, a time
  trial or the race;
- anything but `E` or `E+S` in the easy slot; anything but a long run, a time trial or the
  race in the long slot; a long run or the race in a quality slot;
- a long run with a fast finish in a week that also has two quality sessions (`REVIEW.md` C8);
- a race plan that does not end with `RACE`, or `RACE` anywhere else;
- a `jog` on a session that has none, unknown codes, and numbers out of range.
