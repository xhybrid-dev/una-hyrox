# HybridX promo films

Three 150-second motion-graphics films, one per app, made to be seen together:
the same brand, the same grammar, a different character each.

![Race, Streak and Trail](videos/posters/hybridx-films.jpg)

| Film | File | Character |
|---|---|---|
| **HybridX Race**, "Sixteen" | [`videos/hybridx-race.mp4`](videos/hybridx-race.mp4) | 128 BPM, F minor. The film is itself a race: sixteen chapters, cyan for runs, lemon for stations, a split on every cut. |
| **HybridX Streak**, "The climb" | [`videos/hybridx-streak.mp4`](videos/hybridx-streak.mp4) | 112 BPM, D major. Night sky, teal and lime. Everything moves upwards, and the weeks-climbed counter only ever goes up. |
| **HybridX Trail**, "Follow the line" | [`videos/hybridx-trail.mp4`](videos/hybridx-trail.mp4) | 96 BPM, E minor. A topographic map at night, one orchid line, and the one sharp moment: going off course. |

All three are 1920 × 1080 at 60 fps, H.264 with 48 kHz AAC, mastered to
−14 LUFS. Stills from each are in [`videos/posters/`](videos/posters/).

## The reels

Three portrait cuts of the films for Instagram Reels and adverts, 30 to 40
seconds each, in [`videos/reels/`](videos/reels/) with a cover still for
each in [`videos/reels/covers/`](videos/reels/covers/).

| Reel | File | Length | Cut |
|---|---|---|---|
| **Race** | [`hybridx-race-reel.mp4`](videos/reels/hybridx-race-reel.mp4) | 33.8 s, 18 bars | "8 runs. 8 stations. 1 button.", the sting on the gun, then one button, Roxzone, heart rate, undo and laps, two bars each, the finish and the end card. The top bar is the race's sixteen segments filling. |
| **Streak** | [`hybridx-streak-reel.mp4`](videos/reels/hybridx-streak-reel.mp4) | 34.3 s, 16 bars | "Every workout counts." with sessions landing, the sting, week complete, a shield spent, then the camera climbs all five mountains, 104 weeks, to Everest's summit. |
| **Trail** | [`hybridx-trail-reel.mp4`](videos/reels/hybridx-trail-reel.mp4) | 35.0 s, 14 bars | "Just the line." as the route draws itself, the sting, load a route, follow it, then going off course and back, the loop completed, "Coming soon". |

All three are 1080 × 1920 at 60 fps with their own shortened scores
(`audio/*_reel.py`), at −14 LUFS, and start moving and sounding on the first
frame. Everything that must be read sits inside Instagram's safe area
(`lib/reel.mjs`): below the header (y 250), above the caption (y 1540), and
clear of the button column on the right of the lower half. The same
"read before sharing" notes apply to them, and for paid adverts in
particular UNA's watch and logo need UNA's clearance first.

## Thumbnails

Six YouTube thumbnails in [`videos/thumbnails/`](videos/thumbnails/), 1280 × 720,
two per film. **A** is the one to upload; **B** tries a different hook, for
YouTube's "Test & compare" once the videos are public.

| Film | A | B |
|---|---|---|
| **Race** | "16 splits. One button." The last segment, R2 lit. | "8 runs. 8 stations. 1 button." The first run. |
| **Streak** | "Every week counts." Everest's summit. | "Life happens. Keep climbing." The shield's offer. |
| **Trail** | "Follow the line." The watch calls "Off course". | "Wander off? You’ll feel it." The buzz. |

All six share the films' kit and one layout, so they read as a series: the
lockup top left, the headline on the left, UNA's watch on the right, and
nothing that matters bottom right, where YouTube shows the duration. The
headlines stay readable at sidebar size (168 × 94). Like the films, they show
UNA's watch and logo, so check with UNA before they go public.

## What they share

- **Jon's X mark**, rebuilt as vector geometry (`lib/brand.mjs`) and fitted to
  `hybridx-race/Resources/icon_60x60.png` (97% overlap at icon size): a ">"
  chevron with a flat tip, and two arms that stop short of it. **The gap is
  where each app's colour lives**: the title sting sparks it, the end card
  lets it breathe.
- **Only the watch's colours.** Every accent is from the SDK's 64-colour
  palette (`una-sdk/Libs/Header/SDK/GUI/Color.hpp`) as the display shows it:
  Race's CYAN and LEMON, Streak's TEAL and LIME, Trail's ORCHID.
- **Type:** Poppins, the apps' own face, for everything big; JetBrains Mono for
  small technical labels.
- **The grammar:** the same title sting, the same HUD in the corners, the same
  end card with the family of three and the one being shown lit, and the same
  sonic logo (two notes a fifth apart as the arms lock, then the octave). Each
  film has its own transition built from the brand: Race wipes with the
  chevron pointing forward, Streak with it pointing up, Trail with a drawn line.
