# UNA × HybridX: the explainer

A cinematic, browser-based 3D explainer of the HybridX family of apps on the UNA Watch:
seven apps, one watch, all in sync. For hybridx.club, UNA × HybridX promo material, and video
for Instagram Reels and YouTube. About 62 seconds; 16:9, 9:16 and 1:1.

**The screen stays honest; the world around it explains.** Everything on the watch's display is
drawn from the apps' own layout code and fonts. Everything around it is the film's explanation, in
HybridX yellow (`#FADB5C`), black and white. Decisions and the storyboard: [`PLAN.md`](PLAN.md).

## The film

| Scene | Time | What it shows |
|---|---|---|
| Intro | 0-5 s | A rim light reveals the watch; the UNA × HybridX lockup. |
| The family | 5-15 s | Each app's real screen in turn; its icon lifts out into an orbit with what it does for you. |
| One week, in sync | 15-27 s | X Run, X Ride and X Workout save; each Saved screen counts the week up ("3 of 4 this week"); each session flies to its day. |
| Race day | 27-43 s | HybridX Race: one button, sixteen splits round a ring; Save, and the week is done ("4 of 4 / 12-week streak"). |
| X Streak | 43-55 s | The watch dips from Race to X Streak; "+1 Hybrid · 75 min", "Week complete!", Summit! Snowdon at 12 weeks; then one training load from every app. |
| Outro | 55-62 s | The family, the lockup, HYBRIDX.CLUB. |

## Running it

You need Node 20+, Python 3 with `numpy` and `Pillow`, and the SDK checked out at `../../una-sdk`
(or `UNA_SDK`): UNA's watch art is read from its Figma pack at build time and never committed.

```bash
npm install
```

```bash
npm run assets
```

```bash
npm run dev
```

Then open the address Vite prints. `npm run assets` writes everything the page loads from elsewhere
in the repository into the git-ignored `public/gen/`: UNA's watch render (and where its screen
opening and buttons are) and wordmark; the apps' LVGL fonts as JSON; the traced HybridX X; the family's icons and real screens.
Run it again after a change to any of those.

**URL parameters:** `?t=12.5` (start time), `?scene=week` (`intro`, `family`, `week`, `race`,
`streak`, `outro`), `?ratio=16x9|9x16|1x1`, `?clean=1` (no dev overlay), `?play=0` (paused).

**The dev overlay:** play and pause (or Space), a scrub bar with the scenes marked, a button per
scene, the time and frame, the ratio, and the frame rate. Arrow keys step a frame (Shift: a second).

**For hybridx.club:** `npm run build` writes a static site to `dist/` (about 2.5 MB; relative paths, so
it can sit in any folder). It leaves out the files that are only for the checks.

## Editing

Everything a person might change is in [`src/config.ts`](src/config.ts): the captions (and their
line breaks per ratio), the callouts, the colours, every timing, the family's order and benefits,
the example race, and the week's story. `npm test` checks the captions (8 words at most, no em
dashes, HYROX never named) and that the story's numbers agree with each other and with the apps'
rules. Where things sit per ratio (camera shots, the panel, the orbit, caption and lockup
placement) is [`src/layout.ts`](src/layout.ts).

## Exporting

```bash
npm run export -- --ratio 9x16 --fps 60
```

`--ratio all` exports the three in turn. Playwright opens the page at the delivery size (1080 × 1920,
1920 × 1080 or 1080 × 1080), draws each frame from its time, reads it back, and pipes it to ffmpeg
(H.264, CRF 17, yuv420p, faststart). The films land in `../videos/explainer/`. `--from`, `--to` and
`--out` export a part. ffmpeg comes from `$FFMPEG` or the PATH.

On a laptop with a GPU a frame takes a fraction of a second. In a cloud container without one,
WebGL runs on SwiftShader at about 0.9 s a frame: about an hour for a 60 fps film.

## Checks

| Command | What it proves |
|---|---|
| `npm test` | Caption rules, scene order, the race's 16 splits and buzzes, the loads (Edwards, as `hybridx-link`'s `TrainingLoad.hpp`), the week and the summit agreeing with Streak's rules, no wall-clock or unseeded randomness in the code. |
| `npm run screen-check` | Each screen the renderer draws, quantised, beside the simulator's capture of the same screen, with their difference: 92-99% agreement where the data match (`out/screen-check.png`). |
| `npm run check:determinism` | The same time gives the same frame, byte for byte, whatever was drawn before it. |
| `npm run shots -- --ratio all --scene race` | Stills at a scene's key times (`out/shots/`), for framing and legibility. |
| `npm run typecheck` | TypeScript, strict. |

## What is real, and what is illustrative

- **Real:**
  - every screen layout, string, font and colour on the watch (from each app's `gui/src/screens` and
    the SDK's LVGL widgets);
  - the button angles and hint arcs;
  - when the watch buzzes;
  - the race's stations and order;
  - the Streak line's wording;
  - Streak's summits, rules and moments;
  - the family link itself (`hybridx-link/docs/ACTIVITY_LINK.md`);
  - every benefit, from the checked store text in `hybridx-link/docs/STORE_RELEASE.md`.
- **Real screens, captured:** in the family scene, X Run's, X Trail's, X Ride's, X Workout's and
  X Training's screens are their real simulator previews (the store's), drawn into the same 240 px
  canvas.
- **Illustrative:**
  - the example race (1:15:28, the promo films' race);
  - the week's sessions and their heart-rate zones, with each load worked out from them by the
    family's formula;
  - the Training load screen's numbers (form −9, Building, which agrees with Streak's bands);
  - the chart's curves.
- **The watch** is UNA's own render, the graphite watch from the SDK's Figma pack, with the live
  screen behind its opening. It and UNA's wordmark are UNA's art, used with UNA's permission (Jon,
  22 November 2026). The pack has the front view only, so the watch always faces the camera.

## Where things are

```
src/config.ts          copy, timings, colours, the race and the week
src/layout.ts          per-ratio cameras and placement
src/timeline.ts        the GSAP master timeline (seek(t) every frame) and easing helpers
src/app.ts             the renderer, the stage, renderAt(t)
src/screen/            the watch's UI: LVGL fonts and widgets, Race, Streak, the family, the script
src/watch/             UNA's render as the watch, the 240 px screen and its quantise shader, presses and haptics
src/world/             the family's orbit, the week, the race ring, Streak's mountain and load
src/hud/               captions, callouts, the lockup (troika-three-text, Space Grotesk)
src/render/            post (bloom, SMAA, grain), stage (grid, contact shadow, light)
scripts/               assets, export, shots, screen check, determinism
tests/                 the config and story checks
```

Fonts: Space Grotesk (SIL OFL, via `@fontsource`) for the overlay; the apps' own Poppins builds on
the watch.
