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
- **The buzzer is rhythm only** (no pitch): short and loud for good things, long
  and quiet for bad ones. Never more than a few notes.

## Voice

- British English. Short lines. Dry, a little cheeky, never cruel.
- **Funny start, serious summit.** Stages 1-2 joke about what runners put up
  with (stitches, bonking, doubt). By stage 4 the jokes thin out; stage 5 is
  quiet, and the humour, when it comes, is gentle.
- Creatures are named for the pain they are: Stitch, Bonk, Cramp, Doubt, Gale.
- Never name or echo another franchise (brief section 9).
