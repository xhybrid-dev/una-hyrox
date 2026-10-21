# Trail Quest: notes

Findings, decisions and phase records. The brief (`TRAILQUEST_BRIEF.md`) says
what the game is; this says what we found out and decided while building it.
Newest at the bottom of each section. Where this and the brief disagree, this
is right. British English.

## Decisions

| Date | Decision | By |
|---|---|---|
| 17 Oct 2026 | Tone: funny start, serious summit (stages 1-2 light, 4-5 darker) | Jon |
| 17 Oct 2026 | Scope: the full 5 stages, gated, so we can stop early | Jon |
| 17 Oct 2026 | Planned extra: Daily Trail (seed from the date), phase Q5 | Jon |
| 17 Oct 2026 | Art: generated pixel art in the repo (`Tools/Art/gen_art.py`), PNG override per asset if something needs hand-finishing | Jon |
| 17 Oct 2026 | Name stays "Trail Quest" for now | open |
| 17 Oct 2026 | Core lives in the service, the GUI is a thin renderer; fallback is the core in the GUI if the message round trip shows as lag | Claude |
| 17 Oct 2026 | Font: Press Start 2P (SIL OFL 1.1), 16 px, 1 bit a pixel | Claude |
| 17 Oct 2026 | Losing a duel sends you back to the last hut (with a small cost), not to the stage start | Jon |
| 17 Oct 2026 | Each stage's boss is a big Trail Spirit; the summit's is the Summit Wolf | Jon |
| 17 Oct 2026 | A beaten spirit offers to join; accept or decline; with a full pack, name who leaves | Jon |
| 17 Oct 2026 | No lead partner: R1 plays the partner best placed against the spirit met (Q1 balance) | Claude, for Gate Q1 |
| 17 Oct 2026 | Balance targets as in `BALANCE.md` | agreed at Gate Q1 |
| 18 Oct 2026 | A short run is right: "a quick game on a watch, not a console" (about 15 minutes of screens for the Ridge) | Jon |
| 18 Oct 2026 | Gate Q2 is played on the watch (the CI build), not in the simulator | Jon |
| 18 Oct 2026 | The game saves after every step that ends on the map (not only at huts) | Jon |
| 18 Oct 2026 | Route choices on a top-down map of the stage | Jon |
| 18 Oct 2026 | The beat is judged in the GUI (Beat.hpp is header-only) and sent as its result, so "PERFECT" shows with no round trip | Claude |
| 18 Oct 2026 | R2 held 1.5 s leaves from any screen; a quick R2 is the screen's own (Fuel in a duel) | Claude |
| 18 Oct 2026 | This build stops after the Valley Path (Session::kPlayableStages = 1) until Q4 | Claude |
| 18 Oct 2026 | Creatures are named for how they look, colour and animal (the Red Fox, the Old Tortoise); the runner's jokes stay in their lines and moves | Jon, at Gate Q2 |
| 19 Oct 2026 | The needle sharpens each stage and lap (faster, a narrower good zone; perfect stays two frames); the later stages' spirits soften to match | Jon asked, Claude set the numbers |
| 19 Oct 2026 | Q4 excitement = boss twists (a tell, then a charge or a guard); rare spirits and trail events left for later | Jon |
| 21 Oct 2026 | Fatigue carries over: trail gives back 8 % (was 25), no refill at a boss, the next stage half; the hut is a detour at the second place of every stage (the cache beside it); more Gels and drops; lap 2 spirits are relative to the runner's level with more stamina | Jon asked, Claude set the numbers |
| 21 Oct 2026 | The needle: the target moves each strike (30-85 % of the sweep), and its speed and zone vary by Surge / Settle / Skill, boss and hazard stat; the green stays 200 ms (the two-frame floor); spirits softened again to match | Jon asked, Claude set the numbers |
| 21 Oct 2026 | R1 (Skill) opens a pick list of the pack: Trail craft or one creature's move (its signature), with damage and effect shown; one move each. Reverses Q1's "no lead partner" (the pick is per turn, in the duel, so it can be right for the spirit met) | Jon asked, Claude built |
| 20 Oct 2026 | A second lap puts the spirits 10 levels up (was 5), held to 20; early stages are a victory lap, the summit a real fight; for Jon to react to | Claude |
| 19 Oct 2026 | Q4 in two drops: stages 2-3 first, then 4-5, the summit finale and the second lap | Jon |
| 18 Oct 2026 | Story at the key moments: an opening tale, each stage's start, its boss ahead and beaten; the map says where each way leads | Jon, at Gate Q2 |

## Findings from the SDK (17 October 2026)

Checked at SDK commit `a7a995a`, and re-checked at `bd04a5b` (sdk-v1.6.0),
which CI pins since the merge of 17 October: only finding 3 changed (and
LVGL's pool can now be enlarged per app, see finding 10). Paths are in `una-sdk/`.

1. **10 frames a second.** `SDK::GUI::Config::kFrameRate = 10`
   (`Libs/Header/SDK/GUI/Config.hpp:26`). The kernel sends one `EVENT_GUI_TICK`
   per 100 ms and LVGL renders at most one frame per tick
   (`Docs/Tutorials/RunLVGL/ARCHITECTURE.md`, "Tick and Frame Loop"). A
   full-screen frame costs about 40 ms of it on the watch. *This overturns the
   brief's 30 fps assumption.*
2. **Input is one code per frame.** The codes are click `1 2 3 4`, press
   `q w e r`, release `a s d f` for L1 L2 R1 R2, and `z` for the L1+R2 chord
   (`Libs/Header/SDK/GUI/Button.hpp`). The port posts one to the active screen
   per frame, so a press is only known to the nearest 100 ms. Long press is not
   a code: a screen derives it from press and release. *Measured in the
   simulator:* press, click and release arrive on three consecutive frames
   (`e`, `3`, `d`). The watch's order is not yet measured (Gate Q0).
3. **The buzzer had no pitch; since SDK v1.6.0 it may.** `RequestBuzzerPlay::Note` is `{time ms, volume}`,
   10 notes at most, volumes 0, 33, 66, 100 only
   (`Libs/Header/SDK/Messages/CommandMessages.hpp:378`). The vibro takes up to 8
   notes of 21 named effects or pauses (`:403`). So no chiptune; the sound is
   rhythm, and the vibro carries the meaning.
   **Changed by SDK v1.6.0** (`bd04a5b`): `RequestBuzzerPlay::Note` gains
   `frequency`, in Hz, 500-20000; 0 or anything outside that range plays the
   kernel's default. It fills what was padding, so the note's size is unchanged,
   and "a kernel that predates this field plays every note at its default"
   (`CommandMessages.hpp`, `RequestBuzzerPlay`). So pitched notes, and short
   tunes of up to 10 notes, may be possible, **if the watch's firmware is new
   enough**. Not yet tried on a watch. Until it is, the game must still work as
   rhythm alone (a watch on older firmware hears every note at one pitch).
4. **A game can hold the backlight.** `RequestBacklightSet` has a brightness and
   an `autoOffTimeoutMs` (`CommandMessages.hpp:359`), which RunLVGL uses. The
   SDK says there is no screen-off signal to an app and the backlight is a timed
   pulse (`Docs/service-lifecycle.md` section 6). The service asks for 30 s on
   GUI start and after every beat press. *Whether it works as hoped is a Gate Q0
   check.*
5. **The display is round.** X Weigh-in's source says so ("the round display",
   `Screens.cpp`), and the SDK's `Buttons` widget draws arcs at the four corners
   of a bezel. I did not find a statement in the SDK docs, so this is *to
   confirm on the watch*. Consequence: the 240×240 frame has dead corners, and
   text must fit the chord at its height. At y = 205 the visible width is about
   170 px; at y = 225 about 116 px (2 * sqrt(120^2 - 105^2)).
