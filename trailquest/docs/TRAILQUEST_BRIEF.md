# Trail Quest for UNA Watch: Brief and Plan

**Owner:** Jon
**Status:** Q2 (stage 1 playable) built, 18 October 2026, and waiting at Gate
Q2 for Jon, played on the watch (NOTES, "Phase Q2"). Gates Q0 (17 October) and
Q1 (18 October) passed.
Decisions taken: tone "funny start, serious summit"; full 5 stages; Daily Trail
as the planned extra; generated pixel art; a loss sends you back to the last
hut; bosses are big Trail Spirits; a beaten spirit offers to join; a short run
(a quick game on a watch); saves after every step; a top-down map.
**Read with:** `NOTES.md` (findings, decisions and the gates), `BALANCE.md`
(the numbers) and `DESIGN.md` (the look, the voice and the roster). Where this
brief and `NOTES.md` disagree, `NOTES.md` is right: it was written after the
SDK check, and the sections below were not all rewritten. The corrections are
listed next.

> **Corrections after the SDK check (17 October 2026).**
> 1. The watch GUI runs at **10 frames a second**, not 30, and delivers one
>    button code per frame. Turn-based is right; the timing beat needs wide
>    zones (never under 200 ms) and is optional.
> 2. The buzzer had **no pitch** (a duration and one of four volumes), so no
>    chiptune. *Since SDK v1.6.0 it may have:* each note can carry a frequency,
>    played if the watch's firmware is new enough (NOTES, finding 3). Not yet
>    tried on a watch.
> 3. The display is round, so the game's art is not edge to edge: headings and
>    text sit inside the circle.
> 4. Colours are on a grid of **0x00, 0x40, 0x80, 0xC0** a channel, and indexed
>    LVGL images are not a saving here (the SDK decodes them to 32-bit in its
>    pool). Sprites are our own 4-bit format, expanded into RAM buffers.
> 5. Everything the GUI process uses from the core must be **header-only**: the
>    GUI links none of the library sources.
>
> **Changes in Q1 (17 October 2026)**, which supersede sections 3-5 below (NOTES,
> "Phase Q1", lists them all): there is no lead partner (R1 plays the partner
> best placed against the spirit met); Settle is a Grit move; damage uses the
> stat the move's affinity names; stamina comes back on the trail and before a
> boss; partners do not level on their own; five kinds of item for now; the
> roster is 17 regular spirits and 5 bosses (`DESIGN.md`), and Camber is not in
> it (no river stage). A first crossing of the Ridge is far shorter than the
> "2 to 3 hours" of section 1: a decision at Gate Q1.

---

## 1. The idea

A small Game Boy-style adventure on the wrist. You play a trail runner
crossing a mountain range. You choose the route at each fork, meet the
creatures that live on the trail, and take them on in short turn-based duels
that are won with athletic skill, not magic: pacing, breathing, form and grit.
Win, and a creature joins your pack as a running partner. At the end of each
stage a boss waits at the summit.

