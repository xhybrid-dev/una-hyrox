# Changelog

All notable changes to Knitting are recorded here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and
the project uses [semantic versioning](https://semver.org/spec/v2.0.0.html).

Releases are tagged `knitting-vX.Y.Z`. `Software/cmake/knitting-version.cmake` strips
the prefix for the SDK's version script.

## [Unreleased]

### Added

- Hold L2 for 1.5 s to reset; a tap on L2 only shows a hint. Reset is no longer
  one stray press away.
- Hold R1 for 0.8 s to take back the last row (a mis-tap). It stays inside the set.
  A short buzz tells you when a reset or an undo has happened.

- The count is remembered. It is saved to `state.txt` in the app's folder after
  every change, and read when the app starts, so the app reopens where it was,
  also after a power cycle. The write is crash-safe (a `.bak` copy is kept), and a
  damaged file falls back to the backup, then to a fresh counter.

- K0: the first look. One screen counts rows (R1), starts the next set (L1)
  and resets (L2); R2 exits. The count is kept in memory only.

### Fixed

- A slow, firm press on R1 (half a second or more) sends no click and counted
  nothing. A tap now counts on the click or on the release, whichever comes first.
