# UNA × HybridX explainer: plan

A browser-based 3D explainer of the HybridX family of apps on the UNA Watch, led by HybridX Race and X Streak,, for hybridx.club, promo
material, and Reels and YouTube video. Built with three.js, driven by one GSAP timeline, exported frame
by frame.

**Status:** agreed with Jon on 22 November 2026 (section 7); built (22 November 2026). How to run, edit and export it: [`README.md`](README.md).

The rule: **the screen stays honest; the world around it explains.** Everything on the watch's screen is
drawn from the apps' own code: the same layout, strings, fonts and colours. Everything off the screen is
the film's own explanation, in HybridX yellow, black and white.

---

## 1. What the code says (the inventory)

Read from `hybridx-race/Software/Apps/LVGL-GUI/gui/src/screens`, `hybridx-streak/Software/Apps/LVGL-GUI/gui`,
`hybridx-streak/Software/Libs/Core` and both services, and checked against the simulator captures in
`hybridx-race/docs/screens/` and `hybridx-streak/docs/screens/real/`.

**Buttons.** UNA's bezel prints UP, DOWN, SELECT and BACK. The SDK names them by position:

| Code | Bezel print | Position |
|---|---|---|
| L1 | UP | about 10 o'clock |
| L2 | DOWN | about 8 o'clock |
| R1 | SELECT (START/STOP) | about 2 o'clock |
| R2 | BACK | about 4 o'clock |

A coloured arc on the screen's edge beside a button means it does something: amber for the main action,
white for the second, green for confirm, red for destructive.

### HybridX Race ("HybridX Race" on the watch)

✓ marks what the film shows.

| Screen | What it shows | Buttons |
|---|---|---|
| ✓ **Main menu** (wheel) | Title "HybridX Race". Start race · Format (Full, Half: 1 to 4, Half: 5 to 8) · Pace plan (Off, Last race, Target) · Last race · Settings | L1/L2 move · R1 select (Format and Pace plan cycle in place) · R2 leave the app |
| Settings (wheel) | Roxzone splits (toggle) · Run length (1 km; shorter for a sim) · Split lock (1 to 10 s, default 3) · Vibrate on split (toggle) · Target time | L1/L2 move · R1 change · R2 back |
| ✓ **On your marks** | "On your marks", "Full race", "16 segments", "Roxzone merged", amber tick by R1. No idle timeout (you wait here for the gun) | R1 starts the clock · R2 back |
| ✓ **Race face** | Segment name in its colour (runs cyan, stations lemon, Roxzone orchid), "1 km · 1 of 16", the segment time (big; grey when paused; amber once over a pace target), total time (with a pace plan: the delta beside it, green ahead, amber behind), "Next: SKIERG" or "Last segment", heart rate, a five-zone heart-rate arc | **R2 takes the split** (ignored for 3 s after one) · R1 action menu · L1/L2 status face |
| Status face | Time of day, battery, and with a pace plan the projected finish ("Finish h:mm:ss") | as the race face |
| ✓ **Split toast** | "Split", the segment that just *ended*, its time. 2 s, or any button | any button dismisses |
| (✓) **Action menu** | Resume · Undo last split · Pause ("Resume timer" when paused) · End race · Discard | L1/L2 move · R1 select · R2 back to the race |
| Hold to confirm | Ring fills over 1.5 s, 3-2-1, "Hold to Finish" (green) or "Hold to Discard" (red) | release R1 to cancel |
| ✓ **Finished** | "Race time 1:15:28", "R1 Save    L2 Undo" (Undo only when the race ended with a split) | R1 save · L2 undo the finish and race on · saves itself after 60 s |
| ✓ **Saved** | "Saved", a tick, then **the Streak line** in amber: "3 of 3 this week" / "12-week streak" (and "Load N" in place of the tick when there is a training load). Then the summary. Discarded: "Race has been deleted", then the app closes | none |
| ✓ **Summary** (also Last race) | "Race complete" (or "Ended early"): Total, Runs, Stations, Avg HR, Max HR. Then "Splits 1-5 of 16" pages, five rows each ("2 SKIERG 4:32") | L1/L2 page · R2 to the menu |