6. **The palette grid is 0x00, 0x40, 0x80, 0xC0 a channel**
   (`Libs/Header/SDK/GUI/Color.hpp`). The port keeps the top two bits of each
   RGB565 channel (`Libs/Source/Port/LVGL/LvglPort.cpp:90-98`). *This corrects
   the plan, which said 0/85/170/255* (that is how the launcher icon PNGs are
   snapped, which is a different path). `gen_art.py` fails if a palette entry is
   off the grid.
7. **Indexed LVGL images are no saving.** LVGL v9 decodes them to 32-bit ARGB in
   its 40 KB pool at draw time (`Utilities/Scripts/lvgl_assets/lvgl_assets.py`
   docstring; `RunLVGL/ARCHITECTURE.md`, "Assets"). The port's `lv_conf.h` does
   not enable I1 drawing or the canvas widget. So sprites are our own packed
   4-bit format (288 bytes a frame), and each on-screen sprite expands into a
   static RGB565A8 buffer of its own (`gui/Sprite.cpp`), which the software
   renderer draws without decoding. *This overturns the plan's "indexed 4-bit
   LVGL images".*
8. **The GUI process links none of the library sources.** Anything the GUI uses
   from `Libs/Core` must be header-only (hybridx-race NOTES 3.4). The simulator
   links everything, so it hid this; the watch build caught it
   (`undefined reference to TrailQuest::clampBeat`). `Beat.hpp` and
   `Feedback.hpp` are therefore header-only, and `Libs/Core/Sources/` is empty
   for now. Remember this for every core module the GUI touches.
9. **The service and the GUI need not share a clock.** A beat press carries
   both timestamps from the GUI's clock, and the core takes them as inputs.
   That clock is LVGL's tick, `lv_tick_get()`, which the SDK's port makes wrap
   at 2^32 (`LvglPort.cpp`, `Port::tickCb`), so every difference is an unsigned
   delta (tested across the wrap). The kernel's own `sys.getTimeMs()` returns
   to 0 short of 2^32: never mix the two.
10. **Memory.** `GUI_RAM_LENGTH` is 900K and the stack 24 KB in Weigh-in; we
    copy those. Measured for the Q0 build (watch target, compile check): GUI ELF
    173,956 B text + 1,484 B data + 159,820 B bss = **335,260 B** (RunLVGL is
    about 450-490 KB), of which the sprite buffers are 20,736 B and the LVGL
    pool 40 KB (since SDK v1.6.0 an app may enlarge it by defining
    `LV_MEM_SIZE` in its GUI compile definitions); service ELF 16,220 B. The `.uapp` is 185,864 B. In the
    simulator the probe screen uses 9 KB of the pool, peak 33 %.
11. **A press is judged at the frame on screen.** Each frame the port delivers
    service messages, then one button code, then runs LVGL's timers and renders
    (`LvglPort.cpp`, `Port::run`). So when the R1 code arrives, the screen still
    shows the needle drawn on the previous tick. The GUI therefore sends the
    time that needle was drawn for, not "now", which would judge every press a
    frame (100 ms) late. Measured in the simulator: the judged time is 100 ms
    before the code's arrival. The press also carries the zones as drawn, and
    the service judges with those, so what is on screen is what is judged.

## Platform answers still to get (they need the watch)

- Real render time per frame (the probe prints `r last/worst ms`).
- Whether the 30 s backlight hold holds.
- The order and spacing of button codes on the watch (the probe prints them).
- Which button the system keeps for itself. The probe uses R2 to leave, as
  Weigh-in's BACK does, via `mKernel.sys.exit()`; the SDK's RunLVGL GUI should be
  read for anything more before Q2 relies on it.
- The app package size limit.

## Toolchain notes (this container)

- Local watch builds use Ubuntu's `arm-none-eabi-gcc` plus syscall stubs
  (`hybridx-race/docs/experiments/syscall_stubs.c`), so they are **compile
  checks and never go on a watch**; CI's build is the one to install.
