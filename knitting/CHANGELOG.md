# Changelog

All notable changes to Knitting are recorded here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and
the project uses [semantic versioning](https://semver.org/spec/v2.0.0.html).

Releases are tagged `knitting-vX.Y.Z`. `Software/cmake/knitting-version.cmake` strips
the prefix for the SDK's version script.

## [Unreleased]

### Added

- **Each button is named on the screen.** "New set" (L1) and "Knit row" (R1)
  at the top, "Hold: undo" under "Knit row" (hold R1), and "Hold: reset" at the
  bottom left (hold L2), so nothing has to be remembered. Taps are white, holds
  amber, as the hold prompts are. The numbers move down a little to make room;
  with the largest numbers (888 rows, Set 88, Total 8888) nothing overlaps.

## [1.0.0] - 2026-10-16

The first store release, tagged `knitting-v1.0.0`. App ID `CDA5282FFDCE502F`,
issued by the UNA portal.

### Added

- **Count rows and sets on the watch.** One screen shows the rows in this set
  (big), the set number and the total rows. R1 counts a row, L1 starts the next
  set (not on an empty set, so a double press leaves no empty sets), R2 exits.
  Limits: 999 rows in a set, 999 sets, 99,999 rows in all.
- **The count is remembered.** It is saved to `state.txt` in the app's folder
  after every change and read when the app starts, so the app reopens where it
  was, also after a power cycle. The write is crash-safe (a `.bak` copy is
  kept), and a damaged file falls back to the backup, then to a fresh counter.
- **Hold L2 for 1.5 s to reset.** A tap on L2 only shows a hint, so a stray
  press cannot wipe the project.
- **Hold R1 for 0.8 s to take back the last row** after a mis-tap. It stays
  inside the set. A short buzz tells you when a row has been taken back or the
  count reset.
- **The store package:** `Utilities/pack-store-zip.sh` builds
  `Knitting-<version>.zip` (the `.uapp`, manifest, icon and four previews),
  checking that the App ID agrees in the three files that name it and that both
  SDK validators pass. CI builds it on every push (artifact
  `knitting-store-zip`). `docs/SUBMISSION.md` has the release day.

### Fixed

- A slow, firm press on R1 (half a second or more) sends no click and would
  have counted nothing. A tap now counts on the click or on the release,
  whichever comes first, once.
