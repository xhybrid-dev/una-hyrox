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

- British English. Short lines. Plain and true to running, never cruel. Jon
  (20 October 2026): no gags that mean nothing ("your calves have notes");
  every line is real running sense, and where it can be it is advice that works
  in the game too: low on energy, take a Gel before the wall; cramp (Slow), a
  Salt tab; drained, Water; the scree, short steps and poles; wind, a low stride;
  doubt, steady breathing (Settle).
- **Light start, serious summit.** Stages 1-2 are plain and brisk (stitches,
  bonking, cramp). By stage 4 the tone is exposed and watchful; stage 5 is quiet.
- Creatures are named for what they look like: a colour or a word, and the
  animal (Red Fox, Brown Bear, Old Tortoise). The pain they are is in their
  lines and their moves: the fox gives you a stitch, the bear is the bonk at
  mile 20 (Gate Q2: the pain names did not land).
- Never name or echo another franchise (brief section 9).

## The screens (Q2, the story added in Q3)

Eight screens, one plan (`Libs/Core/Header/Layout.hpp`, checked by host tests
against the round face and the bezel arcs). Captures: `docs/screens/q3-*.png`.

- **The buttons are always in the same place.** A word by each arc says what
  it does now, in the arc's colour: amber L1 (choose), white L2 (back, or the
  pack), green R1 (confirm, go), red R2 (fuel in a duel). R2 held 1.5 s leaves
  from anywhere, and a red arc grows on its bezel while it is held. Every word
  sits inside its arc.
- **Text**: one font (Press Start 2P, 16 px). A headline at the top (the clock,
  a page title, the spirit's stamina), a card through the middle (a title of 12
  characters, then four lines of 13), the runner's stamina at the foot.
- **Title**: TRAIL QUEST, the runner with the Red Fox and the Brown Bear; R1 plays (or starts a
  run), L1 held starts again, L2 turns the beat on or off.
- **Map**: the stage from above, left to right, on its own ground colour (night
  is navy). Paths in tan; places as icons: a flag (the start), a purple mark
  with "!" (a spirit), grey rocks (an obstacle), a red-roofed hut, a box (a
  cache, grey once taken), the boss's face (grey once beaten). The way you are
  choosing is amber, with its name under the map and, under that, what lies
  that way: "A spirit", "A hazard", "A hut", "Supplies" ("Nothing" once
  taken), "Quiet", "The boss". The runner is a blinking ring with a red cap
  that walks the line.
- **Duel**: the Q0 scene, its sky by the weather; the runner left, the spirit
  right (a boss bigger). The spirit's stamina at the top, the runner's at the
  foot, with a three-letter tag for an effect (DRN, SLW, RTL). Choosing: the
  four words, and what R1 plays in green under them. Then the beat (any button
  strikes), the verdict (PERFECT green, GOOD gold, MISS red), and the turn,
  event by event, in amber when it goes your way and red when it does not;
  the one hit blinks.
- **Cards** say what happened: a place reached, a find, the hut, a duel won or
  lost, an obstacle cleared or not. R1 (OK) moves on.
