# Phase 0 experiments

Throwaway code kept because it is evidence, and because the container it ran in
is ephemeral. None of it is part of the app.

## `batched_laps.cpp` + `batched_laps_decode.py`

Answers the question brief §10.1 raises: **can we write every Lap message in one
batch at save time**, after all the Records and before the Session, rather than
streaming each lap as it happens?

We need this because a split can be undone, so a lap is not final until the race
is saved. The SDK's own `ActivityWriter` streams laps immediately, so this
ordering is exercised nowhere in the SDK.

The program builds a realistic file with the SDK's own `FitWriter`: a Full race
(16 segments, Roxzone off), 1 Hz heart-rate records, the three Lap developer
fields (`segment_type`, `round`, `station_id`) and three Session ones
(`race_format`, `roxzone_mode`, `completed`). The Python script decodes it with
the independent `fitdecode` library and checks ordering, lap indices, durations
and developer-field round-trip.

Build and run (needs `UNA_SDK` set and `pip install fitdecode`):

```bash
g++ -std=c++17 -O1 -o lapbatch batched_laps.cpp \
  $UNA_SDK/Libs/Source/Fit/FitWriter.cpp \
  $UNA_SDK/Libs/Source/Fit/FitCrc.cpp \
  $UNA_SDK/Tests/Host/support/KernelTestDoubles.cpp \
  $UNA_SDK/Libs/Source/UnaLogger/Logger.cpp \
  -I$UNA_SDK/Libs/Header -I$UNA_SDK/Tests/Host
./lapbatch && python3 batched_laps_decode.py race.fit
```

Result on 2026-09-21: 4144 records, 16 laps, ordering correct, all laps match,
session reads `sport=training sub_sport=generic`. See `NOTES.md` 0.8.

This becomes `FitOutputTest` in Phase 3. It does **not** prove Strava or Garmin
Connect accept the file — see `ON_WATCH_TESTS.md` T11.

## `syscall_stubs.c`

Link-time workaround that let the *unsupported* distro `arm-none-eabi-gcc` build
RunLVGL in this container. Injected with
`-DCMAKE_EXE_LINKER_FLAGS=<path to stubs.o>` at configure time so no SDK file is
touched. See `NOTES.md` 0.4.

**Not a solution.** The supported toolchain is ST's, from STM32CubeIDE or
STM32CubeCLT, and any build destined for a watch must use it
(`ON_WATCH_TESTS.md` W1).

---

# Phase 4

## `capture_screens.sh`

Not throwaway: this is how `docs/screens/phase4-*.png` were made, and how they
should be remade whenever a screen changes.

The SDK's simulator has no screenshot capability of its own — `LV_USE_SNAPSHOT`
is 0 in its LVGL config — so the rig drives it from outside: `Xvfb` for a
display, `xdotool` to press the four buttons (keys 1-4 are L1, L2, R1, R2),
ImageMagick's `import` to grab the frame.

```bash
UNA_SDK=/path/to/una-sdk ./capture_screens.sh          # into docs/screens
UNA_SDK=/path/to/una-sdk ./capture_screens.sh /tmp/x   # somewhere else
```

It deletes `Software/Output` first so the run starts with no saved race, which
is what makes `phase4-03-main-lastrace.png` show the greyed row.

Two things it will not do for you:

- **`phase4-21-finished-early.png`** comes from a separate short run — start a
  race, open the action menu, hold R1 on `End race` — because the finished
  screen differs there: brief §7.3 refuses `UNDO_FINISH` after an early end, so
  only `R1 Save` is offered.
- **Run one at a time.** Two runs on the same display kill each other's `Xvfb`,
  and the orphaned simulator carries on writing blank 166-byte frames while its
  log fills with `Queue is full`. The script now aborts when it cannot find a
  window and warns on any capture under 500 bytes, but it cannot stop you
  starting the second run.
