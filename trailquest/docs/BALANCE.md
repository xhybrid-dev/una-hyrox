# Trail Quest: balance

How hard the duels are, how the levels grow, and how long the Ridge takes, from
two simulators that play the real game core (Q1, 17 October 2026). The numbers
live in the code (`Software/Libs/Core`); this says what they add up to, and what
Jon is asked to agree at Gate Q1.

Regenerate after changing any number in the core (both tools build with the host
tests, under a second each):

```sh
cmake -S trailquest/Tests/Host -B trailquest/build-tests && cmake --build trailquest/build-tests
trailquest/build-tests/balance_sim 10000 --md out.md --csv out.csv
trailquest/build-tests/run_sim 4000 casual --md out.md     # also: good, menu
```

Both are seeded, so the same build gives the same tables.

## Targets (proposed for Gate Q1)

| What | Target | Why |
|---|---|---|
| A regular spirit at the stage's par level, casual player, fresh | **80-95 % won, 3-6 turns** on average | A duel is a 30-60 s moment between stretches of trail (brief section 4), and a loss should be a surprise, not a routine |
| A boss, the runner one level above par, casual player, fresh | **65-85 % won** | A real fight that a prepared runner usually wins |
| Reaching each boss | **within one level of par** on average | The XP curve keeps pace with the stages without grinding |
| Every run | **reaches the summit** | No dead ends, no walls a casual player cannot pass |

`balance_sim` checks the first two (its Check column); `run_sim` checks the last
two. All rows are inside the targets today.

## The rules, in numbers

| Rule | Value | Where |
|---|---|---|
| Runner at level L | stamina 40 + 4(L-1); pace, form, grit 10 + (L-1); levels 1-20 | `Progress.hpp` |
| A spirit at level L | stamina a per cent of the runner's at L; pace, form, grit offsets from the runner's | `data/Species.hpp` |
| XP | to the next level 8 + 4L; a win 6 + 2L (a boss three times that); an obstacle 3 | `Progress.hpp` |
| A partner | +1 to its affinity's stat; the best one's move plays as R1, Technique | `Progress.hpp`, `Run.cpp` |
| The wheel | Pace beats Form beats Grit beats Pace: x3/2, or x3/4 the other way | `Types.hpp` |
| Damage | power x (10 + attacker's stat) / (10 + defender's stat), in the move's affinity; then the wheel; then the beat (Miss x1/2, Good x1, Perfect x3/2); +2 power on favoured terrain, -1 for Form in rain, -1 for Pace in wind; + grit/2 when the defender is under a quarter; then x(9 + L)/10; at least 1 | `Duel.cpp` |
| Hit chance | 90 % + 2 % a point of pace over the defender, held to 60-100 %; -10 in mist; -10 for the runner in the dark | `Duel.cpp` |
| Runner's moves | Surge 14 Pace, costs 4; Settle 5 Grit, recovers 6; Trail craft 9 in the terrain's affinity | `data/Moves.hpp` |
| Effects | Drain (-2 a turn), Slow (-2 pace), Rattle (-2 grit); 3 turns; one at a time | `Types.hpp` |
| Snow | the runner loses 1 stamina a turn (level-scaled) | `Duel.cpp` |
| Bosses | rest once below 30 %, recovering 25 % | `Duel.hpp` |
| Boss twists | a boss tells every 3rd turn (the Old Tortoise every 4th): a **charge** hits x2, or x1/2 if the runner Settles; a **guard** halves the runner's damage that turn; the Summit Wolf alternates | `Duel.cpp`, `data/Species.hpp` |
| The needle | stage 1: sweep 1600 ms, good zone 400, perfect 200; each stage 100 ms faster and the good zone 40 narrower (stage 5: 1200 / 240); each lap a further 100 / 20, to 1000 / 200. Perfect stays two frames (the 10 fps floor) | `Beat.hpp` (`beatFor`) |
| Stage softening | the spirits of stage 2-5 have 94, 98, 82 and 81 % of their row's stamina, set against the sharper needle | `data/Stages.hpp` (`kSpiritPct`) |
| Duel cap | 12 turns, then the larger share of stamina wins (the spirit on a tie) | `Duel.hpp` |
| Spirits' levels | the stage's band: Valley 1-2, Climb 4-5, Scree 7-9, Ridge 10-12, Summit 13-15; a boss one above the band; +10 a lap (held to level 20) | `data/Stages.hpp`, `Encounter.hpp` |
| Between duels | each stretch of trail gives back 25 % stamina; a hut fills it; so does arriving at a boss | `Run.cpp` |
| A loss | back to the last hut: -30 trail minutes, -1 item, full stamina, partners kept | `Run.cpp` |
| Items | Gel +15; Water +8 and clears Drain; Salt tab +4 and clears Slow; Poles +2 Form for the duel; Headtorch (no dark penalty); a bag of 6 | `data/Items.hpp` |