- **Story** (Q3, `data/Story.hpp`): a headline in gold capitals, the words on
  the card (five lines of 12, no title), and under it a strip of the stage's
  sky and ground with the runner and whoever the page is about. R1 the next
  page (OK on the last), L1 skips the rest. It plays at the key moments only:
  a new game (the opening tale, six pages, then the stage's two), a stage
  starting (the place, then its boss), the boss ahead (once a game, not on a
  retry) and the boss beaten. The map then says the day's weather.

## The roster (Q1; renamed at Gate Q2)

Twenty-two Trail Spirits: four or so a stage and a boss at the end of each. The
names, looks and lines are drafts; the numbers do not depend on them, so any can
change. The source is `Software/Libs/Core/Header/data/Species.hpp`; how hard each
one is, `BALANCE.md`. Affinity is its place on the wheel (Pace beats Form beats
Grit beats Pace). "Night" spirits appear only at dawn, dusk or night.

**Stage 1, Valley Path** (light; teaches the wheel)

| Spirit | Looks like | Affinity | Its line | Moves |
|---|---|---|---|---|
| Red Fox | fox | Pace | A side stitch: sharp pain under the ribs. | Side stitch (drains), Pounce |
| Brown Bear | bear | Grit | The bonk. Fuel runs out and the legs go. | Hit the wall (slows), Lumber |
| Ruby Beetle | beetle | Form | A hot spot in your sock, rubbing raw. | Rub raw (drains), Nip |
| Sand Crab | crab | Form | Grit in your shoe, chafing every step. | Side scuttle (slows), Pinch |
| **Old Tortoise** (boss) | giant tortoise | Grit | Late fatigue. Slow, steady, heavy. | Moss slam, Shell snap |

**Stage 2, Switchback Climb** (stamina matters)

| Spirit | Looks like | Affinity | Its line | Moves |
|---|---|---|---|---|
| Fire Newt | red newt | Grit | Burning quads from the climb. Walk the steeps. | Burn (slows), Tail flick |
| Gnat Swarm | gnat swarm | Pace | Calf cramp. Salt and a shorter stride. | Seize (slows), Swarm (drains) |
| Barn Owl (night) | owl | Grit | Doubt in the dark. Slow your breathing. | Wheeze (rattles), Hoot |
| Pygmy Shrew | shrew | Form | A niggle: small aches that wreck your focus. | Nag (rattles), Scurry |
| **The Great Stag** (boss) | great stag | Pace | The long climb, taken too fast. | Antler rush, Long climb (drains) |

**Stage 3, Scree Fields** (technical; footing matters)

| Spirit | Looks like | Affinity | Its line | Moves |
|---|---|---|---|---|
| Rock Goat | mountain goat | Pace | Loose rock punishes a long stride. | Leap, Rockslide |
| Mist Wisp | mist | Grit | Self-doubt in the mist. Stay on pace. | Second-guess (rattles), Fog (slows) |
| Wall Lizard | lizard | Form | Wet rock. One bad step and you slip. | Skid, Trip (slows) |
| Marmot | marmot | Form | Rolling stones. Watch your footing. | Ankle snap (slows), Burrow |
| **The Ram King** (boss) | stone ram | Form | Loose scree. Short steps. | Shale crash, Landslide (slows) |

**Stage 4, Ridge Line** (exposed; the jokes thin out)

| Spirit | Looks like | Affinity | Its line | Moves |
|---|---|---|---|---|
| Wind Sprite | wind spirit | Pace | A headwind that costs you speed for hours. | Gust (slows), Headwind |
| Raven | raven | Grit | Doubt on the edge. Watch the path, not the drop. | Spiral (rattles), Peck |
| Snow Hare (night) | snow hare | Form | Cold numbs you. Keep moving and keep eating. | Chill (drains), Bound |
| **The Golden Eagle** (boss) | eagle | Pace | The exposed edge. Stay steady. | Stoop, Updraught (rattles) |

**Stage 5, The Summit** (quiet and serious)

| Spirit | Looks like | Affinity | Its line | Moves |
|---|---|---|---|---|
| Ghost Goat | ghost goat | Grit | Thin air. Every breath gives less. | Gasp (drains), Fade (slows) |
| Snow Fox | snow fox | Pace | Cold hands. Keep them moving. | Frost bite (slows), Flurry |
| Mist Wisp, again | mist | Grit | The same spirit as on the scree, higher up; a second line is for the dialogue (Q4). | as on the scree |
| **The Summit Wolf** (boss) | wolf | Grit | Kept pace since the valley. | Howl (rattles), Long chase (drains) |

## Skill: choosing from the pack (Q4)

R1 "Skill" opens a list when the pack is not empty: one card per choice, in green,
with the creature (or "On your own" for Trail craft), the move, its type and the
damage it would do now, what else it does, and "n of N". L1 next, R1 use, L2 or
R2 back; it opens on the best pick. The pack's creatures are the choices, so what
you keep (and its type against the spirit's place on the wheel) is the strategy.

## The summit finale (Q4)

After the Summit Wolf's page and the "WELL RUN" card, the finale: a gold "THE
SUMMIT" headline, a card of the lap done, the runner's level, duels won and
lost, and the pack, with "Again" (green, R1: lap 2 from the valley) and "Back"
(white, L2: the title; the save stays here, so Play returns to the finale). It
reuses the clear screen's runner with arms up and the Win vibro.

## Boss twists (Q4)

Each boss fights its own way, with a warning first. On a turn the boss *tells*
(a card before the choice: its headline in red, what is coming, what to do) the
turn is special, and the playback says so ("Charge! Double hit", "Braced! Half
the hit", "Guarded! Half damage"):

| Boss | Tells | The turn | Answer |
|---|---|---|---|
| Old Tortoise | every 4th turn | **Guard**: your hits do half | Settle or Fuel; keep the Surge |
| The Great Stag | every 3rd turn | **Charge**: its hit is double | Settle braces: it hits half |
| The Ram King | every 3rd turn | **Guard** | as above |
| The Golden Eagle | every 3rd turn | **Charge** (a dive) | as above |
| The Summit Wolf | every 3rd turn | **Charge**, then **Guard**, and so on | read which it is |

A player who ignores the tells still wins (menu-only play wins about 70 % against
the Great Stag); one who reads them wins more, and the fight is shorter.

## The needle (Q4)

The timing needle (the beat) sharpens as you climb: stage 1 is the original
(a 1.6 s sweep, a 400 ms good zone, a 200 ms perfect zone), each later stage
sweeps 100 ms faster with a good zone 40 ms narrower, to 1.2 s and 240 ms at the
summit; each lap round the Ridge a little more, to 1.0 s and 200 ms. The perfect
zone stays 200 ms: that is two frames at the watch's 10 frames a second, and
anything narrower cannot be told from luck. The later stages' spirits give a
little in return (`kSpiritPct`), so the game stays winnable (`BALANCE.md`).

Each strike also has its own shape (21 October): the target sits somewhere new
(30-85 % of the sweep) so there is no rhythm to learn; Surge is faster and tighter,
Settle slower and wider, a partner's move the stage's own; a boss's fight is a little
tighter; hazards by their stat (a leap fast, a scramble tight, footing slow and wide).

A beaten regular spirit offers to join; up to three run with you. Each lends +1
to its affinity's stat, and its own-affinity move becomes R1's Technique when it
is the best partner for the spirit in front of you. Bosses never join.

Silhouettes to keep distinct at 24×24: the two fox and two goat pairs (the Red
Fox and the Snow Fox, the Rock Goat and the Ghost Goat) need different poses and the stages'
colours (green valley, grey scree, white summit), and the Mist Wisp is the one creature with no solid
outline (mist).