It is a game first. Short sessions (2 to 5 minutes), one run through the
stages in well under an hour of play (Gate Q1: "a quick game on a watch, not
a console"), then a harder second lap.

It is not a copy of anything. No Pokémon names, creatures, moves, art, music,
type chart or catchphrases are used. The shared idea is only "meet creatures,
battle them turn by turn", which is generic. Section 9 lists the rules we keep
to so it stays that way.

## 2. The world

**The Ridge** is a 5-stage trail across one mountain range, each stage a
different terrain with its own creatures, weather and boss:

| Stage | Terrain | Feel |
|---|---|---|
| 1. Valley Path | Woods, streams | Gentle; teaches the controls |
| 2. Switchback Climb | Steep forest | Stamina matters |
| 3. Scree Fields | Loose rock, mist | Technical; fall risk |
| 4. Ridge Line | Exposed, wind | Hard choices, big rewards |
| 5. The Summit | Snow, thin air | The final boss |

**Route choices.** Each stage is a small graph of 6 to 9 nodes, so there are
real forks:

- the fast, steep line against the long, easy one;
- the river crossing against the bridge;
- the shortcut through the scree against the safe track;
- stop at the hut to rest, or push on in the dusk.

Each choice trades stamina, time and risk against the chance of a creature, a
rare item or a shortcut. Time of day passes as you go (dawn, day, dusk, night)
and changes what you meet.

**Node types:** trail (nothing happens, maybe a find), creature encounter,
obstacle (a decision with a skill check), rest hut (restore, save, swap
partner), supply cache (item), and the boss.

## 3. Creatures ("Trail Spirits")

Original, athlete-flavoured, and named for what they do to a runner. Working
names only, to be replaced by Jon's:

- **Stitch**: a fox-like creature that gives you a stitch, so it hits stamina.
- **Bonk**: a heavy bear-shape that drains energy late in the stage.
- **Scree-hopper**: a goat that is fast on loose ground.
- **Gale**: a wind spirit that slows your pace on exposed ground.
- **Cramp**: a small, quick pest with a crippling nip.
- **Doubt**: a mist creature that attacks confidence (morale).
- **Camber**: a river otter that is brilliant on wet ground.
- **Summit Wolf**: the final boss.

Roughly 16 creatures in v1 (3 to 4 per stage plus bosses), each a 32×32 sprite
with 2 idle frames. Each has four stats and one or two signature moves.

Beating a creature in a duel makes it a **partner**. You carry up to three.
A partner lends you a move and a small stat bonus, and levels up with you.
There is no capturing and no cages: they run with you, which fits the theme.

## 4. The duel

Turn-based, one screen, four buttons. A duel is 3 to 6 turns, about 30 to 60
seconds.

**Your stats**

- **Stamina** (health): reach zero and you're spent and lose the duel.
- **Pace**: who moves first, and how often a move lands.
- **Form**: damage and defence; drops on rough ground.
- **Grit**: resists morale attacks and powers finishing moves.

**Each turn you pick one of four moves** (one button each):

| Button | Move | Idea |
|---|---|---|
| L1 | **Surge** | A hard push: big effect, costs stamina |
| L2 | **Settle** | Steady rhythm: small effect, recovers some stamina |
| R1 | **Technique** | Uses terrain and your best partner move |
| R2 | **Fuel** | Use an item (gel, water, salt tabs) |

**Terrain and weather change the numbers**, so route choice matters before
the duel starts: Scree favours Form, steep ground drains Stamina faster, and
so on. Instead of a "type chart" there is a simple trade-off wheel (Pace beats
Form, Form beats Grit, Grit beats Pace), kept deliberately plain and
different from any existing game's.

**The skill beat (optional, the part that is not just menus).** After you pick
a move, a half-second timing bar appears. Press at the right moment for a
"good" or "perfect" result. This is the one-button timing mini-game from the
earlier idea list, reused. It can be switched off for a pure menu mode.

**Obstacles** (outside duels) are one-off checks: a fallen tree to hurdle, a
stream to cross, a scree slide to ride out. Each is a single timing beat or a
choice, with a result and a small cost.

## 5. Progress and meta

- **XP and levels** for the runner and partners, to level 20 in v1.
- **Items**: gels, water, trekking poles, a headtorch for night nodes. About 10
  kinds.
- **Save** at huts and between stages (one slot, on the watch).
- **Optional real-life link** (see decision 4): a daily bonus from steps or a
  logged run, as a small buff, never required to progress.
- **Records**: best stage times, creatures met, perfect-timing streaks.

## 6. The watch (what shapes the design)

From the SDK docs checked on 17 October 2026 (to be re-verified in Q0):

- 240×240 display (round, see NOTES), 64 colours (ABGR2222), software rendered
  at 10 frames a second.
- Four buttons (L1, L2, R1, R2), no touchscreen.
- Vibration (8-note melodies of 21 named effects) and a buzzer (10 notes, each
  a duration and a volume of 0, 33, 66 or 100; a pitch since SDK v1.6.0, if the
  firmware supports it).
- No MMU, no heap churn in the loop, fixed-size messages (the standing rules).

Design consequences:

- **Turn-based on purpose.** It redraws only on a change, which is cheap and
  kind to the battery; only the timing beat and a few animations need a
  frame loop.
- **Four moves on four buttons**, so no menus to navigate in a duel.
- **A Game Boy look**: chunky pixel art on a 120×120 logical grid drawn at an
  exact 2×, in palettes of 16 colours taken from the display's 64, with a pixel
  font (Press Start 2P). Stage palettes give each terrain its own mood.
- **Art budget**: sprites are 24×24 logical at 4 bits a pixel (288 bytes), drawn
  from code by `Tools/Art/gen_art.py`. The few on screen are expanded into
  static RGB565A8 buffers (about 7 KB each), outside LVGL's 40 KB pool.

## 7. Architecture

Like the other apps. The game rules are pure C++ with an injected clock and a
seeded random-number generator, so every duel and route is reproducible and
host-tested.

- `Software/Libs/Core` (pure): world graph, route and encounter rules, the duel
  engine, stats and levelling, items, save/load (fixed-size struct, versioned),
  and the RNG. No SDK use except the JSON/CBOR reader for save data.
- `Software/Libs/App`: the service, from RunLVGL; input, haptics, save file.
- `Software/Apps/LVGL-GUI`: screens (map, route choice, duel, partner, items).
- `Data/`: creatures, moves, stages and items as plain tables, checked by a
  host test for consistency (no creature without a move, no dead-end route).
- `Tools/Art/`: PNG to tile/sprite conversion.

Tests worth having from day one: a duel is deterministic for a given seed;
every route in every stage reaches the boss; balance runs (simulate 10,000
duels and report win rates per creature and level, so tuning is data, not
guesswork).

## 8. Phases and gates

Same rules as the other apps: plan mode at the start of each phase, stop at
each gate, summarise what was done, how it was verified and what Jon must test
or decide. Build the watch target and the simulator after every change.

| Phase | Work | Gate: what must be true to go on |
|---|---|---|
| **Q0: feasibility** | A probe screen: a scene, three animated sprites, the button bezel, a timing beat, the feedback patterns, and live frame/memory numbers. Measure on a real watch | **Gate Q0:** the art looks right and the timing beat is playable at 10 frames a second on the watch, or Jon accepts a menu-only duel |
| **Q1: core** | World graph, duel engine, stats, items, save; host tests and the balance simulator | **Gate Q1:** duels are deterministic and balanced on paper (win-rate targets agreed with Jon) |
| **Q2: one stage** | Stage 1 playable end to end: map, route choice, 4 creatures, one boss, huts, save (and, since Jon tests on the watch, the save and haptics on the watch) | **Gate Q2:** Jon plays stage 1 on the watch (the CI build) and it is fun for 10 minutes |
| **Q3: service and watch** | Sound (pitched jingles, if the firmware plays them), the beat tuned from real strikes, what the Q2 test turns up | **Gate Q3:** stage 1 plays well on a real watch; battery over a 30-minute session recorded |
| **Q4: the rest** | Stages 2 to 5, the remaining creatures, items, levelling, second lap | **Gate Q4:** a full run can be finished; no dead ends found by the route tests |
| **Q5: release** | Icons, store package, tags `trailquest-vX.Y.Z` | **Gate Q5:** Jon uploads to the portal |

A realistic size: Q0 to Q2 is the risky part and tells us whether the idea is
worth finishing. Q4 is mostly content and balance, not new engineering.

## 9. Keeping it original

- No Pokémon (or other franchise) names, creature designs, move names, music,
  logos, phrases ("gotta..."), or the type chart. We use our own.
- Creature art is drawn fresh for the theme (trail runners' pains and terrain),
  not shaped after existing monsters.
- "Spirit", "partner", "duel" and "stage" are generic words and fine. We avoid
  words that are marks of another game ("Poké-", "-mon", "trainer badge").
- Sound is our own (rhythms on the buzzer, patterns on the vibro), not anything
  lifted from a game.
- This is a design guideline, not legal advice. If it becomes a paid product,
  have someone check the names and art.

## 10. Decisions for Jon

1. **Name.** "Trail Quest" is a placeholder (it follows Weigh-in and Velocity).
   Other options: "Ridge Runner", "X Ridge", "Summit Spirits".
2. **The pack.** Up to three partners that lend moves (as proposed), or a single
   partner to keep it simple?
3. **Timing beat.** Include the skill beat in duels (more action, more battery),
   or menu-only in v1?
4. **Real-life link.** None in v1 (pure game), or a small daily buff from steps
   or a logged run? It would make the game on-brand for HybridX but adds a data
   dependency, so I'd leave it out of v1.
5. **Length.** Five stages, or three for a smaller first release?
6. **Art.** Who draws the sprites and tiles? Options: you, a freelance pixel
   artist, or generated placeholders first with a pass of hand-finishing.
   About 16 creatures at 2 frames each, plus 5 tile sets and a runner sprite.
7. **Tone.** Light and funny (Stitch, Bonk, Doubt) or more serious and
   atmospheric? The names above lean light.
8. **Sound.** Reopened by SDK v1.6.0: notes can carry a pitch. Short pitched
   jingles (a win, a level-up, a boss) are worth a test on the watch; the
   game must still work as rhythm on older firmware (NOTES, finding 3).

## 11. Questions for UNA / SDK checks (do not guess; answer in Q0)

Answered from the SDK (details and citations in `NOTES.md`): the frame rate is
10 a second; the buzzer has pitch only since SDK v1.6.0 and only on new enough
firmware; press, click and release codes exist;
saving is through `IFileSystem` in the app's folder. Still open for the watch
itself (the Q0 gate): real render time per frame, whether the backlight hold
works, the order button codes arrive in, which button the system keeps for itself,
and the app package size limit.

## 12. Risks

- **Fun risk.** A turn-based game lives or dies on its numbers and content.
  Q2 exists to find out early; the balance simulator helps.
- **Art cost.** Sixteen creatures plus tiles is the biggest job and is not
  engineering. Placeholder art first keeps development moving.
- **Scope creep.** An overworld, trading, multiplayer, and a story each double
  the work. v1 is one linear mountain range with forks, and nothing else.
- **Battery and frame rate.** Mitigated by being turn-based and drawing only
  on change; checked in Q0.
- **Four buttons.** Enough for duels, tight for menus (items, partners). The
  pause menu may need a hold or a long press, to be checked in Q0.
- **Originality.** Section 9; revisit before any public release.