## The simulated player

`Tests/Host/tools/Policy.hpp`. It is a sensible player, not a perfect one:

- below 35 % stamina it fuels (Water when drained, else a Gel) if it has one;
- otherwise it takes the action worth most now: a move's damage (less Surge's
  cost), or Settle's damage plus what it recovers (up to what is missing); no
  Surge at 8 stamina or less.

Its timing beat is drawn from a skill profile:

| Profile | Perfect | Good | Miss | Mean damage |
|---|---|---|---|---|
| casual | 25 % | 45 % | 30 % | x0.98 |
| good | 45 % | 40 % | 15 % | x1.15 |
| menu-only | 0 | 100 % | 0 | x1.00 |

Q4: the profiles are the rates on stage 1's needle. On a sharper needle
(`Policy.hpp`, `scaledFor`) perfects scale with the sweep (a faster needle is
harder to place) and good-or-better with the good zone's width, so a casual
player at the summit lands about 19 % perfect and 42 % good-or-better. The
player also answers a boss's tell (Settles) 50 % of the time when casual, always
when good, never menu-only.

The simulated player always plays the best Skill pick (the one the duel opens on), so
choosing from the pack (21 October) changes nothing here: a human can do the same
or pick an effect move instead.

So the menu-only mode (no beat) plays like a casual player who uses the beat,
which is the intent: the beat rewards practice, and leaving it off costs nothing
against a casual player. The profiles are guesses; Q3 measures real presses on
the watch.

## Spirit by spirit (`balance_sim`)

Each row is 10,000 duels per profile against that spirit at a level drawn from
its stage's band (a boss at its fixed level), in the stage's two weathers half
and half, never in the dark. The runner is at the stage's par level for a
regular spirit, one above for the boss, at full stamina, with a Gel and a Water
(two Gels for a boss). From stage 2 the runner has a pack of three partners from
earlier stages (one of each affinity) and plays the best of them as Technique;
**Solo** is the same duel with no partners, for comparison.