**What makes the watch buzz** (`Service::notifySegment`, `finishRace`): a run starts with **one** strong
click, a station with **two**, a Roxzone with one short tick, and the finish with a **1-second buzz**.
Menu presses do not buzz.

**Stations, as the watch writes them** (`RaceData.hpp`): SKIERG 1000 m · SLED PUSH 50 m · SLED PULL 50 m ·
BURPEE BROAD JUMPS 80 m · ROW 1000 m · FARMERS CARRY 200 m · SANDBAG LUNGES 100 m · WALL BALLS 100 reps.
The summary uses short names: BURPEES, CARRY, LUNGES.

### X Streak ("X Streak" on the watch)

| Screen | What it shows | Buttons |
|---|---|---|
| ✓ **Home** | The mountain you are climbing (five shapes), a climber on its trail, "Snowdon · 1 week to go", "11 week streak" (or "Your first week", "Start a new streak"), pips "●●○ this week", a coach line ("1 more · 3 days left"; amber when at risk; "Week banked. Rest up.") | L1, L2 or R1 open the menu · R2 leaves |
| Menu | This week · By sport · Your numbers · Log a session · Trophy case · Training load · Settings | L1/L2 move · R1 select · R2 back |
| This week | Each session counted; R1 asks "Leave this out?" | |
| Log a session | What (Run, Ride, Walk, Strength, Workout, Hybrid, Row, Other), then When (Today, Yesterday) | |
| Trophy case | The summits, the session badges, best week, longest streak, all sessions | |
| Training load · By sport · Your numbers | Fitness, fatigue and form; the week by sport; the family's numbers | |
| Settings | Weekly target (1 to 7, default 3) · Week starts · What counts · Shortest · One per day | |

**The moments** play on Home when Streak opens and finds something new (`HomeScreen.cpp`, `DESIGN.md` §6):

| Moment | On screen | Buzz |
|---|---|---|
| ✓ Session found | "+1 Hybrid · 75 min" replaces the coach line; the new pip pops | sharp tick |
| ✓ Step up (target met) | "Week complete!" in lime, the climber glides one step (1.1 s), the trail behind turns lime, an 8-ray burst | strong double click |
| ✓ Summit | Full screen: "Summit!", the peak, a waving flag, 16 pieces of confetti, "Snowdon · 12 weeks", "Next: Ben Nevis" | two strong pulses |
| ✓ Shield offer | A shield, "Life happens.", "1 of 3 last week. Use a shield to keep your streak of 12 weeks?", "2 shields left"; green tick by R1, cross by R2 | none |
| ✓ Shield spent | "Streak saved." in lime, "Your streak of 12 weeks climbs on. Rest well.", "1 shield left" | soft bump |
| Fresh start | A sunrise, "Fresh start.", "Your climb is safe: 6 of 14 on Ben Nevis. A new streak starts today." | none |

**The rules** (`StreakModel.hpp`, `Summits.hpp`):
- A week counts when its target is met, at once, not at the week's end. The streak and the climb step up then.
- The climb is measured in total weeks achieved, never the current streak. **Missing a week never knocks
  you back down a mountain.** The ladder: Arthur's Seat 4, Snowdon 12, Ben Nevis 26, Mont Blanc 52,
  Everest 104 weeks, then Everest again every 52.
- **Shields:** one for every 4 weeks achieved, at most 2. A missed week with a streak running offers one.
  Decline, or have none, and it is a fresh start.
- Your first week (or the week after a goal change) is a trial: missing it does no harm.
- Session badges: Trailhead 10, Ridge Walker 50, Centurion 100, Mountaineer 250 sessions.

**How a race reaches Streak** (real, `hybridx-link/docs/ACTIVITY_LINK.md`): Race writes the saved race to
`act_race.json`. It works out the Streak line itself, by Streak's rules, and shows it on Saved. Streak
reads the file when it opens and counts the race as a **Hybrid** session. That gives the film a true
bridge between the two apps.

