# Trail Quest: look and voice

How the game looks, sounds and talks. The numbers live in the brief and the
code; this is the feel. Started at Q0 (17 October 2026) and grown as stages are
written.

## Look

- **A Game Boy on a round face.** Chunky pixel art: a 120×120 logical grid,
  each logical pixel 2×2 on the panel, no scaling at run time. Sprites are
  24×24 logical (48×48 shown), outlined, with a two-frame idle.
- **Palette.** Sixteen colours at most a stage, all on the display's grid (each
  channel 0x00, 0x40, 0x80 or 0xC0). Index 0 is transparent, index 1 is the dark
  navy outline. The current set is in `Tools/Art/gen_art.py`.
- **By stage.** 1 green and mist; 2 forest dusk; 3 grey rock; 4 wind-bleached
  blue and orange; 5 cold white, indigo and one red. Brighter and jokier early,
  quieter near the summit.
- **Type.** Press Start 2P at 16 px (twice its 8 px grid) for everything the
  player reads. Developer numbers use Poppins 14 and are not for release.
- **Round face.** Nothing important in the corners. Text is centred and sized to
  the chord at its height: about 170 px wide at y = 205, 116 px at y = 225, 144
  px at y = 24.
- **The bezel.** The SDK's four arcs mark what each button does: amber to
  choose, white to go back, red to discard or leave, green to confirm.

## Sound and touch

- **The vibro does the talking.** Six patterns so far: Hit, Miss, Perfect, Level
  up, Win, Lose. Each must feel different with eyes closed (a host test checks
  they are not identical; the wrist decides the rest).
- **The buzzer is rhythm first**: short and loud for good things, long and quiet
  for bad ones. Never more than a few notes. SDK v1.6.0 adds pitch, which a watch
  on new enough firmware plays (NOTES, finding 3): short tunes become possible,
  but every sound must still read as rhythm alone on older firmware.

## Voice

- British English. Short lines. Dry, a little cheeky, never cruel.
- **Funny start, serious summit.** Stages 1-2 joke about what runners put up
  with (stitches, bonking, doubt). By stage 4 the jokes thin out; stage 5 is
  quiet, and the humour, when it comes, is gentle.
- Creatures are named for the pain they are: Stitch, Bonk, Cramp, Doubt, Gale.
- Never name or echo another franchise (brief section 9).

## The screens (Q2)

Seven screens, one plan (`Libs/Core/Header/Layout.hpp`, checked by host tests
against the round face and the bezel arcs). Captures: `docs/screens/q2-*.png`.

- **The buttons are always in the same place.** A word by each arc says what
  it does now, in the arc's colour: amber L1 (choose), white L2 (back, or the
  pack), green R1 (confirm, go), red R2 (fuel in a duel). R2 held 1.5 s leaves
  from anywhere, and a red arc grows on its bezel while it is held. Every word
  sits inside its arc.
- **Text**: one font (Press Start 2P, 16 px). A headline at the top (the clock,
  a page title, the spirit's stamina), a card through the middle (a title of 12
  characters, then four lines of 13), the runner's stamina at the foot.
- **Title**: TRAIL QUEST, the runner with Stitch and Bonk; R1 plays (or starts a
  run), L1 held starts again, L2 turns the beat on or off.
- **Map**: the stage from above, left to right, on its own ground colour (night
  is navy). Paths in tan; places as icons: a flag (the start), a purple mark
  with "!" (a spirit), grey rocks (an obstacle), a red-roofed hut, a box (a
  cache, grey once taken), the boss's face (grey once beaten). The way you are
  choosing is amber, with its name under the map; the runner is a blinking
  ring with a red cap that walks the line.
- **Duel**: the Q0 scene, its sky by the weather; the runner left, the spirit
  right (a boss bigger). The spirit's stamina at the top, the runner's at the
  foot, with a three-letter tag for an effect (DRN, SLW, RTL). Choosing: the
  four words, and what R1 plays in green under them. Then the beat (any button
  strikes), the verdict (PERFECT green, GOOD gold, MISS red), and the turn,
  event by event, in amber when it goes your way and red when it does not;
  the one hit blinks.
- **Cards** say what happened: a place reached, a find, the hut, a duel won or
  lost, an obstacle cleared or not. R1 (OK) moves on.

## The roster (Q1, for Jon's review at Gate Q1)