| Spirit | Stage | Runner | Casual: won, turns | Good | Menu-only | Solo, casual | Check |
|---|---|---|---|---|---|---|---|
| Red Fox | 1 | L1 alone | 85 %, 5.4 | 91 %, 4.6 | 84 %, 5.4 | 86 % | ok |
| Brown Bear | 1 | L1 alone | 90 %, 4.3 | 94 %, 3.7 | 91 %, 4.3 | 90 % | ok |
| Ruby Beetle | 1 | L1 alone | 84 %, 3.0 | 92 %, 2.6 | 86 %, 2.8 | 84 % | ok |
| Sand Crab | 1 | L1 alone | 89 %, 3.1 | 94 %, 2.7 | 91 %, 2.9 | 89 % | ok |
| Old Tortoise (boss) | 1 | L2 alone | 76 %, 6.4 | 91 %, 5.5 | 89 %, 6.4 | 76 % | ok |
| Fire Newt | 2 | L4 with pack | 86 %, 5.0 | 90 %, 4.4 | 84 %, 4.9 | 79 % | ok |
| Gnat Swarm | 2 | L4 with pack | 89 %, 4.1 | 93 %, 3.5 | 95 %, 3.7 | 75 % | ok |
| Barn Owl | 2 | L4 with pack | 83 %, 5.1 | 90 %, 4.5 | 89 %, 4.7 | 81 % | ok |
| Pygmy Shrew | 2 | L4 with pack | 87 %, 3.7 | 92 %, 3.2 | 90 %, 3.7 | 75 % | ok |
| The Great Stag (boss) | 2 | L5 with pack | 75 %, 5.0 | 92 %, 4.4 | 69 %, 5.6 | 40 % | ok |
| Rock Goat | 3 | L8 with pack | 89 %, 4.3 | 93 %, 3.8 | 90 %, 4.0 | 70 % | ok |
| Mist Wisp | 3 | L8 with pack | 92 %, 4.4 | 96 %, 3.8 | 98 %, 3.6 | 84 % | ok |
| Wall Lizard | 3 | L8 with pack | 81 %, 3.5 | 88 %, 3.1 | 88 %, 3.2 | 71 % | ok |
| Marmot | 3 | L8 with pack | 87 %, 3.8 | 92 %, 3.3 | 93 %, 3.3 | 72 % | ok |
| The Ram King (boss) | 3 | L9 with pack | 74 %, 4.7 | 84 %, 4.1 | 93 %, 3.2 | 50 % | ok |
| Wind Sprite | 4 | L11 with pack | 82 %, 3.1 | 87 %, 2.8 | 92 %, 2.5 | 69 % | ok |
| Raven | 4 | L11 with pack | 87 %, 4.3 | 92 %, 3.7 | 98 %, 3.4 | 47 % | ok |
| Snow Hare | 4 | L11 with pack | 88 %, 3.2 | 92 %, 2.8 | 95 %, 2.4 | 77 % | ok |
| The Golden Eagle (boss) | 4 | L12 with pack | 77 %, 4.2 | 91 %, 3.8 | 95 %, 2.9 | 52 % | ok |
| Ghost Goat | 5 | L14 with pack | 86 %, 4.6 | 91 %, 4.2 | 97 %, 3.5 | 60 % | ok |
| Snow Fox | 5 | L14 with pack | 88 %, 3.9 | 91 %, 3.5 | 96 %, 3.1 | 77 % | ok |
| Mist Wisp | 5 | L14 with pack | 83 %, 5.1 | 89 %, 4.6 | 94 %, 4.5 | 52 % | ok |
| The Summit Wolf (boss) | 5 | L15 with pack | 78 %, 4.9 | 93 %, 4.6 | 93 %, 3.8 | 30 % | ok |

Turns are the mean over all duels, won or lost. The Mist Wisp appears twice on purpose:
it follows you from the scree to the summit, stronger.

## Whole games (`run_sim`)

4,000 games per profile through the real game flow (`Run`): random choices at
every fork, every offer to join accepted (the partner from the oldest stage
leaves when the pack is full), obstacles and duels played by the policy above.
"Level at the boss" is on first arriving there; "Par" is the boss table's level.
Play time is estimated at 5 s a duel turn, 6 s a stretch of trail, 6 s an
obstacle and 4 s an offer: screens only, no reading.

**Casual.** Every game reached the summit; level 17.0 at the end. About 14 minutes.

| Stage | Par at the boss | Level at the boss (mean, range) | Duels won / lost | Regular duels won | Boss attempts | Boss first time | In the dark / wind | Play time |
|---|---|---|---|---|---|---|---|---|
| 1 Valley Path | 2 | 2.0 (1-3) | 3.5 / 0.4 | 89 % | 1.2 | 80 % | 0 % / 0 % | 2 min |
| 2 Switchback Climb | 5 | 5.3 (4-7) | 3.7 / 0.8 | 83 % | 1.3 | 73 % | 0 % / 0 % | 2 min |
| 3 Scree Fields | 9 | 8.5 (7-11) | 4.4 / 2.0 | 71 % | 1.6 | 52 % | 7 % / 0 % | 3 min |
| 4 Ridge Line | 12 | 11.9 (10-15) | 3.8 / 1.3 | 73 % | 1.3 | 70 % | 15 % / 49 % | 2 min |
| 5 The Summit | 15 | 15.0 (12-18) | 4.3 / 1.9 | 71 % | 1.6 | 57 % | 26 % / 0 % | 3 min |