### Where the brief and the code disagree (the code wins)

1. **Launch and "race/workout selection".** The film can't show the watch's own app list, because UNA's
   launcher isn't in our code. Race also has no workout choice: you choose a **Format** (Full, Half 1-4,
   Half 5-8) and a **Pace plan**. The film opens on Race's main menu and shows the Format cycling.
2. **The lap button is R2**, under the bezel's printed "BACK", not SELECT.
3. **Station names:** the watch says ROW, not Rowing.
4. **The split lifting off the screen.** On the watch the split is a 2-second "Split" toast. The floating
   panel is the world's copy of it.
5. **Fastest and slowest** are not marked anywhere in the app. The highlight is the world's explanation.
6. **Streak is weekly, not daily.** Days don't build up. Sessions fill a week's pips, a met week is a step
   up the mountain, milestones are **summits** (weeks) and **session badges**, and the protection is
   **shields**. Scene 4 is rewritten around that.
7. **Colours.** The screen uses the watch's 64-colour palette, so Race is cyan, lemon and teal, and Streak
   is teal and lime. Both use **amber** for button hints, the Streak line and "at risk". The heart-rate
   arc has orange and red. "No orange" can only apply off the screen.
8. **The yellow.** The brand files give HybridX yellow as **`#FADB5C`**
   (`hybridx-link/Tools/Brand/brand.py`, from hybridx-web-x), not `#FFD400`.
9. **Haptics.** Only splits, the finish and Streak's moments buzz on the watch, not every press.
10. **`/assets` doesn't exist.** See section 2.

---

## 2. Assets