Twenty-two Trail Spirits: four or so a stage and a boss at the end of each. The
names, looks and lines are drafts; the numbers do not depend on them, so any can
change. The source is `Software/Libs/Core/Header/data/Species.hpp`; how hard each
one is, `BALANCE.md`. Affinity is its place on the wheel (Pace beats Form beats
Grit beats Pace). "Night" spirits appear only at dawn, dusk or night.

**Stage 1, Valley Path** (light; teaches the wheel)

| Spirit | Looks like | Affinity | Its line | Moves |
|---|---|---|---|---|
| Stitch | fox | Pace | Finds the gap under your ribs and moves in. | Side stitch (drains), Pounce |
| Bonk | bear | Grit | Sits on you at mile 20. Does not apologise. | Hit the wall (slows), Lumber |
| Blister | beetle | Form | Small. Patient. Lives in your left sock. | Rub raw (drains), Nip |
| Chafe | crab | Form | Walks sideways into every seam you own. | Side scuttle (slows), Pinch |
| **Old Mossback** (boss) | giant tortoise | Grit | Guards the valley's end. Has never once hurried. | Moss slam, Shell snap |

**Stage 2, Switchback Climb** (stamina matters)

| Spirit | Looks like | Affinity | Its line | Moves |
|---|---|---|---|---|
| Lactic | red newt | Grit | Pours hot sand into your quads on every bend. | Burn (slows), Tail flick |
| Cramp | gnat swarm | Pace | Bites the calf you were relying on. | Seize (slows), Swarm (drains) |
| Puff (night) | owl | Grit | Asks if you are sure about this, very loudly. | Wheeze (rattles), Hoot |
| Niggle | shrew | Form | Not an injury yet. Keeps you guessing. | Nag (rattles), Scurry |
| **The Steep** (boss) | great stag | Pace | The hill that keeps going after it should stop. | Antler rush, Long climb (drains) |

**Stage 3, Scree Fields** (technical; footing matters)

| Spirit | Looks like | Affinity | Its line | Moves |
|---|---|---|---|---|
| Scree-hopper | mountain goat | Pace | Makes loose rock look easy, on purpose. | Leap, Rockslide |
| Doubt | mist | Grit | Speaks in your own voice. Knows your splits. | Second-guess (rattles), Fog (slows) |
| Slip | lizard | Form | Waits on the wet slab for your best stride. | Skid, Trip (slows) |
| Ankle-biter | marmot | Form | Knows exactly which stone will roll. | Ankle snap (slows), Burrow |
| **The Shale King** (boss) | stone ram | Form | The mountain, deciding if you may pass. | Shale crash, Landslide (slows) |

**Stage 4, Ridge Line** (exposed; the jokes thin out)

| Spirit | Looks like | Affinity | Its line | Moves |
|---|---|---|---|---|
| Gale | wind spirit | Pace | Leans on you from the side, for hours. | Gust (slows), Headwind |
| Vertigo | raven | Grit | Circles where the ground falls away. | Spiral (rattles), Peck |
| Hypo (night) | snow hare | Form | The cold you stop feeling. That's the danger. | Chill (drains), Bound |
| **The Knife-Edge** (boss) | eagle | Pace | Owns the drop on either side. | Stoop, Updraught (rattles) |

**Stage 5, The Summit** (quiet and serious)

| Spirit | Looks like | Affinity | Its line | Moves |
|---|---|---|---|---|
| Thin Air | ghost goat | Grit | Takes a little of every breath. | Gasp (drains), Fade (slows) |
| Frostnip | snow fox | Pace | Quick, white, and already at your fingers. | Frost bite (slows), Flurry |
| Doubt, again | mist | Grit | The same spirit as on the scree, higher up; a second line is for the dialogue (Q4). | as on the scree |
| **The Summit Wolf** (boss) | wolf | Grit | Has followed you since the valley. | Howl (rattles), Long chase (drains) |

A beaten regular spirit offers to join; up to three run with you. Each lends +1
to its affinity's stat, and its own-affinity move becomes R1's Technique when it
is the best partner for the spirit in front of you. Bosses never join.

Silhouettes to keep distinct at 24×24: the two fox and two goat pairs (Stitch
and Frostnip, Scree-hopper and Thin Air) need different poses and the stages'
colours (green valley, grey scree, white summit), and Doubt is the one creature with no solid
outline (mist).
