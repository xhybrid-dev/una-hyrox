# Vo2Link: Run's window builder and file codec, copied

These files are **copies** of HybridX Run's (`hybridx-run/Software/Libs/`), so
that HybridX Trail turns its runs into the same window records and writes them
in the same file for Run to count. The apps stay independent: nothing links
across, and CI fails if a copy drifts
(`hybridx-trail/Utilities/check-vo2-link-copies.sh`).

**Edit Run's copy, then copy it here byte for byte** (the script says how).

| Here | Run's original |
|---|---|
| `Header/Vo2WindowConfig.hpp`, `Header/Vo2Window.hpp`, `Header/Vo2WindowFile.hpp` | `Core/Header/` |
| `Sources/Vo2Window.cpp`, `Sources/Vo2WindowFile.cpp` | `Core/Sources/` |
| `Header/SafeFile.hpp`, `Sources/SafeFile.cpp` | `App/Header/`, `App/Sources/` (itself a copy of Streak's) |

Run's judging and estimate (`Vo2Run`, `Vo2Config.hpp`) are not copied: Trail
builds records and judges nothing. See `hybridx-run/docs/VO2_LINK_PLAN.md`.