- The stubs must be compiled with the SDK's flags, or the link fails with "uses
  VFP register arguments": `-mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard
  -mthumb` (`cmake/una-app.cmake:101-103`).
- The packaging step needs the Python packages `pyelftools` (and `Pillow`).
- Build recipe used:

  ```
  mkdir -p trailquest/build     # git-ignored, so nothing here can be committed
  arm-none-eabi-gcc -c hybridx-race/docs/experiments/syscall_stubs.c \
      -o trailquest/build/stubs.o \
      -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb
  cmake -G "Unix Makefiles" -S trailquest/Software/Apps/TrailQuest-CMake \
      -B trailquest/build/arm -DCMAKE_EXE_LINKER_FLAGS=$PWD/trailquest/build/stubs.o
  make -C trailquest/build/arm
  ```
- The SDK can also log its own frame numbers: configure with
  `-DUNA_LVGL_FRAME_STATS=ON` (as RunLVGL's `-DRUNLVGL_FRAME_STATS=ON`) for one
  line per 100 ticks with render and hand-off times.
- Simulator: `cmake -S trailquest/Software/Apps/LVGL-GUI/simulator -B
  trailquest/Software/Apps/LVGL-GUI/simulator/build -G Ninja` (needs
  `libsdl2-dev`); screens with `docs/experiments/capture_screens.sh` (needs
  `xvfb xdotool imagemagick`).

## Phase Q0: feasibility (17 October 2026)

**Built.** The app folder (from X Weigh-in's skeleton), the version file for the
`trailquest-v*` tags, CI wiring (build, collect, host tests), the art generator
and its first three sprites (the runner, Stitch, Bonk), the pixel font, the icon,
and the probe screen:

- a scene (sky, hills, path, sun) with three animated 2× sprites;
- the four-arc button bezel (L1 amber: play the selected feedback pattern; L2
  white: next pattern; R1 green: press the beat; R2 red: leave);
- the timing beat: a 1.6 s sweep, a 400 ms "good" zone and a 200 ms "perfect"
  zone (two frames: the floor), judged by the service;
- six feedback patterns (Hit, Miss, Perfect, Level up, Win, Lose);
- live numbers: render time last/worst, frames a second, LVGL pool use, the
  round trip of a beat press, and the last three button codes.

**Verified.**

- Host tests: 22 pass after the review below (16 for the beat: zone edges,
  the fold round the loop for odd and even sweeps, zones that always fit the
  track, the track mapping, the wrap of LVGL's tick, the two-frame floor; 6
  for the feedback table: kernel limits, known effect numbers, supported
  volumes, patterns distinct from each other). The watch and simulator builds
  also check the feedback table's limits and effect numbers against the SDK
  at compile time.
- Simulator: builds with no warnings from our code; a captured run shows the
  press reaching the service ("beat 1: perfect, 15 ms") and the answer one frame
  (100 ms) later. Screens in `docs/screens/q0-*.png` (cut to the round face).
- Watch target: builds and links (compile check with stubs, not installable);
  `TrailQuest_0.0.0-dev.uapp`, 185,864 bytes. CI builds the real one.
- Art: `gen_art.py` passes its palette check; sheet in
  `docs/screens/q0-art-sheet.png`.

**Not verified (cannot be, here).** Anything on a real watch: how it looks,
the real frame time, the haptic and buzzer feel, the backlight hold, the button
order. The simulator's render times (1-3 ms on a PC) say nothing about the
watch's roughly 40 ms.

**Gate Q0: what Jon must test.** Install the `.uapp` from the CI run's
`watch-apps` artifact on branch `claude/x-trailquest-q0`, the way the other
apps go on. Then:

1. Does it look right? The runner, the fox and the bear should read at a glance;
   the colours should match `docs/screens/q0-2-press-1.png`; nothing clipped by
   the round edge.
2. Is the beat playable? Press R1 ten times, aiming for the green zone. How many
   are PERFECT? (If you cannot hit it on purpose, say so: the sweep is too
   twitchy at 10 frames a second and we fall back to a pause-and-pick meter.)
3. Read the stats line (`r a/b ms  f  p%`): the render time, the frame rate (10
   is the best it can do) and the pool use. Note the numbers.
4. The key line (`e 3 d rt100`): what order do the codes arrive in on the
   watch, and what is `rt`?
5. Feedback: L2 picks a pattern, L1 plays it. Could you tell the six apart with
   your eyes closed?
6. Does the screen stay on for about 30 s after the last R1 or L1 press?
   (L2 does not tell the service anything, so it does not renew the hold.)
7. Does R2 leave the app cleanly?
8. Optional: leave it running 10 minutes and note the battery drop.

**Decisions for Jon at the gate.**

- Keep the timing beat (and its zones), or go menu-only / a pause-and-pick meter?
- Is the look right (palette, font, sprite style), or should it change before
  more art is made?
- The name.

## Gate Q0: result (17 October 2026)

Jon installed a CI build of the probe and ran it on his watch: run 168
(`5b215fb`), or run 169 (the merge of 17 October, same Trail Quest code, SDK
v1.6.0); which one is not recorded. His verdict: "that probe looks amazing and the battle probe works really
well". **Gate Q0 is passed** on the two questions it existed for: the look, and
whether the timing beat is playable at 10 frames a second.

Read from his photo of the probe on the watch:

| What | On the watch | Notes |
|---|---|---|
| Display | Round, as assumed (finding 5) | Nothing clipped; the scene's square corners fall outside the face as planned |
| Render time | 4 ms last, 16 ms worst in a second | Far under the 100 ms frame and the ~40 ms full-screen figure: LVGL redraws only what changed (the needle, the sprites, a label) |
| Frames | "f11" | The counting window closes on the first tick at or after 1000 ms, so it can catch an eleventh frame; the rate is the kernel's 10 a second |
| LVGL pool | 14 % | Lower than the simulator's 23 % |
| Beat | PERFECT, -61 ms | A press landed on purpose in the 200 ms zone |
| Round trip | 97 ms | One frame: the service's answer arrives on the next tick, so the core can stay in the service (no lag to justify moving it) |
| Button codes | `q 1 a` for L1 | Press, click, release, as in the simulator (finding 2) |
| Bezel | UP, DOWN, SELECT, BACK printed by L1, L2, R1, R2 | Matches the arcs: R1 (SELECT) is the action, R2 (BACK) leaves |

Seen in the photo, for the screens to come: the 16 px result line ("PERFECT
-61") reaches the L2 and R2 bezel arcs at its ends. Lines at that height should
stay inside about 150 px, not the full chord, so they clear the arcs.

**Decisions at the gate.**

- Keep the timing beat, with the zones as built (perfect 200 ms, good 400 ms,
  a 1.6 s sweep), until the balance work in Q1 says otherwise.
- The look stands: the palette, Press Start 2P and the sprite style.
- The name is Trail Quest (decided before the gate).

**Not reported yet** (none blocks Q1): whether the six feedback patterns can be
told apart blind, whether the 30 s backlight hold holds, whether R2 leaves
cleanly, and the battery over 10 minutes. Ask Jon when convenient; record here.

**Next:** Q1, the core and balance (no GUI), starting with plan mode.

## Phase Q1: the game core and balance (17 October 2026)

The whole game engine, pure C++ with no screens, so that Q2 is mostly screens.

**Built.** All in `Software/Libs/Core`, namespace `TrailQuest`: integer maths,
fixed-size structs, no heap, every table index checked (an id out of range reads
the first row, never past the end).

| Module | What | Header-only (the GUI can use it) |
|---|---|---|
| `Rng.hpp` | xorshift32; `below(n)` by rejection (no modulo bias); `chance(pct)`; the state is one `uint32_t`, saved with the game | yes |
| `Types.hpp` | the three affinities and the wheel; stats; effects (Drain, Slow, Rattle); terrain; weather; time of day from the trail clock | yes |
| `data/Moves.hpp` | 47 moves: the runner's three, and two for every spirit | yes |
| `data/Species.hpp` | 22 Trail Spirits: 17 regular (Doubt lives in two stages) and 5 bosses; numbers relative to the runner at the same level | yes |
| `data/Items.hpp` | Gel, Water, Salt tab, Poles, Headtorch (a key item); a bag of 6 | yes |
| `data/Stages.hpp` | the five stages: 9 places each, four forks, a hut, a cache or a find, two obstacles, the boss; 10 obstacles | yes |
| `Progress.hpp` | levels 1-20, stats by level, the XP curve, the partner bonus | yes |
| `Duel.hpp/.cpp` | the duel: a turn in, a log of events out (for the GUI to play back); deterministic from the setup, the turns and the Rng | no (service) |
| `Encounter.hpp/.cpp` | which spirit and level a place gives (night spirits only outside full daylight); obstacle odds; finds | no |
| `RunState.hpp` | the whole saved game in 48 bytes | yes |
| `Run.hpp/.cpp` | the game flow: travel, obstacles, duels, huts, caches, offers to join, losses, stage clears, laps; says when to save | no |
| `SafeFile`, `SaveCodec`, `SaveStore` | the save: "key=value" text (`tq=1`, under 384 bytes), strictly checked on load; written via `.tmp` with a `.bak` (SafeFile is Knitting's); load tries the file, then the `.bak`, then a new game | no |
| `DailyTrail.hpp` | the Daily Trail seed from the date (the stage generator is Q5) | yes |

Tools, built with the host tests (`Tests/Host/tools/`): `balance_sim` (every
spirit, 10,000 duels a row per skill profile), `run_sim` (whole games through
`Run`), and the simulated player they share (`Policy.hpp`). Their results, the
targets and the tuning history are in **`BALANCE.md`**.

**Design changes from the brief and the plan** (these supersede both):

1. **No lead partner.** R1 (Technique) plays the move of the partner best
   placed against the spirit met, or Trail craft with no partners. A lead chosen
   before the duel was often the wrong one for the spirit met, and changing it
   would be one more menu on a watch.
2. **Settle is a Grit move** (the plan had Form), so the runner has an answer
   to every affinity: Surge (Pace), Settle (Grit), Trail craft (the terrain's).
3. **Damage uses the stat the move's affinity names**, so pace, form and grit
   all count in a duel, and scales with level, so duels stay 3-6 turns all the
   way up. The beat matters more: Miss x1/2, Perfect x3/2.
4. **Stamina comes back on the trail**: a quarter for each stretch walked, and
   all of it on arriving at a boss (a breather before the big one).
5. **A loss costs 30 trail minutes** (the plan said 60), one item and the walk
   back from the hut; partners and XP are kept.
6. **Spirits are relative to the runner** at the same level (stamina a per cent,
   stats offsets), and the stages' level bands were lowered to what a runner
   actually reaches: Valley 1-2, Climb 4-5, Scree 7-9, Ridge 10-12, Summit 13-15.
7. **Wind is symmetric**: Pace moves are 1 weaker for both sides (it was -2
   pace for the runner only, which made the Ridge a wall).
8. **Five kinds of item**, not about ten (brief section 5); Q4 can add more. The
   plan's "Recover" effect was dropped: Settle and items do that.
9. **Partners do not level on their own.** A partner lends +1 to its affinity's
   stat and its move, which grows with the runner's level, so it "levels up with
   you" without a second XP track.
10. **The trail clock runs on from stage to stage**, so slow runs reach the
    Ridge and the Summit at dusk or after dark; the Headtorch (in the Ridge's
    rescue box) takes away the dark's -10 % to hit.

**Findings** (details in `BALANCE.md`):

- Every regular spirit is won 81-94 % of the time by a casual player at par, in
  3.0-5.4 turns; every boss 76-81 % fresh. In a real run (part recovered, a pack
  picked by chance, sometimes in the dark), 74-89 % and 44-84 % at the first try.
- The menu-only mode plays like a casual player who uses the beat (x1.00 against
  x0.98 on average), so leaving the beat off is never a handicap for most people;
  a good beat player does clearly better.
- Partners matter most for bosses: without a pack The Steep is 25 % and the
  Summit Wolf 13 %.
- The summit is the hardest place: snow, the dark in a quarter of runs, and the
  longest fight. A casual player takes 1.8 tries at the Summit Wolf.
- A whole crossing of the Ridge is about 15 minutes of screens (12 for a good
  player), against the brief's "2 to 3 hours". A decision for the gate.
- The first balance table ran every duel in clear weather, which hid how much
  rain (the valley) and snow (the summit) cost. It now uses the stages' own
  weather, and four spirits were retuned for it (`BALANCE.md`, step 9).

**Verified.**

- Host tests: **119 pass**, 97 of them new in Q1: Rng 8 (sequence, range,
  evenness, zero seed, save round trip); Data 10 (every move, spirit and item
  complete and named once, stats by level, the XP curve); World 9 (every stage:
  Start first and Boss last, exits only forward so no loops or dead ends, every
  place reachable and able to reach the boss, real choices, parameters in range,
  lookups bounds-checked, time of day); Duel 24 (damage worked by hand for each
  rule: the wheel, terrain, rain, wind, the beat, the finishing push; hit chance;
  turn order; fuel; poles; effects; the boss's rest; the turn cap; replay from a
  seed); Encounter 5 (night spirits, level bands, obstacle odds by hand, finds);
  Run 26 (the whole flow: travel, obstacles, huts, caches, the headtorch, the
  dark, losses back to the hut, XP and levels, offers and a full pack, the best
  partner, stage clears, laps, resuming, replay from a seed); Save 12 (round
  trips, a resumed game carries on exactly as the original, missing, doubled,
  out-of-range and inconsistent values refused, a torn file falls back to the
  `.bak`, nonsense in both gives a new game, an invalid state is never written);
  DailyTrail 3.
- `balance_sim`: all 23 rows inside the targets. `run_sim`: 4,000 games for each
  of casual, good and menu-only, and every one reached the summit.
- Watch target: builds and links (compile check with stubs, not installable). All
  six core sources compile for the Cortex-M33 with the SDK's flags and no
  warnings: 17.6 KB of code and 6.6 KB of tables as compiled (each file carries
  its own copy of the tables it uses, which the linker merges). The `.uapp` is
  185,752 bytes, the same as Q0's, because nothing calls the core yet: Q2 wires
  it into the service.
- Simulator: builds with the core; the probe is unchanged.
- CI: see the run on the pushed commit.

**Not verified (cannot be yet).** Anything on screen (Q2). Real beat rates: the
skill profiles are guesses until Q3 logs presses on the watch. The save on the
watch's own file system (the host tests use a stand-in, as Knitting's do).
Balance on the second lap (`run_sim` stops at the summit; Q4).

**Gate Q1: what Jon must decide.** Read `BALANCE.md` ("For Gate Q1") and the
roster in `DESIGN.md`, then:

1. Agree or change **the targets**: a regular spirit at par 80-95 % won in 3-6
   turns; a boss 65-85 % fresh; each boss reached within a level of par; every
   run finishes.
2. **The roster**: the 22 names, and each spirit's line (`DESIGN.md`). Change
   any you like; the numbers do not depend on the names.
3. **The summit**: keep the Summit Wolf at about 1.8 tries for a casual player,
   or soften it.
4. **The length of a run**: about 15 minutes of screens (perhaps 30-45 in play)
   against the brief's 2-3 hours. Accept a short first run (the second lap and
   the Daily Trail add more), make the stages bigger in Q4 (up to 12 places, more
   forks), or both.
5. **The rule changes** above, chiefly: no lead partner, stamina back on the
   trail and before a boss, a loss costing 30 minutes.

Still open from Gate Q0, whenever convenient: can the six feedback patterns be
told apart blind; does the 30 s backlight hold; does R2 leave cleanly; the
battery over 10 minutes.

## Gate Q1: result (18 October 2026)

Jon: "Short run is good, it's a quick game on a watch, not a console. Let's
progress to Q2." **Gate Q1 is passed.** The run's length (about 15 minutes of
screens for the Ridge) is accepted as it is; the targets, the roster, the
summit's difficulty and the rule changes stand as proposed (`BALANCE.md`).

## Phase Q2: stage 1, playable (18 October 2026)

**Decided with Jon at the start** (see Decisions): Gate Q2 is played on the
watch; the game saves after every step; route choices on a top-down map. So the
watch-side work the brief left for Q3 (saving on the watch, haptics in play)
came forward; Q3 keeps sound (pitched jingles), battery and whatever the watch
test turns up.

**Built.**

- **Core** (`Software/Libs/Core`, all host-tested):
  - *Save after every step.* `Run` asks to save whenever a step ends on the
    map, at an offer to join, at a stage clear and at the summit; never in the
    middle of an obstacle or a duel. `RunState` gains `offer` (a beaten spirit
    waiting for an answer), so quitting at an offer comes back to it. The save
    accepts any place but an unbeaten boss's (`SaveCodec`, key `offer`).
    Quitting mid-duel comes back to the last step: the stretch is walked again.
  - *A stirred resume.* `Run::resume(state, stir)` mixes the service's clock
    into the saved chance, so coming back after quitting mid-duel meets a fresh
    spirit (quitting cannot dodge a fight, nor replay it). Stir 0 replays exactly.
  - *`Session`* (service-only): the game as the service runs it. Commands in
    (new game, travel, obstacle, duel turn, join, advance), views out
    (`View.hpp`: `WorldView`, `DuelView`, plain structs the GUI reads), and when
    to save. A command that does not fit the moment is refused and changes
    nothing.
  - *Text that fits* (`TextFit.hpp`, `Layout.hpp`, `Narrate.hpp`, header-only):
    the round face's width at each row; the rows every screen uses; and every
    line the game says (each duel event, every card), built to fit. Press Start
    2P is monospace, so fitting is arithmetic, and the host tests prove every
    name, move, item, place, exit and narrated line fits where it is drawn,
    clear of the bezel arcs.
  - *The map's layout* (`data/MapLayout.hpp`): all five stages share one shape,
    so one layout serves them; tests check every stage matches it, every place
    is on the face and clear of the text, and no path crosses a place.
  - *`HoldTimer.hpp`* from Knitting (with its tests), for the holds.
  - The settings (the beat on or off) in their own file, `settings.txt`.
- **Messages** (`Libs/App/Header/Commands.hpp`): World and Duel snapshots from
  the service; Hello, NewGame, Travel, ObstacleTry, DuelTurn, Join, Advance,
  Settings and FxPlay from the GUI, each numbered; the World echoes the number.
  The Q0 probe's BeatPress/BeatResult are gone. Every message is asserted under
  256 bytes (the World is about 120, the Duel about 180).
- **Service**: loads the save and settings, runs the Session, answers every
  command with the World (and the Duel while there is one), then saves if asked;
  holds the backlight 30 s after each command. One log line a step.
- **GUI** (`Software/Apps/LVGL-GUI`): a screen manager (HybridX Trail's, but no
  heap: two static slots), and seven screens: the title, the map, an obstacle,
  the duel, an offer to join, the pack (three pages) and the stage clear.
  Shared widgets: corner words by the arcs, a card, bars, the beat bar, flat
  shapes for the map (Streak's `Paint`). Every screen: R2 held 1.5 s leaves (a
  red arc grows on its bezel), L1+R2 shows the developer numbers. A click
  counts only if its press came to the same screen outside the beat; a slow
  press with no click counts on its release. An answer that does not come
  within 2 s is asked for again. The Q0 probe screen is gone.
- **Art**: Blister (a beetle), Chafe (a crab) and Old Mossback (a giant
  tortoise, drawn on 32x32, so it stands taller); sprites now carry their size.
  Sheet: `docs/screens/art-sheet.png`.
- **Scripts**: `docs/experiments/capture_screens.sh` (every screen of a scripted
  run, `docs/screens/q2-*.png`, replaced by `q3-*.png` in Q3), `docs/experiments/sim_play.sh` (plays stage 1
  to the clear with the beat off, then leaves and checks the save is picked up).

**Findings.**

- *The bezel arcs eat text.* The first screens had words under the arcs ("Play"
  on R1's, "Beat" on L2's, an exit name between L2's and R2's). The arcs sit at
  a radius of 110-116 px in four spans, so the layout now keeps every label's
  box inside 109 px and every centred line clear of the spans (a host test,
  from the SDK's arc angles). Consequences: a card is a title of 12 characters
  and four lines of 13; exit names are 10 at most (some renamed: "The stream",
  "Straight", "The pool"); the corner words are at most 5 at the top, 6 at the
  bottom left.
- *Text that does not fit is caught by the tests, not on the watch.* They found
  five spirits' lines too long for a card (reworded: Bonk, Niggle, The Shale
  King, Hypo, The Knife-Edge), two moves too long for a line ("Side scuttle",
  "Antler rush") and a card line about a full bag.
- *A strike's click is not a tap.* The button that strikes the beat sends its
  click a frame later, after the result has arrived, and it dismissed the
  result card at once. Fixed in the screen base (above).
- *The beat in the simulator*: the needle starts when the choosing button's
  click arrives, about 0.2 s after it goes down, so a script strikes PERFECT 1.3 s
  after choosing. A player reacts to the needle they see, so this does not touch
  play; it makes the scripts reliable.
- *A player who misses every beat loses.* An auto-player striking at a fixed
  0.9 s lost six duels running (Miss is half damage). With the beat off (every
  turn Good) it clears the stage, as `run_sim`'s menu-only player does.

**Verified.**

- Host tests: **170 pass** (51 new: Session 8, TextFit 7, Layout 8, MapLayout 5,
  HoldTimer 17, Run 3, Save 3; and the old ones updated for the new save rule).
  Among them: every save asked for in 60 played games reloads exactly; stage 1
  played to the clear by commands alone, 40 seeds; quitting at an offer comes
  back to it; a stirred resume meets other spirits.
- `balance_sim` and `run_sim`: unchanged (the save rules touch no numbers).
- Simulator: builds clean; `sim_play.sh` clears stage 1 (54 presses) and the save
  is picked up after leaving; `capture_screens.sh` runs a whole stage with the
  beat on, two duels won, one obstacle, the boss, all 26 screens. LVGL's pool
  peaks at 49 % (of 37 KB) on any screen.
- Watch target: builds and links (compile check). GUI ELF 198,000 B text +
  4,396 data + 173,252 bss = 375,648 B (the sprite buffers 33 KB, the screen
  slots 1.2 KB); service ELF 33,752 B, now with the engine; `.uapp` 232,868 B
  after the review's fixes.
- CI: run 174 green on the first Q2 commit (`1651487`); the fixes' run follows.

**Not verified (cannot be, here).** Anything on the watch: how it reads at arm's
length, the map's redraw time, whether the backlight hold is enough between
turns, the haptics in play, whether a slow press sends a click on the watch,
battery. That is Gate Q2.

**Review before Gate Q2.** An independent review of the Q2 commit found three
things, all fixed and checked in the simulator:

1. *The bezel arcs were missing* on the map after the pack, a duel, an obstacle
   or an offer, and on the title after a stage clear: the screens chose the
   arcs' colours while building, before the arcs existed. The screen base now
   keeps the colours and applies them once the arcs are made.
2. *The L1+R2 chord* (the developer numbers) would also have counted as an L1
   tap and an R2 tap on the watch (in a duel, a Surge). The chord now drops any
   press in progress and any hold. The simulator has no chord, so this is
   untested until the watch.
3. *A lost message* could replay the previous step (the screen took the answer
   to its "asking again" for the answer to its command), or leave a duel
   showing Choose after it had ended. The World now carries the last command's
   number and the last Hello's separately, so a command that never ran is told
   apart from a slow answer, and the screen lets the player try again; a duel
   ends when the World says so.

**Gate Q2: what Jon must test.** Install the CI build of this branch
(`watch-apps` artifact, `TrailQuest_*.uapp`), the way the Q0 probe went on. Then:

1. Play stage 1 from the title to Old Mossback (about 10 minutes, perhaps a few
   tries). On the title R1 plays; on the map L1 picks the way, R1 goes, L2 shows
   the pack; in a duel L1 Surge, L2 Settle, R1 the skill (its name in green),
   R2 fuel; after choosing, any button strikes the beat.
2. Is it fun? What would make it more so?
3. The beat every turn: right, or a chore? (L2 on the title turns it off.)
4. Is the text readable at arm's length? Anything clipped by the edge or the arcs?
5. Is the map clear: which way you are choosing, where you are, what is where?
6. Do the buzzes help (a hit, a miss, a perfect, a win, a loss)?
7. Leave in the middle (hold R2 1.5 s) and come back: are you where you were?
8. Still open from Q0: does the screen stay on long enough between turns;
   battery over the session.

## Gate Q2: result (18 October 2026)

Jon played stage 1 on the watch (the CI build): "It's working incredibly well."
**Gate Q2 is passed.** Two notes, both taken into Q3 (below):

1. "I'm not convinced on the naming of the creatures as chafe and blister etc,
   can they be typical creatures based on their appearance." Decided: colour
   and animal.
2. "A bit hard to follow the journey. Perhaps a few text story explaining
   screens could help build a story?" Decided: story at the key moments.

## Phase Q3: the Gate Q2 notes (part 1, 18 October 2026)

Q3 is "what the Q2 test turns up", then sound and the beat. Part 1 is Jon's two
notes; part 2 (pitched jingles, the beat tuned from real strikes) and the
battery over a session follow.

**Built.**

- **Names.** Every spirit is named for how it looks; ids, numbers and lines
  are unchanged, so saves and the balance are untouched (`balance_sim` and
  `run_sim` re-run: every number the same). One line changed (the stag's: it
  leaned on "The Steep") and two places ("Tortoise pool", "Stag's crag").

  | Stage | Spirits | Boss |
  |---|---|---|
  | 1 Valley Path | Red Fox, Brown Bear, Ruby Beetle, Sand Crab | Old Tortoise |
  | 2 Switchback Climb | Fire Newt, Gnat Swarm, Barn Owl, Pygmy Shrew | The Great Stag |
  | 3 Scree Fields | Rock Goat, Mist Wisp, Wall Lizard, Marmot | The Ram King |
  | 4 Ridge Line | Wind Sprite, Raven, Snow Hare | The Golden Eagle |
  | 5 The Summit | Ghost Goat, Snow Fox, the Mist Wisp again | The Summit Wolf |

  The `Spirit::` ids and the sprites (`gen_art.py`: fox, bear, beetle, crab,
  tortoise) follow; the art itself is unchanged.
- **Story** (`Libs/Core/Header/data/Story.hpp`, header-only): pages of a
  headline (9 characters), a body (five lines of 12 on the card) and who
  stands in the scene. The opening tale (six pages: the Ridge, the spirits, a
  duel and the beat, the wheel, the pack, the map and leaving); for each of the
  five stages, two pages as it starts (the place, then its boss), one at the
  boss and one when it is beaten. All five stages are written, so Q4 has them.
  `StoryPlan` plays one or two stories in a row (a new game: the opening, then
  stage 1's).
- **The story screen** (`gui/screens/StoryScreen.*`): the headline, the card,
  and a strip of the stage's sky and ground with the runner and the spirit or
  boss the page is about. R1 the next page (OK on the last), L1 skips. When it
  ends it goes where the game is (`screenFor`), so it needs no command and
  cannot get out of step with the service. Where it plays: a new game (title,
  eight pages, then the map); a new stage (Q4's); the boss ahead, the first
  time this game (not on a retry after a loss); the boss beaten, before the
  stage clear. Picking a save up still says "Welcome back".
- **The map says where each way leads**, on a new row under the way's name:
  "A spirit", "A hazard", "A hut", "Supplies" ("Nothing" once taken), "Quiet",
  "The boss". The stamina bar moved down under it. The map's stage-start card
  now says only the day's weather and the time ("Clear today", "It's dawn."):
  the story has said where you are, but the weather changes the fight (rain
  dulls Form moves, wind Pace ones), so it stays. (The plan dropped this card;
  kept, as one short card, for that reason.)

**Verified.**

- Host tests: 174 pass (was 170): every story page fits its card and has a
  headline that fits row 28; the story's rows and the map's new row are clear
  of the bezel arcs; every way on every map says something that fits; a plan
  pages through its stories in order and never past them.
- Simulator: `capture_screens.sh` (now `docs/screens/q3-*.png`, 38 screens, the
  q2 set replaced) plays the opening tale, the map with its new row, both boss
  pages and the clear; `sim_play.sh` still clears stage 1 (64 presses, 10 of
  them story pages) and the save is picked up. LVGL pool peak 49 % (the story
  screen 27-36 %).
- Watch compile check: links; GUI 379,532 B (+3.9 KB), service 33,816 B,
  `.uapp` 236,692 B.

**Gate Q3 (part 1): what Jon must test** (the CI build, `watch-apps`):

1. Start a new game (hold L1 on the title over the save). Read the opening tale:
   the right length, or skip it with L1? Does it make the game clear?
2. Do the names read better? Any you would change?
3. Is the journey easier to follow: the stage's pages, the boss's page before
   the fight and after, and the map's row saying where each way leads?
4. The copy, line by line (`Libs/Core/Header/data/Story.hpp`; the pages are
   captured in `docs/screens/q3-02` to `q3-09`, `q3-32` and `q3-36`): change anything.
5. Still open: battery over a 30-minute session.

### Jon's watch test of Q3 part 1 (19 October 2026)

Battery drain "seems ok considering the use and screen time". Three notes on
the screens, all fixed:

1. "What does Beat mean?" It was the timing beat (press as the needle hits the
   green), but the word said nothing. The title's L2 is now **Timing**, with
   "(on)" or "(off)" under it.
2. The title was cramped between where you are and L2's word: a line now sits
   between them, and the rows are spaced out.
3. In a duel the words at the top ("Surge Skill" over the technique) touched
   each other, sat on the bright sky, and nothing said what Surge and Skill do.
   The rows are 20 px apart (was 16), on a darker band, and the first duel of a
   game opens with a help card (three pages: L1 Surge and L2 Settle; R1 Skill
   and R2 Fuel; the needle, only while the beat is on). Any of L1, L2 or R1
   pages on. `Narrate::helpPage`, host-tested to fit.

## Phase Q4: the rest of the Ridge (19 October 2026, drop 1)

Jon after the watch test: the needle is "fairly easy but good. Could increase in
speed and accuracy for later levels"; "continue through Q4 with some exciting
adventures". Decided: boss twists as the excitement; two drops, stages 2-3 first.

**Built (drop 1).**

- **The needle** (`Beat.hpp`, `beatFor(stage, lap)`): stage 1 unchanged; each
  stage 100 ms faster with a good zone 40 ms narrower; each lap 100 / 20 more,
  floored at 1000 / 200. Perfect stays 200 ms (two frames: the floor at 10 fps).
  `BeatBar::create` takes the config; the duel and the obstacle pass the
  stage's.
- **Boss twists** (`Duel.cpp`, `data/Species.hpp`): `Duel::tell()` says what the
  coming turn is. Charge: the boss's hit x2, or x1/2 if the runner chose Settle;
  Guard: the runner's damage x1/2. The Old Tortoise guards every 4th turn, the
  Great Stag and the Golden Eagle charge and the Ram King guards every 3rd, the
  Summit Wolf alternates. `DuelView::tell`; the duel screen shows a warning card
  first (`Narrate::twistTell`) and the playback says the effect
  (`Charged`, `Braced`, `Guarded` events).
- **Balance** (`BALANCE.md`, "Q4 tuning"): the simulated player scales its
  beat rates to the needle and answers tells 50 % (casual) or always (good). The
  stage-level softening `kSpiritPct` (100, 94, 98, 82, 81) and each boss's own
  stamina put every row back in the targets; every run reaches the summit.
- **Art**: ten sprites, stages 2 and 3 (Fire Newt, Gnat Swarm, Barn Owl, Pygmy
  Shrew, Great Stag; Rock Goat, Mist Wisp, Wall Lizard, Marmot, Ram King), same
  palette; the sheet wraps into rows.
- **Stages 2 and 3 are open** (`kPlayableStages = 3`): the stage clear goes on to
  the next stage's story pages. The route graph tests (no loops, no dead ends,
  every place reachable) already covered all five stages.

**Verified.** Host tests 184 (the beat ladder, the tell schedule per boss, a
charge doubling and a Settle bracing it, a guard halving, every warning and event
line fits its card, stages 1-3 played through the real session);
`sim_play.sh` plays stages 1-3 (513 presses for a plain R1-only player, 9 losses)
and picks the save up at the stage 3 clear; `capture_boss.sh [stage]` starts just
before a boss from a written save and captures its page, warning card and twist
(`docs/screens/q4-stag-*`, `q4-ram-*`); the watch compile check links (GUI
388,260 B, service 34,224 B, `.uapp` 245,940 B).

**Gate Q4a: what Jon must test** (the CI build, `watch-apps`): play stages 1-3.

1. Do the boss warnings and twists feel fair and exciting? Is the text clear
   (what to do on a Charge, on a Guard)?
2. Is the sharper needle right at stages 2 and 3 (still fair, a little harder)?
3. Do the new creatures read well on the watch? Any you would redraw?
4. Is each stage's difficulty about right (you can win, you can lose)?

## Phase Q4: the rest of the Ridge (20 October 2026, drop 2)

Plan mode first, as the rules ask. Jon's Gate Q4a notes are not in the repo or
the request, so there was nothing to fix first; drop 2 started on his
instruction. (The session's branch is `claude/hybridx-race-workspace-setup-kppnmz`;
`claude/x-trailquest-q0` no longer exists.) Baseline re-run: 184 host tests,
`balance_sim` 23 rows OK, every `run_sim` run reaches the summit.

**Built (drop 2).**

- **Art**: seven sprites in `gen_art.py` (Wind Sprite, Raven, Snow Hare, Golden
  Eagle 32x32, Ghost Goat, Snow Fox, Summit Wolf 32x32), mapped in `gui/Art.hpp`
  (no spirit falls back to the Red Fox any more). The Snow Fox is white and
  blue, crouched, long and low with a raised brush tail; the Ghost Goat is
  sky-blue with hollow eyes and fades into wisps. Sheet: `docs/screens/art-sheet.png`.
- **All five stages open** (`kPlayableStages = 5`). `Session::advance()` from the
  last stage's clear card goes on to `Finished`, so the summit shows "WELL RUN"
  first, then the finale. `Narrate::stageClear` lost its "coming soon" case.
- **The summit finale**: `ClearScreen` in a second mode (not a new screen: one
  fewer slot in the screen manager) when the phase is `Finished`: "THE SUMMIT",
  a card of the lap, level, duels won and lost, and the pack (`Narrate::finale`,
  fit-tested at the extremes). R1 "Again" starts the next lap (the stage 1 story
  pages and the map); L2 "Back" goes to the title, and the save stays at the
  summit, so Play brings the finale back.
- **Lap 2**: `kLapLevelStep` 5 to 10 (see `BALANCE.md`, "The second lap", for why:
  at 5 every spirit in stages 1-4 was a walk-over). `run_sim --laps N` plays N laps.
- **Tools**: `capture_finale.sh` (the finale and lap 2's start from a written
  save), `sim_play.sh` plays all five stages (`STAGES=3` for fewer).

**Verified.**

- Host tests 184 pass (the five-stage `SessionTest` plays all five stages by
  commands, then "Run again" into lap 1 with a sharper needle; `LayoutTest` fits the
  finale at its extremes). `balance_sim 10000` unchanged (23 OK; lap 1 does not
  use the step). `run_sim` lap 1 unchanged for all profiles; with `--laps 2` and
  `--laps 3` every run finishes.
- Simulator and watch target build (watch: compile check, `.uapp` 251,336 B, no
  warnings from our code; the SDK's own warnings are as before).
- Captures: `docs/screens/q4-eagle-*`, `q4-wolf-*` (page, choose, tell, twist) and
  `q4-finale-*`. `sim_play.sh` (R1 only, beat off) cleared all five stages in 621 presses (9 losses, level 17 at the summit) and the save was picked up at the summit.

### Story and text rewrite (20 October 2026)

An independent read of the text found a thin arc and some wrong or vague help.
Jon: no gags that mean nothing ("your calves have notes"); put real running into
the journey (low on energy, take a Gel; cramp, a Salt tab; the climb, fuel
before it). Done (`Story.hpp`, `Species.hpp`, `Narrate.hpp`; `DESIGN.md` has the
voice rule and the new roster lines):

- **Opening** is now eight pages and teaches the game with real advice: pace
  yourself; spirits are trail troubles (stitch, bonk, cramp); the needle; the
  wheel; ENERGY (a Gel before the wall); CRAMP (a Salt tab; Water for drain);
  the pack; the map (huts restore, it saves itself).
- **Every stage's pages** say what the ground asks of a runner and hint at what
  the boss does and what helps (Gel before the Stag, poles on the scree, a low
  stride in wind, eat and drink in the cold); the boss-ahead page gives the
  tactic (Settle to brace, the Ram King's guard halves your hits).
- **The grey shape** is seeded: glimpsed after the Stag, Ram King and Eagle, then
  waiting at the top (the wolf page and its arrival page agree: it kept pace).
- **Spirit lines** are the trouble each one stands for (the Red Fox a side stitch,
  the Brown Bear the bonk, the Ruby Beetle a hot spot, the Gnat Swarm calf cramp,
  the Mist Wisp self-doubt, the Wind Sprite a headwind, ...).
- **Help cards** now four: L1/L2, R1 (the partner's move, else Trail craft: the
  old card named a partner you do not have in the first duel), R2 (Gel energy,
  Water drain, Salt tab cramp), the needle. The Guard tell says "Your hits do
  half damage".
- **Lap 2** opens with its own first pages ("NEXT LAP"), then the Tortoise.
  `story()` and `StoryPlan` take the lap.
- Not changed: the finale card (stats) and the join offer's wording. Tests: 184
  pass; the fit test now covers lap 1's pages too.

### Choose whose move to use (21 October 2026)

Jon, after stages 4-5 ("really good"): Skill is a pack creature's attack but not
clear which, and he cannot choose; choosing would make the pack matter. Built:

- **R1 opens a pick list** once the pack has a creature (with no pack it still plays
  Trail craft at once). A card per choice: the creature (or "On your own"), its move,
  its type and the damage it would do now with a Good beat, "May drain / slow /
  rattle" or "No effect", and "2 of 4". L1 next (it starts on the best pick, so
  R1 R1 plays the old default), R1 use (then the needle), L2 or R2 back.
- The wheel shows in the numbers: against a Form spirit a Pace creature's move hits
  hardest; against Pace, a Grit one. So which creatures you keep decides your options.
- Core: `DuelSetup::skills[4]`/`skillCount`, `PlayerTurn::skill` (kNone: the default,
  so tools and the simulated player are unchanged), `Duel::estimateSkill`,
  `DuelView::skill/skillDmg/skillBest`, `DuelTurn::skill`. One move per creature
  (its signature move), by Jon's choice; both of each creature's moves is a possible
  later step.
- Verified: host tests 189 (new: the skill list, a chosen move plays, an out-of-range
  pick falls back, estimates match, the view, every card fits); `balance_sim` and
  `run_sim` unchanged; simulator captures `docs/screens/q4-skill-*` (via
  `docs/experiments/capture_skill.sh`); the watch build links (`.uapp` 252,896 B).

### A harder, more varied needle (21 October 2026)

Jon: the green is "fairly consistent and easy" (the target was fixed at 3/4 of every
sweep, so the rhythm could be learned), and the timing should vary by challenge. The
perfect zone is already at its two-frame floor, so "harder" means a target that moves
and a faster needle with a smaller orange zone, not a thinner green. Built (`Beat.hpp`,
header-only; `BeatBar::reconfigure`; the duel and obstacle screens; details in
`BALANCE.md`, "The moving needle"):

- the target is somewhere new on every strike (30-85 % of the sweep, from the GUI's tick,
  not the game's Rng, so saves and replays are unaffected);
- the needle's shape by what it is for: Surge faster and tighter, Settle slower and
  wider, Skill the stage's own, a boss tighter, hazards by stat;
- the ladder by stage and lap is unchanged.

The simulated player's cost for the moving target is an assumption; the spirits were
re-tuned to it (all rows inside the targets again; menu-only play is now easier than a
casual player with the needle). Host tests 193 (the new kinds, the target's range and
spread, a moved target judged where it is drawn); `balance_sim` and `run_sim` all OK;
simulator captures `docs/screens/q4-beat-*` (`docs/experiments/capture_beats.sh`).

### Fatigue, the hut as a detour, a harder lap 2 (21 October 2026)

Jon: lap 2 felt the same; health came back too fast so Fuel was rarely needed; fatigue
should carry over, recovered by Fuel or a hut detour off the standard route. Built (details
and numbers in `BALANCE.md`, "Fatigue carries over" and "The second lap"):
- 8 % per stretch (was 25), no refill at a boss, half back on the next stage; huts and
  a loss as before (a loss with no hut visited returns to the stage's start);
- the hut and its cache are a fork: place 2 of every stage can go straight on or to the
  hut (`data/Stages.hpp`; the map layout is unchanged);
- 3 starting Gels, 35 % drops; the simulated player detours to a hut below 60 %;
- lap 2: spirits relative to the runner (+1 level a lap) and +5 % stamina a lap;
- bosses softened and `kSpiritPct` retuned for the run-level difficulty (the fresh-boss
  target is 65-90 % now; the real target is the run's first-try rate, 50-75 % casual).
Host tests 197 (the hut is optional, no boss refill, the small breather, a loss before
any hut returns to the start, lap spirits relative to the runner).

### Pitched jingles (21 October 2026)

Jon: "the buzzes are all the same pitch, it doesn't change, only the beat and tempo;
more jingle would be fine; firmware 1.6+ is mandatory." (He also confirmed the backlight
behaviour is fine, closing that Gate Q0 item.) `Feedback.hpp`: each buzzer note has a
pitch (`hz`, 500-20000, 0 for a rest; the service sets `Note.frequency`, SDK v1.6.0). The six old
patterns are now short jingles (rising for good news, falling for bad), and seven are
new: Boss warning (two-tone alarm, when a boss tells), Stage clear, Finale (the summit's
card), Fuel (an item used), Found (a find or cache), Hut (arriving at one), Join (a
spirit joins; it used Win). Host tests 200: every pitch in range and rests silent,
good news rises and bad falls, none longer than a second (the finale 1.1 s is the
longest), every pair distinct. Not heard on a watch yet: whether the notes read as tunes
on the real buzzer, and whether any is too long or too shrill, are Jon's ears to judge.

### Gate Q4b: result, and Phase Q5 (21 October 2026)

Jon played the full game on the watch: stages 4 and 5 "good and work well"; the moving needle
"much better, more interesting and fun to play"; the backlight behaviour ok; the buzzes
were all one pitch. He asked to ship, with the optional extras as updates after. Since then:
fatigue, the hut detour, a harder lap 2 and the pitched jingles were built (above), so
**Gate Q4b is passed for the build he played, and the final build (fatigue, lap 2, jingles)
is for him to play before the tag.**

**Phase Q5 (release) is prepared:**
- `Utilities/pack-store-zip.sh` (from Knitting's), the CI step and the `trailquest-store-zip`
  artifact (a dry run until a `trailquest-v*` tag); the packager's dry run passes and its
  refusal of a version that is not the tag's works (the development-ID refusal is from Knitting's script);
- the manifest in the portal's form, six store previews (`docs/experiments/store_previews.sh`),
  `CHANGELOG.md` (1.0.0 not yet released) and **`docs/SUBMISSION.md`**: the release day
  step by step, the store text and the facts it rests on.
**App ID:** Jon created the app on the portal and gave the ID `AB98F6B289B6738E`, now in the three places (it replaces the development ID `B5FA567FFD85BA6D`).
**Jon's part:** play the final build; make the tag `trailquest-v1.0.0` (a session cannot push tags);
upload the zip.

### The launcher icon (21 October 2026)

Jon asked for a more unique, engaging logo. Five options were drawn from the game's own sprites
(`Resources/make_icon_options.py`, a comparison sheet in `docs/icons/`): dawn on the ridge, the duel,
the wolf and moon, the needle, the trail map. **Jon chose 2, "The duel"** (the runner facing the Red Fox
under a clash star): it is now `Resources/icon_60x60.png` / `icon_30x30.png`, drawn by
`make_icons.py`, and is the `icon.png` in the store package (checked byte for byte).

**Gate Q4b: what Jon must test** (the CI build, `watch-apps`): finish a full run.

1. Do stage 4 and 5's spirits read well on the watch? Snow Fox against Red Fox,
   Ghost Goat against Rock Goat, the Summit Wolf (a grey wolf with red eyes: does
   it read as a wolf, not a dog or goat?).
2. The Golden Eagle (a dive, every 3rd turn) and the Summit Wolf (charge then
   guard): fair and clear?
3. Stage 4's wind and stage 5's snow and dark: too hard, about right?
4. The summit: "WELL RUN", then the finale card. Does it feel like an ending?
   Anything you want on it (the time, the spirits met)?
5. "Again": is lap 2 right? It begins as a victory lap (the levels carry over) and
   gets hard at the Ridge and Summit, with a sharper needle. Keep that, or make lap
   2 harder from the start?

## Handoff: where we are, and what comes next (20 October 2026)

**State.** Q4 drop 2 is built (see "Phase Q4 ... drop 2"): all five stages open,
art for stages 4-5, the summit finale and "Again" for lap 2, `run_sim --laps`.
Gates Q0-Q2 passed; Q3 played; Gate Q4a (stages 1-3) has no notes recorded.
**Waiting on Jon: Gate Q4b**, a full run on the CI build (steps above); and the
Gate Q4a notes if he has any.

**Next**: whatever Gate Q4b turns up, then Q5: icons and the store package, tags
`trailquest-vX.Y.Z`; Jon uploads to the portal.

**Later, if Jon wants them**: rare spirits; trail events; pitched jingles (needs a
firmware that plays pitch: ask whether the buzzes sounded different); Daily Trail;
a lap 2 that is hard from the first stage (spirits relative to the runner's level,
since the runner arrives at 17-20 and is capped there); more on the finale (time,
spirits met).

**Working notes for the next session.**
- Read the brief, this file's last sections, `BALANCE.md` and `DESIGN.md` first.
  Where the brief and NOTES disagree, NOTES is right.
- Host tests: 184 pass. Re-run `balance_sim 10000` and `run_sim 4000 casual|good|menu`
  (and `--laps 2`) after any number change; every lap-1 row must be OK and every
  run must finish.
- Balance levers, in order: `kSpiritPct` per stage (`data/Stages.hpp`), a boss's
  `staminaPct`, a single spirit's `staminaPct` (`data/Species.hpp`), and for laps
  `kLapLevelStep` (`Encounter.hpp`). `tune_balance.py` moves the first two.
- The container needs `libsdl2-dev gcc-arm-none-eabi libnewlib-arm-none-eabi
  xdotool imagemagick python3-pyelftools python3-pil` (apt) for the simulator,
  captures and the watch compile check; `apt-get update` first.
- Pitfalls met: `pkill -f <script>` kills your own shell (use `pkill -x
  TrailQuestSimul` and `pkill -x Xvfb`); the simulator scripts share the save in
  `Software/Output` and must not run at once; a long foreground command times out
  at 10 minutes (run it in the background and poll); a hand-written save must be
  valid (a boss as a partner is refused, and the game then starts a new run).
- Never edit `assets/sprites/sprites.c` or `gui/include/gui/Sprites.hpp`: change
  `gen_art.py` and regenerate. The palette is 16 fixed entries on the display's
  grid; do not change them without flagging it.
- GUI rules that bite: anything the GUI uses from `Libs/Core` must be
  header-only; text must be proven to fit in `LayoutTest`; one button code a
  frame at 10 fps.
- Commits end with the attribution lines in the session's system reminder; push
  with `git push -u origin <the session's branch>`; no pull request unless Jon asks.