| Asset | Where it is | How it's used |
|---|---|---|
| UNA watch, front renders (graphite, teal, white; 2000 px, screen cut out) | the SDK's Figma pack, `una-sdk/Docs/Templates/Figma-UI-Kit/UNA-Watch-UI-Resource-Pack.fig` | **the film's watch**: the graphite render, with the live screen behind its opening (23 November 2026, Jon: use UNA's official mockup, not a model) |
| UNA wordmark (white, 4096 px) | same pack | the lockup |
| HybridX X mark (`x-mark.png`, 593 px) | `hybridx-link/Tools/Brand/assets/` | traced to a vector, extruded for the lockup |
| Space Grotesk, Inter (OFL) | `hybridx-link/Tools/Brand/fonts/`, or `@fontsource` | the overlay type |
| App icons, 1024 px | `hybridx-link/docs/brand/icons/race-1024.png`, `streak-1024.png` | the outro |
| The apps' screen fonts (Poppins, LVGL 2 bpp bitmaps) | `*/Software/Apps/LVGL-GUI/assets/fonts/*.c` | the on-watch text, glyph for glyph |

There is **no GLB** and the pack has the **front view only** (no side or back). The first build modelled
the watch in code to match the render; since 23 November 2026 the film shows UNA's render itself
(section 4). As the existing promo films do, UNA's art is read from the SDK at build time
(`scripts/extract-una.py`, into the git-ignored `public/gen/una/`) and never committed.

Measured from UNA's front render by the extractor (`meta.json`), in screen radii (R): the screen opening
is a circle of 366.6 px at (999.5, 1010.5) in the 2000 px image; the buttons' outer edges are at
L1 299.3° R 1.62, L2 239.5° R 1.64, R1 60.5° R 1.62, R2 120.5° R 1.64 (TouchGFX degrees), within a degree
of the SDK's hint angles.

---

## 3. The film (16:9, about 62 s)

**Jon, 22 November 2026:** keep the example race (1:15:28, "nice and average"); leave HYROX out and say
HybridX; the UNA × HybridX lockup is fine (Jon is working with UNA on this); and **don't dwell on small
features: show the general capabilities, the athlete's benefits and, above all, the family of apps
working together on UNA.**

So the film is about **one week, seven apps, one watch.** Each app is shown by what it does for the
athlete, and the thread through the film is the real link between them:
- every app's saved session counts in X Streak;
- every app's Saved screen shows your week ("3 of 4 this week / 11-week streak");
- every session's heart rate becomes one training load (fitness, fatigue and form);
- X Trail's and X Training's runs feed X Run's VO2max.

All of it happens on the watch itself (`hybridx-link/docs/ACTIVITY_LINK.md`), and every claim comes from
the checked store text in `hybridx-link/docs/STORE_RELEASE.md`. HYROX is never named.

**The family** (the names on the watch): HybridX Race, X Run, X Trail, X Ride, X Workout, X Training and
X Streak. X Weigh-in, X Sauna and the rest stay out.

**The week** (one story, consistent with the apps' rules):
- the athlete's target is 4 sessions a week;
- before Monday: an 11-week streak, 11 weeks climbed (Snowdon, 1 week to go), 2 shields;
- Monday, X Run: "1 of 4 this week / 11-week streak";
- Wednesday, X Ride: "2 of 4";
- Thursday, X Workout: "3 of 4";
- Saturday, HybridX Race, 1:15:28: "4 of 4 this week / 12-week streak";
- X Streak then opens on "+1 Hybrid · 75 min", "Week complete!" and **Summit! Snowdon · 12 weeks**.

Each "Load N" is worked out by the family's own formula (Edwards, `TrainingLoad.hpp`) from the example
sessions' heart rates.

| # | Scene | Time | On the watch (real) | Around it (the world) | Caption |
|---|---|---|---|---|---|
| 0 | **Intro** | 0-5 | dark | Black. A rim light sweeps on and reveals the graphite watch over a fading grid. The UNA × HybridX lockup resolves above it. | (lockup) |
| 1 | **The family** | 5-15 | Each app's real screen in turn, about 1.2 s each: X Run's VO2max, X Trail's map, X Ride riding, X Workout's circuit, X Training's today, HybridX Race racing, X Streak's mountain | The seven app icons lift out of the screen one by one and settle into an orbit round the watch, each with its benefit in a few words. | "One watch. A whole family of training apps." |
| 2 | **One week, in sync** | 15-27 | X Run Saved "1 of 4 this week", X Ride Saved "2 of 4", X Workout Saved "3 of 4", each with its Load | A week strip (MON to SUN) floats beside the watch. Each session leaves its app's icon as a tile, runs along a line and lands on its day. Four pips fill, three of four. A callout: "SHARED ON THE WATCH". | "Every session counts, whichever app." then "Every app knows your week." |
| 3 | **Race day** | 27-43 | HybridX Race: "On your marks" → R1 → the race face, run and station; R2 at each split, the "Split" toast → finish "1:15:28" → Saved "4 of 4 this week / 12-week streak" | A ring of 16 segments round the watch, filling split by split (the middle as a time-lapse). At the finish the ring folds into one tile, "1:15:28", which flies to Saturday. The fourth pip lights. | "Race day. One button, every split." then "Week done the moment you save." |
| 4 | **X Streak** | 43-55 | The watch turns to X Streak: "+1 Hybrid · 75 min", "Week complete!", the climber steps up → "Summit! Snowdon · 12 weeks" → the Training load screen: Form and Fitness | The week's tiles fuse into one week block, which joins a staircase of 12 weeks climbing a wireframe Snowdon. The counter goes 11 → 12, and the five summits mark the ladder. Then lines from all seven icons feed one fitness curve. A callout: "MISS A WEEK? A SHIELD SAVES IT". | "Hit your week. Climb a mountain." then "One training load, from every app." |
| 5 | **Outro** | 55-62 | X Streak's mountain, now Ben Nevis | The watch settles. The seven icons close into a ring, the lockup returns, then the address. | "Seven apps. One watch. All in sync." · HYBRIDX.CLUB |

**On the screen, what's live and what's captured.** Race's and X Streak's screens are redrawn live from
their code: menus, race face, toasts, Saved, the mountain, the moments. In scene 1 the other apps' screens
are their **real simulator previews** (`*/Resources/previews/`, the store's), drawn into the same 240 px
canvas and quantised the same way. The Saved screens in scene 2 are redrawn from each app's own
`ResultScreen` with the week's numbers.

**Press feedback.** A ring pulse at the pressed button every time. A 2-3 px shake only where the watch
really buzzes: a split, the finish, X Streak's moments.

**Other ratios.** Each shot has camera keyframes per ratio in `config.ts`.
- **9:16** (1080 × 1920, the priority) puts the watch in the upper half and stacks the week strip and
  panels below it. All text stays in Instagram's safe area: below y 250, above y 1540, and clear of the
  right-hand button column in the lower half (the numbers in `promo/lib/reel.mjs`).
- **1:1** (1080 × 1080) centres the watch, a little smaller.

---

## 4. Tech

- **Vite + TypeScript (strict) + three.js** (latest at install, version pinned exactly) and **GSAP** for one
  master timeline. It stays paused; every frame is `timeline.seek(t)` then `render()`.
- **Deterministic.** Everything derives from `t`: seeded RNG (mulberry32, seed in config), grain seeded by
  frame number, no `Math.random`, `Date.now` or frame deltas in scene code (a test greps for them).
  `performance.now()` only advances `t` during live playback.
- **Post:** EffectComposer with RenderPass, UnrealBloomPass (half resolution, threshold 1.0), SMAAPass,
  an optional faint grain pass, and OutputPass. The screen is drawn at most 1.0 bright, so it never
  blooms; holograms, callouts and particles are drawn brighter than 1.0, so they do.
- **Lighting:** RoomEnvironment through PMREM for reflections, plus a key and a rim light. Contact shadow
  from the three.js contact-shadow technique (a depth render from below, blurred, at 512 px).
- **Floor:** a shader plane with a fine perspective grid that fades with distance, on `#0A0A0A`.
- **The watch: UNA's own render** (graphite, from the SDK's Figma pack; `src/watch/model.ts`):
  - a card that always faces the camera (the pack has no other view), with a slight idle sway; the
    camera and the world move round it;
  - the live screen sits just behind its transparent opening, on black;
  - lit in its shader: the room's light scales UNA's own shading (full light shows the render as it
    is), an edge light picks out the outline on the rim light's side, and in the dark intro a soft
    band of light sweeps across it;
  - the strap's cut ends fade out, as in `promo/tools/una_mockups.py`;
  - drawn in two passes, depth where it is solid and then colour, so the race ring passes behind it;
  - a plain case shape, never drawn, casts the floor's contact shadow;
  - race to X Streak: the watch dips back and returns, the screen passing through black (it was a
    full turn while the watch was a model);
  - a press shows as a ring pulse from the button's measured edge (the render's buttons cannot move).
- **The screen:** a 240 × 240 **CanvasTexture**, redrawn every frame, NearestFilter, no mipmaps. The
  screen shader quantises it to the display's 64 colours (2 bits per channel: 0, 85, 170, 255), as the MIP
  display shows them: flat colour, no glow.
- **The UI on it:** a TypeScript port of the existing promo's 240 px redraw (`promo/lib/lvgl.mjs`,
  `ui-race.mjs`, `ui-streak.mjs`: the SDK's title rule, button arcs, wheel menu, the heart-rate arc, the
  summit scene's geometry), **brought up to date with today's code**. That adds the Pace plan item, the
  "HybridX Race" title, the Streak line on Saved and Streak's current Home.
  - Text uses the apps' own LVGL bitmap fonts (Poppins at 2 bpp, no kerning), converted from their `.c`
    files by a script, so every glyph is the watch's own pixels.
  - A check sheet renders every screen state beside its simulator capture.
- **Overlay type:** Space Grotesk (static 500 and 700) via troika-three-text, so all text is in the canvas
  and in the export. Labels are tracked uppercase at the brand's 0.12 em, with big numerals. Captions sit
  on an orthographic HUD layer, drawn after post (no bloom).
- **Leader lines:** Line2 (fat lines, pixel widths), anchored to points on the watch.
- **Colours:** off the screen, only `#FADB5C`, `#0A0A0A`, white and greys of them.
- **Graceful on mobile:** device-pixel ratio capped at 1.5, half-resolution bloom, no grain, and a lighter
  shadow when the GPU is weak (quality only; never the timeline).

### Controls

- **Dev overlay** (DOM, so it never appears in an export; hidden with `?clean=1`): play/pause, a scrub bar
  with scene ticks, scene buttons, the time and frame, a ratio switch, an FPS readout.
- **URL:** `?t=12.5`, `?scene=streak`, `?ratio=16x9|9x16|1x1`, `?clean=1`.
- **`npm run export -- --ratio 9x16 --fps 60`:**
  1. builds and serves the page, and opens it in Playwright's Chromium at exactly 1080 × 1920 (or 1920 ×
     1080, or 1080 × 1080);
  2. for each frame, calls `window.__explainer.renderAt(frame / fps)`, which resolves once the frame is
     drawn and every troika text has synced;
  3. reads the canvas's pixels and pipes raw RGBA to ffmpeg (`/usr/bin/ffmpeg`, or `ffmpeg-static`);
  4. ffmpeg encodes H.264 (CRF 16, yuv420p, faststart) to `promo/videos/explainer/`.
  - Silent; a soundtrack is not in this brief.

