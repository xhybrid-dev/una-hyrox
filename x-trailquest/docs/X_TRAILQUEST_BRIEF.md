# X Trail Quest for UNA Watch: Brief and Plan

**Owner:** Jon
**Status:** Brief only (17 October 2026). Nothing is built yet. Next: Jon's
decisions (section 10), then Q0: a feasibility prototype.
**Read with:** the SDK docs for the platform facts in section 6 (still to be
confirmed in Q0).

---

## 1. The idea

A small Game Boy-style adventure on the wrist. You play a trail runner
crossing a mountain range. You choose the route at each fork, meet the
creatures that live on the trail, and take them on in short turn-based duels
that are won with athletic skill, not magic: pacing, breathing, form and grit.
Win, and a creature joins your pack as a running partner. At the end of each
stage a boss waits at the summit.

It is a game first. Short sessions (2 to 5 minutes), one run through the
stages in about 2 to 3 hours of play, then a harder second lap.

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

- 240×240 display, 64 colours (ABGR2222), software rendered.
- Four buttons (L1, L2, R1, R2), no touchscreen.
- Vibration and a buzzer, usable for hits, crits and the timing beat.
- No MMU, no heap churn in the loop, fixed-size messages (the standing rules).

Design consequences:

- **Turn-based on purpose.** It redraws only on a change, which is cheap and
  kind to the battery; only the timing beat and a few animations need a
  frame loop.
- **Four moves on four buttons**, so no menus to navigate in a duel.
- **A Game Boy look**: small tile-based art on a limited palette, 160×144
  logical resolution scaled up and centred, which keeps sprites tiny and
  memory low. Stage palettes give each terrain its own mood.
- **Art budget**: tiles in flash, with no per-frame allocation. Sprites and
  tiles are generated from PNG files by a build script, with the 2-bit
  conversion the SDK already uses for icons.

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
| **Q0: feasibility** | A throwaway prototype: one scaled 160×144 screen, a tile map, two sprites, a timing bar. Measure frame rate and battery on a real watch | **Gate Q0:** the art looks right and the timing beat is smooth on the watch, or Jon accepts a menu-only duel |
| **Q1: core** | World graph, duel engine, stats, items, save; host tests and the balance simulator | **Gate Q1:** duels are deterministic and balanced on paper (win-rate targets agreed with Jon) |
| **Q2: one stage** | Stage 1 playable end to end in the simulator: map, route choice, 4 creatures, one boss, huts, save | **Gate Q2:** Jon plays stage 1 in the simulator and it is fun for 10 minutes |
| **Q3: service and watch** | Service from RunLVGL, haptics and sound, real-watch build from CI | **Gate Q3:** stage 1 plays on a real watch; battery over a 30-minute session recorded |
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
- Music and sound are our own chiptune, not anything lifted from a game.
- This is a design guideline, not legal advice. If it becomes a paid product,
  have someone check the names and art.

## 10. Decisions for Jon

1. **Name.** "X Trail Quest" is a placeholder (it follows Weigh-in and Velocity).
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
8. **Sound.** Haptics only, or also short chiptune buzzer jingles (if the
   buzzer allows pitch control; to be checked in Q0)?

## 11. Questions for UNA / SDK checks (do not guess; answer in Q0)

- What frame rate can LVGL sustain on the real watch for a scaled 160×144
  tile screen? Can a full-screen redraw at 30 fps run without draining the
  battery?
- Can the buzzer play pitched tones, or only fixed buzzes? How long can a
  vibration pattern be?
- How much flash and RAM can the app use for assets? What is the app package
  size limit?
- Is there a supported way to persist a small save file between runs?
- Are button hold and release events available (for charge-and-release moves)?

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