**Good.** Every game reached the summit; level 16.4 at the end. About 11 minutes.

| Stage | Par at the boss | Level at the boss (mean, range) | Duels won / lost | Regular duels won | Boss attempts | Boss first time | In the dark / wind | Play time |
|---|---|---|---|---|---|---|---|---|
| 1 Valley Path | 2 | 2.1 (1-3) | 3.2 / 0.2 | 94 % | 1.0 | 93 % | 0 % / 0 % | 1 min |
| 2 Switchback Climb | 5 | 5.2 (4-7) | 3.3 / 0.4 | 88 % | 1.1 | 89 % | 0 % / 0 % | 2 min |
| 3 Scree Fields | 9 | 8.2 (7-10) | 3.9 / 1.3 | 76 % | 1.4 | 65 % | 1 % / 0 % | 2 min |
| 4 Ridge Line | 12 | 11.6 (10-14) | 3.5 / 0.9 | 77 % | 1.1 | 82 % | 4 % / 47 % | 2 min |
| 5 The Summit | 15 | 14.6 (12-18) | 3.8 / 1.2 | 74 % | 1.3 | 74 % | 19 % / 0 % | 2 min |

**Menu-only.** Every game reached the summit; level 16.6 at the end. About 11 minutes.

| Stage | Par at the boss | Level at the boss (mean, range) | Duels won / lost | Regular duels won | Boss attempts | Boss first time | In the dark / wind | Play time |
|---|---|---|---|---|---|---|---|---|
| 1 Valley Path | 2 | 2.0 (1-3) | 3.3 / 0.3 | 90 % | 1.1 | 89 % | 0 % / 0 % | 2 min |
| 2 Switchback Climb | 5 | 5.2 (4-7) | 3.8 / 0.7 | 87 % | 1.3 | 68 % | 0 % / 0 % | 2 min |
| 3 Scree Fields | 9 | 8.4 (7-11) | 3.9 / 1.1 | 78 % | 1.3 | 68 % | 1 % / 0 % | 2 min |
| 4 Ridge Line | 12 | 11.7 (10-14) | 3.4 / 0.6 | 83 % | 1.1 | 86 % | 6 % / 50 % | 1 min |
| 5 The Summit | 15 | 14.6 (13-17) | 4.0 / 1.1 | 82 % | 1.4 | 64 % | 25 % / 0 % | 2 min |

## Reading the results

- **The duels are where they should be.** Every regular spirit is 81-94 % for a
  casual player, fresh, in 3.0-5.4 turns; every boss is 74-78 % (after Q4's twists
  and sharper needle, tuned back into the targets: see "Q4 tuning" below).
- **A real run is harder than a fresh duel**, by design: the runner arrives part
  recovered (a stretch of trail gives back a quarter), with whatever is left in
  the bag, a pack picked by chance, and sometimes in the dark. Regular duels in
  a run are won 71-89 % of the time, and the first try at a boss 52-80 %.
- **The summit is the hardest place**, and meant to be: snow costs the runner
  stamina every turn, a quarter of runs get there after dark, and the Summit
  Wolf alternates charge and guard. A casual player needs 1.6 tries at it on
  average, a good one 1.3. This is the "serious summit". If it is too much, the
  first levers are snow (not scaling it by level) and the Wolf's stamina.
- **Partners matter most for bosses.** Without a pack, the Great Stag falls to 40 %
  and the Summit Wolf to 30 %; regular spirits drop about 10 points. Collecting
  spirits is not optional flavour.
- **The XP curve keeps up.** The runner reaches every boss within a level of par
  on average, in every profile (within half a level for a casual player), with no grinding (the simulated player never
  goes back for more fights; it cannot, as the trail only goes forward).
