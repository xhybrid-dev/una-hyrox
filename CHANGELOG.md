# Changelog

All notable changes to HybridX Race are recorded here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and
the project uses [semantic versioning](https://semver.org/spec/v2.0.0.html).

Releases are tagged twice on the same commit: `vX.Y.Z` for humans and GitHub,
and `apps-vX.Y.Z` because that is the pattern the SDK's version script matches
when it stamps `BUILD_VERSION` into the `.uapp`.

## [Unreleased]

Everything below becomes 0.1.0 once it has run on a watch (Gate 6).

### Added

- **Race timing.** Full race and both halves, with real round numbers in each
  (the second half is rounds 5 to 8, and says so). Optional Roxzone splitting
  turns each station into a Roxzone-in / station / Roxzone-out trio, 31 segments
  instead of 16. No Roxzone-out ever follows the final station.
- **One-button splitting**, with a configurable 1–10 s lock afterwards so a
  double press cannot swallow a segment. The split is timestamped at the button
  press, not when the service gets round to it.
- **Undo** of the last split and of the finish. Times, pauses and heart-rate
  samples merge back exactly, so an undo followed by a re-split is
  indistinguishable from never having undone.
- **Pause and resume**, with paused time excluded from segment and race active
  time but kept in elapsed time.
- **Per-segment heart rate** — average and maximum — from the watch's own sensor
  or an external strap.
- **FIT output**: one lap per segment, carrying `segment_type`, `round` and
  `station_id` as developer fields, plus `race_format`, `roxzone_mode` and
  `completed` on the session. Laps are written in one batch at save time, which
  is what makes undo possible.
- **Summary**: total, runs total, stations total, Roxzone total, average and
  maximum heart rate, then the full split list five rows a page.
- **Adjustable run length.** A HYROX run is 1 km, but a test run-through is
  often 500 m or 800 m, so the run distance is settable in 100 m steps from
  100 m to 1 km. Shorten it and the app calls the session a sim rather than a
  race, on the watch and in the FIT file.
- **Phone-editable settings** through AppConfig: Roxzone splits, run length,
  split lock, vibrate on split, target finish.
- **Haptics** by segment type, and backlight on every split.
- **Store packaging**: `Utilities/pack-store-zip.sh` builds a validated portal
  zip and refuses to build an invalid one.

### Notes

- The service stays resident and keeps timing when the GUI closes mid-race, then
  autosaves and exits after five minutes. The SDK's own activity apps exit about
  500 ms after their GUI closes and lose the in-progress file; see `NOTES.md`
  0.7.
- Every inter-process message is `static_assert`ed against the kernel's 256-byte
  pool, because an oversized send returns null and is dropped in silence on the
  watch while working fine in the simulator.
- Target pacing (F14), the splits face (F15) and FIT workout-step names (F16)
  are deliberately not in this release.