- **The real screens.** Race and Streak's watch screens are redrawn from the
  apps' layout code (label positions, LVGL font metrics, the SDK's button
  arcs, title rule and wheel menu, the Summit scene's mountain geometry), and
  checked against the committed simulator captures.

## Read before sharing

- **Race times are an example race** (a 1:15:28 finish), invented for the film.
  They are not pacing data and nothing in the app uses them.
- **Trail is at its T0 probe.** Its screens here are concept designs for the
  brief's v1 features (route list, map, zoom, heading-up, the off-course
  banner, distance along the line); the real screens are phase T3. Its end
  card says "Coming soon". The GPX numbers (5,001 points, 1,001 kept 10 m
  apart, 10.05 km, 80 m ascent) are the probe's simulator run on the test
  loop from `Tools/TestRoutes/make_test_gpx.py`.
- **The watch is UNA's own.** The watch shown is UNA's render from the SDK's
  Figma UI Resource Pack (`una-sdk/Docs/Templates/Figma-UI-Kit`): graphite
  for Race, teal for Streak, white for Trail. It carries the UNA logo, so the
  renders are read from the SDK when a film is rendered (`tools/una_mockups.py`
  caches them in `out/una/`) and are never committed here. The SDK's
  `TRADEMARK.md` allows saying an app is "for UNA Watch" but grants no right
  to UNA's logos, so **check with UNA before publishing** films that show
  their product and logo. Without the SDK checked out, `lib/watch.mjs` falls
  back to a generic round watch.
- **Trademarks.** UNA is named only as "for UNA Watch". HYROX is not named.
  Strava, Garmin Connect, OS Maps and Komoot are named in plain text, as the
  apps' docs do.
- **The address** on the end card, `hybridx.club`, comes from the wordmark
  lockup in Jon's brand files. Change it in `lib/brand.mjs` if it's wrong.

## Rendering

Frames are pure functions of time, drawn with Skia (`@napi-rs/canvas`) and
piped to ffmpeg; the soundtracks are synthesised from oscillators and noise
with numpy and scipy, and placed from each film's own cue sheet, so every hit
lands on its frame.

You need Node 20+, Python 3 with `numpy`, `scipy` and `Pillow`, an ffmpeg with
libx264 (`pip install imageio-ffmpeg` provides one; or set `FFMPEG`), and the
SDK checked out at `../una-sdk` (or `UNA_SDK`) for the watch renders.

```bash
cd promo
npm install
python3 audio/race.py            # writes audio/out/race.wav
node render.mjs race             # renders in parallel, muxes, writes videos/hybridx-race.mp4
```

Quicker looks while working:

```bash
node render.mjs race --sheet 2.5          # a contact sheet, one frame per 2.5 s
node render.mjs race --still 31,135       # full-size frames
node render.mjs race --from 20 --to 40 --preview   # a half-size clip
node render.mjs race --cues               # the cue sheet the score is built from
python3 audio/spectrum.py audio/out/race.wav        # the mix's octave balance
```

A full film renders in five to six minutes on four cores (H.264 at CRF 18).

The reels work the same way, with their own ids:

```bash
python3 audio/race_reel.py       # writes audio/out/race-reel.wav
node render.mjs race-reel        # writes videos/reels/hybridx-race-reel.mp4
node posters.mjs                 # film posters and the reels' covers
node thumbnails.mjs              # the YouTube thumbnails, and out/thumbnails_review.png
```

## Where things are

```
render.mjs            the renderer: stills, sheets, parallel video, muxing
lib/core.mjs          frame constants, easing, springs, seeded noise, tempo
lib/gfx.mjs           type, kinetic text, layers, glow, shapes
lib/brand.mjs         palette, the X mark, sting, HUD, wipes, end card
lib/watch.mjs         UNA's watch (or a generic one), presses, haptic shake
tools/una_mockups.py  extracts UNA's watch renders from the SDK's Figma pack
lib/lvgl.mjs          240 x 240 screen primitives matching the SDK's LVGL helpers
lib/ui-race.mjs       Race's screens
lib/ui-streak.mjs     Streak's screens and the Summit scene
lib/ui-trail.mjs      Trail's concept screens
lib/terrain.mjs       Trail's height field, contours and route
lib/film.mjs          headline, callout and press helpers
films/*.mjs           the three films: scenes, timing, cue sheets
films/*-reel.mjs      the three portrait reels
lib/reel.mjs          the reels' safe area, top bar and closing card
thumbnails.mjs        the YouTube thumbnails, two per film
audio/synth.py        the synthesiser, effects and mix bus
audio/common.py       the sonic logo, split sound, loudness normalisation
audio/*.py            the three scores, and the reels' (*_reel.py)
videos/               the finished films and their posters
```

## Licences

Poppins and JetBrains Mono are under the SIL Open Font Licence and come from
the `@expo-google-fonts` npm packages. The soundtracks contain no samples.