---

## 5. Files

As built (22 November 2026); the layout is in [`README.md`](README.md), "Where things are". It
differs from the first plan in one way: there is no `src/scenes/` folder. Each scene's
choreography is the master timeline's tracks (`src/timeline.ts`) plus the world modules'
time-driven motion (`src/world/`), and the camera's shots per ratio are in `src/layout.ts`.

---

## 6. How it's built and checked

Scene by scene. After each one:
- Playwright screenshots at its key times in all three ratios, which I check for framing, legibility and
  overlaps before moving on;
- the screen check sheet after any change to a screen.

1. Scaffold: config, timeline, URL params, dev overlay, and the export proven end to end on a test scene.
2. The watch, lighting, floor, shadow and post: **scene 0**. (Since 23 November 2026 the watch is UNA's
   render, not a model.)
3. The screen renderer: fonts, Race's screens, check sheet.
4. **Scenes 1, 2 and 3.**
5. Streak's screens and **scene 4**.
6. **Scene 5** and the lockup.
7. 9:16 and 1:1 polish, the three exports, README.

**What I can't check here:** this container has no GPU, so the export runs on software WebGL (slow, but
the pixels are the same) and I can't measure 60 fps. You'll need to check the frame rate on your laptop
with the overlay's FPS readout.

---

## 7. Decisions (Jon, 22 November 2026)

1. **Assets:** UNA's art from the SDK's Figma pack, extracted at build and not committed. The X mark is
   traced from `x-mark.png`. The UNA × HybridX lockup is fine (working with UNA).
2. **Yellow:** `#FADB5C`, from the brand files.
3. **"No orange"** applies off the screen only; the screen keeps its real colours.
4. **Story:** the family and its sync, one week across four apps, then X Streak (section 3).
5. **The race:** the films' example race, 1:15:28.
6. **HYROX:** never named; HybridX throughout.
7. **Shake:** only where the watch buzzes; a ring pulse on every press.
8. **Small features** (undo, split lock, buzz counts, shields in detail, the pace plan) stay out, or
   are at most a callout.
9. **Outro icons:** the real store icons (Race yellow, Streak navy and teal, and the rest in their
   colourways).
10. **Launch:** no UNA app list; the film shows each app's own screens.
11. **Where it lives:** `promo/explainer/`.
12. **The watch** (23 November 2026): UNA's official Figma mockup, not a model of it. Its front view is
    the only one, so the watch faces the camera throughout and the turn to X Streak became a dip.
