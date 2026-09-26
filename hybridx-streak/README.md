# HybridX Streak

A weekly training-streak tracker for the [UNA Watch](https://unawatch.com).

Every activity your watch records — from any app — counts automatically
towards a weekly target. Hit it, and the climber steps up a mountain; miss
it and a shield can save the streak before it resets. It's independent of
HybridX Race, which lives alongside it in this same repository.

- **Automatic counting.** No separate logging step for the apps you already
  use — the app scans on open and credits what qualifies.
- **A weekly target**, default 3 sessions from Monday, adjustable, with an
  optional minimum duration and a "one per day" cap.
- **Live achievement.** The week is won the moment it hits target, not at
  the boundary — the boundary only judges a miss.
- **Shields.** One earned per four achieved weeks (cap 2), offered on a
  miss rather than resetting the streak outright.
- **A glance**, so the week is visible without opening the app.
- **Manual logging** for anything the watch never recorded.

Status: **v0.1.0, unreleased.** Phases S0–S4 of the build plan are complete.
Nothing has yet run on a watch — see `docs/PROBE.md` (Gate 0) and
`docs/PLAN.md` §11 (Gate 6).

![Week complete](docs/screens/real/02-week-complete.png)

## Controls

Buttons follow UNA's convention: R1 means yes or go, R2 means no or back.
L1/L2 move between items.

| | Home | Menu | This week | Log a session | Settings |
|---|---|---|---|---|---|
| **L1/L2** | — | Move | Move | — | Move |
| **R1** | Open menu | Select | Leave out / undo | Next | Choose |
| **R2** | Exit | Home | Back | Back | Back |

## Repository layout

```
hybridx-streak/
  Resources/                    icon, app-manifest.json, store previews
  Software/
    Libs/
      Core/                     StreakModel, ActivityScanner, FitSessionReader (pure C++)
      App/                      Service, Commands, AppConfig
      Glance/                   the glance's projection + layout
    Apps/HybridXStreak-CMake/     the main watch build
    Apps/HybridXStreakGlance-CMake/  the glance's watch build
    Apps/LVGL-GUI/               the GUI process, and its PC simulator
  Tests/Host/                   host tests, no watch needed
  Utilities/pack-store-zip.sh   builds the portal upload package (main app only)
  Tools/Probe/                  the Gate 0 go/no-go probe
  docs/                         brief, PLAN, DESIGN, NOTES, screenshots
```

The workspace root's `README.md`, `CHANGELOG.md`, `LICENSE` and
`THIRD-PARTY-LICENSES.md` predate this app (this repo started as HybridX
Race alone) and describe Race specifically; this file and
`CHANGELOG.md` here are Streak's own.

## Getting set up

The one-time SDK/toolchain setup (`una-sdk`, `UNA_SDK`, the ARM toolchain,
CMake/SDL2 for the simulator) is shared with HybridX Race — see the root
[`README.md`](../README.md) "Getting set up" section. Nothing here repeats
it.

## Building

**For the watch.** Produces `hybridx-streak/Output/HybridXStreak_<version>.uapp`.

```bash
cmake -G "Unix Makefiles" -S hybridx-streak/Software/Apps/HybridXStreak-CMake -B hybridx-streak/build
cmake --build hybridx-streak/build
```

**The glance**, a separate binary — produces
`hybridx-streak/Output/Glance/HXStreakGlance_<version>.uapp`.

```bash
cmake -G "Unix Makefiles" -S hybridx-streak/Software/Apps/HybridXStreakGlance-CMake -B hybridx-streak/build-glance
cmake --build hybridx-streak/build-glance
```

**The host tests.**

```bash
cmake -S hybridx-streak/Tests/Host -B hybridx-streak/build-tests
cmake --build hybridx-streak/build-tests
hybridx-streak/build-tests/hybridx-streak-host-tests
```

**The simulator.** The whole app, service included, on your PC.

```bash
cmake -S hybridx-streak/Software/Apps/LVGL-GUI/simulator -B hybridx-streak/Software/Apps/LVGL-GUI/simulator/build
cmake --build hybridx-streak/Software/Apps/LVGL-GUI/simulator/build
```

It cannot run glances (no `EVENT_GLANCE_*` in the simulator's kernel test
doubles) — the glance's layout is previewed separately: see
`docs/experiments/glance_preview.py`.

Build the main app, the glance, **and** the simulator after every change;
their source lists (`Software/Libs/libs.cmake`) are maintained separately.

## Installing on a watch

Only install a `.uapp` built by CI's **Watch builds** workflow (see the root
README's "Installing on a watch" — the same artifact, `watch-apps`, holds
HybridX Race, plus this app's main binary, its glance and its probe). Then,
per binary:

1. Connect the watch by USB, create `Apps/HybridXStreak/` (or
   `Apps/HXStreakGlance/` for the glance), and copy its `.uapp` in.
2. Eject safely, unplug, power-cycle the watch.
3. Top right button, and the app is in the list. The glance appears on the
   glances screen instead, once installed.

## Building the store package

```bash
hybridx-streak/Utilities/pack-store-zip.sh
```

Main app only — the glance stays side-load/CI-only for now; see
`docs/NOTES.md` S5.1 for why. Otherwise this works exactly like Race's
script: it reads the version out of the built `.uapp`, stamps
`minKernelVersion`, runs both SDK validators, and only then writes
`hybridx-streak/Output/HybridXStreak-<version>.zip`.

> **Before the first real upload:** the `APP_ID` in `CMakeLists.txt` and
> `app-manifest.json` is a locally generated development ID — see the root
> README's equivalent note for the exact steps.

## Versions

Tagged `streak-vX.Y.Z`; `Software/cmake/streak-version.cmake` strips the
prefix for the SDK's version script (kept separate from Race's `apps-v*`
tags so the two don't collide).

```bash
git tag streak-v0.1.0 && git push --tags
```

## Where everything is written down

| | |
|---|---|
| [`docs/UNA_STREAK_TRACKER_BRIEF.md`](docs/UNA_STREAK_TRACKER_BRIEF.md) | the specification this is built to |
| [`docs/PLAN.md`](docs/PLAN.md) | phases, gates, decisions made with Jon |
| [`docs/DESIGN.md`](docs/DESIGN.md) | the Summit look and voice |
| [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | how the app is put together, and why |
| [`docs/NOTES.md`](docs/NOTES.md) | the running log: findings, measurements, decisions, every place the SDK and the plan disagree |
| [`docs/PROBE.md`](docs/PROBE.md) | the Gate 0 go/no-go check, step by step |
| [`CHANGELOG.md`](CHANGELOG.md) | what changed, per release |

## Licence

MIT, same terms as the rest of this workspace — see the root
[`LICENSE`](../LICENSE) and [`THIRD-PARTY-LICENSES.md`](../THIRD-PARTY-LICENSES.md).
