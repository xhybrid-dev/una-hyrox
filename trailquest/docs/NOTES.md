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

## Findings from the SDK (17 October 2026)

SDK commit `a7a995a` (the one CI pins). Paths are in `una-sdk/`.

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
3. **The buzzer has no pitch.** `RequestBuzzerPlay::Note` is `{time ms, volume}`,
   10 notes at most, volumes 0, 33, 66, 100 only
   (`Libs/Header/SDK/Messages/CommandMessages.hpp:378`). The vibro takes up to 8
   notes of 21 named effects or pauses (`:403`). So no chiptune; the sound is
   rhythm, and the vibro carries the meaning.
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
   170 px; at y = 225 about 130 px.
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
   both timestamps from the GUI's clock (`lv_tick_get()`), and the core takes
   them as inputs. Both are the wrapping millisecond count, so every difference
   is an unsigned delta (tested across the wrap).
10. **Memory.** `GUI_RAM_LENGTH` is 900K and the stack 24 KB in Weigh-in; we
    copy those. Measured for the Q0 build (watch target, compile check): GUI ELF
    173,344 B text + 1,484 B data + 159,724 B bss = **334,552 B** (RunLVGL is
    about 450-490 KB), of which the sprite buffers are 20,736 B and the LVGL
    pool 40 KB; service ELF 16,212 B. The `.uapp` is 185,228 B. In the
    simulator the probe screen uses 9 KB of the pool, peak 33 %.

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
  arm-none-eabi-gcc -c hybridx-race/docs/experiments/syscall_stubs.c -o stubs.o \
      -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb
  cmake -G "Unix Makefiles" -S trailquest/Software/Apps/TrailQuest-CMake \
      -B build-arm -DCMAKE_EXE_LINKER_FLAGS=$PWD/stubs.o
  make -C build-arm
  ```
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

- Host tests: 18 pass (12 for the beat: zone edges, the sweep's wrap, the
  kernel clock's wrap, the two-frame floor; 6 for the feedback table: kernel
  limits, known effect numbers, supported volumes, patterns distinct from each
  other).
- Simulator: builds with no warnings from our code; a captured run shows the
  press reaching the service ("beat 1: perfect, 15 ms") and the answer one frame
  (100 ms) later. Screens in `docs/screens/q0-*.png` (cut to the round face).
- Watch target: builds and links (compile check with stubs, not installable);
  `TrailQuest_0.0.0-dev.uapp`, 185,228 bytes. CI builds the real one.
- Art: `gen_art.py` passes its palette check; sheet in
  `docs/screens/q0-art-sheet.png`.

**Not verified (cannot be, here).** Anything on a real watch: how it looks,
the real frame time, the haptic and buzzer feel, the backlight hold, the button
order. The simulator's render times (1-3 ms on a PC) say nothing about the
watch's roughly 40 ms.

**Gate Q0: what Jon must test.** Install the `.uapp` from the CI run's
`watch-apps` artifact on branch `claude/trailquest-q0`, the way the other
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
6. Does the screen stay on for about 30 s after the last press?
7. Does R2 leave the app cleanly?
8. Optional: leave it running 10 minutes and note the battery drop.

**Decisions for Jon at the gate.**

- Keep the timing beat (and its zones), or go menu-only / a pause-and-pick meter?
- Is the look right (palette, font, sprite style), or should it change before
  more art is made?
- The name.

## Open items

- `Libs/Core/Sources/` is empty until a module needs more than a header.
- The scene is a few flat shapes. A tile map (the route screens) is not tested
  yet: the LVGL pool is small, so it should be one custom draw callback with
  runs of rectangles, not a widget per tile. Q2 decides this with a measurement.
- The app has no configuration fields, so CI's manifest check is not wired for it.