- **Night and wind come from the clock and the weather**, not from the route: the
  trail clock runs on from stage to stage, so by the Ridge some runs are in the
  dark, and the Ridge is windy half the time. The headtorch in the Ridge's rescue
  box takes away the dark's penalty.
- **The run is short.** About 12-15 minutes of screens for the whole Ridge,
  before any reading, animations or dialogue. Even at three times that, a first
  run is under an hour, against the brief's 2 to 3 hours. This is a decision for
  Gate Q1 (below), not a balance fault.

## How the numbers got here (Q1 tuning)

The first draft (the plan's numbers) failed the targets badly. Three of its
rules changed in Q1 beside the steps below: damage uses the stat the
move's affinity names (the draft used Form for every move), so pace, form and
grit all count; the beat's range is wider (Miss x1/2 and Perfect x3/2, from x3/4
and x5/4), so practice shows; and Surge is 14 power (from 12). Then, in order (the spirits by their names since Q3):

1. **Bosses were unwinnable and regular spirits too easy.** Spirits were re-based
   on the runner (stamina a per cent of the runner's, stats offsets from the
   runner's at the same level), so each row tunes on its own and stays the same
   fight as both level up.
2. **The Red Fox was too hard alone.** The runner had no move that beats a Pace
   spirit (Grit does). Settle became a Grit move.
3. **Duels got longer with every level**, because stamina grew faster than
   damage. Damage, recovery and drain now scale with level ((9 + L)/10), as
   stamina does.
4. **Runs ground through many losses.** XP to the next level became 8 + 4L (from
   20L), and each stretch of trail gives back a quarter of stamina, so a runner
   does not limp from duel to duel.
5. **Bosses were rarely beaten at the first try.** Arriving at a boss fills
   stamina: a breather before the big one, as a runner would take.
6. **Wind punished only the runner, and hard**: -2 pace made the runner slower,
   likelier to miss and weaker with Surge, so the Ridge was a wall. It now takes
   1 from every Pace move, on both sides.
7. **The lead partner was a chore.** In the first draft R1 played a lead partner
   chosen before the duel, which was often the wrong one for the spirit met, and
   changing it would be one more menu on a watch. The partner best placed against
   the spirit now plays its move as Technique automatically.
8. **Stages 1 and 2 were pitched too high** for the level a runner actually
   reaches. The bands were lowered (Valley 1-2, Climb 4-5).
9. **The simulator ran every duel in clear weather.** Duels now use the stage's
   two weathers half and half, as the game does. That showed rain at the valley
   and snow at the summit hurt more than it looked, and mist made the climb's
   Grit spirits drag past six turns. The Old Tortoise's stamina came down (135 to
   120 %), the Fire Newt and the Barn Owl were made shorter and sharper (less stamina, one more
   grit), and the Summit Wolf's stamina came down (115 to 105 %).

## Q4 tuning (19 October 2026)

Two things changed the fights: the needle sharpens stage by stage, and each boss
has a twist (a warning, then a charge or a guard). Run as they were, the
simulator's casual player fell to 44-55 % against the later bosses and to
66-78 % against several regular spirits. Without shortening the needle's ladder
(Jon asked for it) the spirits gave:

