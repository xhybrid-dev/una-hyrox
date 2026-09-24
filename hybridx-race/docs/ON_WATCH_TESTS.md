# On-watch test checklist — HybridX Race

Jon has no watch during Phases 0-5, so every check that needs hardware is
**deferred, not skipped**, and lands here. Nothing in this file has been run.

Work through it in order when a watch is available. `W*` items are gates that
were deferred; `T*` items are brief §12.3's acceptance tests.

**Before anything else:** find the watch's firmware/kernel version (watch
settings, or the UNA phone app) and check it against `minKernelVersion` **1.4.0**
(ABI 3) recorded in `NOTES.md` 0.2. An app built against a newer SDK can refuse
to launch on older firmware, and one past case corrupted memory — do not install
on a mismatch.

---

## W — deferred gates

| # | Check | Pass condition | Why it is deferred |
|---|---|---|---|
| **W1** | **Rebuild with ST's toolchain.** Install STM32CubeIDE or STM32CubeCLT, put its `arm-none-eabi-gcc` on `PATH`, and rebuild **without** the `-DCMAKE_EXE_LINKER_FLAGS=<stubs.o>` workaround. | Links with no `_write`/`_close`/`_read`/`_lseek` errors, and produces a `.uapp`. | The container has only the distro toolchain, which the SDK documents as incompatible. See `NOTES.md` 0.4. **Do this before W2 — nothing built here should go on a watch.** **Now done by CI** (`NOTES.md` 5.20): the *Watch builds* workflow builds with ST's toolchain in UNA's own CI image, without the stubs. Install its `watch-apps` artifact, not a container build. |
| **W2** | **Gate 0.** Install the *unmodified* RunLVGL `.uapp` and run it. | App appears, main menu renders, full Run flow works. | Needs hardware. If it fails and no firmware update exists, the LVGL decision reverts to TouchGFX (brief §5.1) and Phases 2-4 need rework. |
| **W3** | **Gate 2.** Install HybridX Race and see it on the watch under its own name. | App launches, shows "HybridX Race". | Needs hardware. |
| **W4** | **Gate 4.** Run T1-T8 below. | All pass. | Needs hardware. |
| **W5** | **Gate 6.** Run T9-T15 below. | All pass. | Needs hardware. |
| **W6** | **Memory headroom on target.** Log the LVGL pool peak and check the service against its RAM budget during a full race. | No allocation failures; pool peak comfortably under 100 %. | Simulator figures (50 % of 35 936 B on the main screen) do not predict the watch. |

---

## T — acceptance tests (brief §12.3)

| # | Test | Pass condition |
|---|---|---|
| T1 | Install and launch | App appears, main menu renders, exits cleanly |
| T2 | Short Full race (20-30 s per segment) | 16 segments, correct labels, haptics per type |
| T3 | Roxzone on | 31 segments, no `ROX_OUT` after the final station |
| T4 | Double press R2 | One split only; second press inside the lockout recorded nowhere |
| T5 | Undo last split | Segments merged exactly, total time unchanged. Also try undoing **while paused**: brief §7.3 allows undo from RUNNING only, so it is currently refused (`NOTES.md` 1.5) — confirm that is not annoying in practice |
| T6 | Pause and resume | Pause excluded from active time, included in elapsed |
| T7 | Finish, undo finish, finish again | Single correct finish |
| T8 | End early, then Discard | Correct summary; discarded file removed |
| T9 | Leave app mid-race and return | Behaviour matches `NOTES.md` 0.7 — **see the note below** |
| T10 | External HR strap | HR shown and recorded; `hr_source` reads external |
| T11 | FIT import | **Strava and Garmin Connect both show one lap per segment with correct times** |
| T12 | Phone config change | New value applied on next app launch |
| T13 | Real gym simulation | Usable with sweaty hands; no missed or phantom splits |
| T14 | Battery over 90 minutes | Drain recorded in `NOTES.md` |
| T15 | Exit with no race | No leaked service — sensors disconnected, service gone |
| T16 | Read the race face at arm's length, mid-effort | Segment time legible without squinting; accent colour tells run from station from Roxzone at a glance (the display is 2 bits per channel — see `NOTES.md` 4.3) |
| T17 | The middle dot | `RUN 3/8 · 1 km` shows a dot, not an empty box. The fonts were regenerated for it in Phase 4 but have never been rendered on a real panel |
| T18 | Nothing clipped by the bezel | Every screen, especially the summary's bottom split row and the heart rate above the zone arc. `NOTES.md` 4.3 has the widths these were laid out to |
| T19 | "On your marks" left open | The screen has no idle timeout by design; check the watch's own backlight and sleep behaviour makes that acceptable rather than a battery leak |

### T9 deserves special attention

`NOTES.md` 0.7 found that RunLVGL's service exits ~500 ms after its GUI closes
**even mid-activity**, losing the in-progress FIT until the next launch repairs
it. Our app deliberately diverges. T9 must confirm which of these actually
happens on hardware:

1. Leave the app mid-race → service stays resident and keeps timing.
2. Re-enter the app → race still running, elapsed time correct and continuous.
3. Leave the app mid-race and do **not** come back → after the 5-minute grace
   window the race autosaves and the service exits: no leaked service, no lost
   race. `NOTES.md` 1.2 explains why there is a window rather than an immediate
   save.
4. Kill the app from the launcher mid-race → the FIT is either finalised or
   recoverable, never silently lost.

### T11 deserves special attention

`NOTES.md` 0.8 proved that batched laps (all laps written at save, after the
records and before the session) decode cleanly with `fitdecode`. That is
necessary but **not sufficient** — only a real upload proves Strava and Garmin
Connect accept the ordering. If either rejects it or mis-orders the laps, brief
§10.1's fallback applies: write each lap at the moment it can no longer be
undone, and restrict undo accordingly.

---

## Also worth checking once, opportunistically

- **Companion activity report.** `NOTES.md` 0.6 item 6 concluded the CBOR report
  is built off-watch, with no app-side API. Confirm the phone app shows per-lap
  start times and average HR from our FIT, and that the `supports*` manifest
  flags drive what it renders.
- **Split lockout feel.** The 3 s default (D8) is a guess. T13 is the chance to
  find out whether it is right for sweaty hands mid-race.
- **Haptic legibility.** Whether one strong pulse vs two is actually
  distinguishable through a sweaty wrist at high heart rate, mid-sled-push.

### Added in Phase 5

| ID | Test | Pass criteria |
|---|---|---|
| T20 | Install from the store zip rather than by copying the `.uapp` | The package built by `Utilities/pack-store-zip.sh` installs through the portal and the companion app, the icon and previews appear, and the four AppConfig settings are editable from the phone and reach the watch (T12 covers the value arriving; this covers the packaging around it) |
| T21 | **LVGL pool headroom.** Walk the whole app on the watch — every menu row, a race with splits and toasts, the action menu, finished, saved, both summary pages — with the debug UART attached | `ScreenManager` logs `LVGL pool: .../... B used, peak N%` after every screen switch. In the simulator the peak is **91 %** of a 40 KB pool we cannot enlarge (`NOTES.md` 5.6). Record what the watch reports. Anything at or above 90 % means the fix must land before more screens are added; any rendering glitch or hang during a screen switch is this until proved otherwise |
