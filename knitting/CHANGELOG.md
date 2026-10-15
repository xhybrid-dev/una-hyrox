# Changelog

All notable changes to Knitting are recorded here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and
the project uses [semantic versioning](https://semver.org/spec/v2.0.0.html).

Releases are tagged `knitting-vX.Y.Z`. `Software/cmake/knitting-version.cmake` strips
the prefix for the SDK's version script.

## [Unreleased]

### Added

- The count is remembered. It is saved to `state.txt` in the app's folder after
  every change, and read when the app starts, so the app reopens where it was,
  also after a power cycle. The write is crash-safe (a `.bak` copy is kept), and a
  damaged file falls back to the backup, then to a fresh counter.

- K0: the first look. One screen counts rows (R1), starts the next set (L1)
  and resets (L2); R2 exits. The count is kept in memory only.