## Open items

- The scene is a few flat shapes. A tile map (the route screens) is not tested
  yet: the LVGL pool is small, so it should be one custom draw callback with
  runs of rectangles, not a widget per tile. Q2 decides this with a measurement.
- The app has no configuration fields, so CI's manifest check is not wired for it.
- Gate Q0's unreported items (feedback patterns blind, backlight hold, R2 exit,
  battery): see the gate record above.
- The second lap is tuned only for the summit end (BALANCE.md): stages 1-3 of lap 2
  are a victory lap. Decision for Jon after Gate Q4b.
- Dialogue is one line a spirit (its intro); a second line for the Mist Wisp's
  return is Q4 content (the bosses' pages are written: `data/Story.hpp`).
- The needle's stage-1 zones are Q0's (a 1.6 s sweep, 400 ms good, 200 ms
  perfect), which Jon finds "fairly easy but good"; later stages sharpen (Q4).
  Real strikes are not logged off the watch, so the simulator's profiles remain
  guesses.

## Review before Gate Q0 (17 October 2026)

A full code review of the branch (high effort) raised 15 findings and one
low item. All were checked against the code and the SDK and all held; all are
fixed:

1. Presses were judged a frame late (finding 11 above). Fixed: judged at the
   needle on screen.
2. The gate steps named the branch `claude/trailquest-q0` (the rename's search
   and replace); it is `claude/x-trailquest-q0`.
3. A zone near either end of the sweep would have been drawn off screen (an
   unsigned underflow) while still being judged. Fixed at the root:
   `clampBeat()` now keeps the good zone inside the track, and the GUI draws
   the needle and the zones with the core's tested `beatToTrack()`.
4. The fold round the loop was one off for an odd sweep. Fixed (`2d` against
   the sweep) and tested.
5. The round trip was timed against the latest press, not its own. Fixed: by
   sequence number.
6. L1 (play a pattern) did not renew the backlight hold. Fixed.
7. The chord width at y = 225 was wrong (130 px; it is 116 px). Fixed here and
   in `DESIGN.md`.
8. The sprite index was not bounds-checked. Fixed, and the sprite header is now
   generated with the table, so the index constants and the count cannot drift.
9. The GUI and the service each made their own beat configuration. Fixed: the
   press carries the zones as drawn.
10. The feedback table's copies of the kernel's limits were not tied to the
    SDK. Fixed: `static_assert`s in `Service.cpp`.
11. The core's doc named the kernel clock; the clock is LVGL's tick. Fixed
    (finding 9), and the test renamed.
12. The GUI re-implemented the core's track mapping. Fixed (item 3).
13. The labels were set with `lv_label_set_text` every second, which allocates
    in LVGL's pool. Fixed: fixed buffers and `lv_label_set_text_static`.
14. An unused font (`poppins_semibold_20.c`) was compiled. Removed.
15. `ModelListener::bind()` and its `model` were dead. Removed.
16. (Low) The build recipe wrote `stubs.o` and `build-arm/` at the repository
    root, outside `.gitignore`. Fixed: they now go in `trailquest/build/`.