10. **A stage-level softening**, `kSpiritPct` (the stages' spirits at 100, 94,
    98, 82 and 81 % of their row's stamina), tuned so the mean casual win in each
    stage is about 87 %. It is a stage's lever, not a species', because the Mist
    Wisp lives in two stages and the needle is a stage's.
11. **Each boss's own stamina** was then tuned so a casual player wins 74-78 %.
    The Wall Lizard was left at 75 % and came down (125 to 113 %).

Every row is inside the targets again, and every run in every profile reaches
the summit. Menu-only play, which never reads a tell, wins 69 % against the Great
Stag (65-85 % is the target): the twists are meant to be read.

## The second lap (Q4 drop 2, 20 October 2026)

`run_sim [runs] [casual|good|menu] --laps 2` carries on after the summit with
`Run::startNextLap()`: spirits ten levels up (`kLapLevelStep`, held to level 20),
the needle one lap sharper (a 1500 ms sweep at stage 1 down to 1100 ms at the summit),
the runner's level, pack and bag kept, the clock on to the next morning.

**Why ten.** At the first try, five (the old step) made lap 2 a walk-over: the
runner arrives at level 17-20, so the valley's spirits at levels 6-7 were won
100 % of the time, and so were most others. Ten, with spirits and the runner
both held at level 20, makes lap 2 a victory lap in stages 1-2 that builds to a
real fight from stage 4: the summit is as hard as in lap 1, with a sharper
needle. Thirteen only made stage 3 too hard without helping stages 1-2, because
a runner at level 17 is simply far above a valley spirit. The runner does not
lose levels, so a harder start would need a different design (lap spirits
relative to the runner's level, or a level cap on lap 2); that is for Jon.

Casual, 4,000 games (every run of every profile finishes both laps):

| Stage | Par at the boss | Level at the boss (mean, range) | Duels won / lost | Regular duels won | Boss attempts | Boss first time | In the dark / wind | Play time |
|---|---|---|---|---|---|---|---|---|
| 1 Valley Path | 12 | 17.9 (15-20) | 3.1 / 0.0 | 100 % | 1.0 | 100 % | 0 % / 0 % | 1 min |
| 2 Switchback Climb | 15 | 19.7 (18-20) | 3.1 / 0.0 | 99 % | 1.0 | 99 % | 0 % / 0 % | 1 min |
| 3 Scree Fields | 19 | 19.9 (19-20) | 3.3 / 0.2 | 96 % | 1.1 | 89 % | 0 % / 0 % | 1 min |
| 4 Ridge Line | 22 | 20.0 (20-20) | 3.4 / 0.8 | 75 % | 1.0 | 93 % | 0 % / 48 % | 2 min |
| 5 The Summit | 25 | 20.0 (20-20) | 3.9 / 1.7 | 67 % | 1.2 | 79 % | 3 % / 0 % | 3 min |

For good and menu-only play lap 2 has the same shape and is easier: at the
summit regular spirits are won 78 % (good) and 89 % (menu) and the Summit Wolf
96 % and 92 % at the first try. Against the targets (regular spirits 80-95 %, bosses 65-85 %), lap 2 is above them
in stages 1-3 (a victory lap, on purpose) and, for a casual player, a little under
the regular target at the Ridge (76 %) and the summit (66 %); bosses are 79-93 %
at the first try.

## What the simulators do not know

- **Real presses.** The beat profiles are guesses. Q3 logs real Perfect, Good
  and Miss rates on the watch; if they differ much, re-run with them.
- **Real choices.** The simulated player picks forks at random and takes every
  offer. A thoughtful player (picking routes for terrain, keeping one partner of
  each affinity) will do better; that is the point of the choices.
- **Reading time.** Play time counts screens, not the lines a player reads.
- **Laps beyond the second.** `run_sim --laps 3` finishes every run, but only
  lap 2 is tuned and reported (below).

## For Gate Q1

**Decided, 18 October 2026** (NOTES, "Gate Q1: result"): a short run is right,
"a quick game on a watch, not a console"; the targets, the summit and the rule
changes stand as proposed. The questions as they were put:

1. **The targets** above (regular 80-95 % in 3-6 turns; bosses 65-85 % fresh;
   each boss within a level of par; every run finishes).
2. **The summit's difficulty**: about 1.8 tries at the Summit Wolf for a casual
   player. Keep it, or soften it.
3. **The length of a run**: about 15 minutes of screens, perhaps 30-45 minutes
   in play, against the brief's 2-3 hours. Options: accept a short first run
   (it suits 2-5 minute watch sessions, and the second lap and the Daily Trail
   give it more life); make the stages bigger in Q4 (up to 12 places each, more
   forks); or both.
