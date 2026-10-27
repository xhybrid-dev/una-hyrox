# HybridX Activity Link

The way our activity apps tell HybridX Streak what you did. Each app leaves a
small summary in the watch's shared folder; Streak reads them. Built so a bike
or workout app can join later without touching Streak.

**Status.** Phases 1 (the format, the shared code and its host tests), 2 (Run
writes its summary), 3 (Streak reads the summaries, in the app and the glance) and
4 for Race and Trail (each writes its summary) are built and checked in the simulator,
together as well as one at a time ("Family check"). Nothing has run on a watch yet: that
is Jon's test, below.

**Who is in the family.** Writers: HybridX Run (`act_run.json`), HybridX Race
(`act_race.json`), HybridX Trail (`act_trail.json`). Reader: HybridX Streak (the app and
its glance). Not in it: X Sauna (Jon, 27 October 2026: its sessions stay out of the
shared data and the streak count), and the apps that record no activity (X Weigh-in,
Knitting, Trail Quest).

## Why it exists

Streak counts every activity the watch records, but on a real watch it cannot
read other apps' `Activity/` folders: the firmware lets an app open
`../SharedData/...` and nothing else (Streak NOTES, Gate 0), and the UNA phone
app deletes each `.fit` after a sync. So the apps tell Streak themselves, through
the one folder that is open. This is Streak PLAN §3 option B.

**What it does not do.** It counts our own apps only. UNA's built-in Running,
Walking and the rest still cannot be counted until UNA gives apps a way to see
them (draft request: `hybridx-streak/docs/UNA_ACTIVITY_REQUEST.md`, not sent).

## The rules

1. **One file per app, and that app is its only writer.** The SharedData folder
   has no locking, and a failed `open()` cannot tell "missing" from "another app
   has it". Single writers cannot corrupt each other. (The Run link and Streak's
   public file follow the same rule.)
2. **Fixed file names.** Listing the shared folder is not shown to work, so the
   reader opens the names in `kSlotPaths`, and never lists.
3. **Written only for an activity that was saved.** After the app's `.fit` has
   closed successfully. Not on discard, not for an empty session, not if the file
   failed to close.
4. **Never in the way.** A failure to write the summary is logged as a warning.
   It must never change whether the activity is saved.
5. **Integers only, none negative.** The SDK's JSON writer misprints signed 32-bit
   values on the host (`%ld`). The time-zone offset is stored 1440 higher.

## The file

`../SharedData/HybridX/act_<slot>.json`, written with the app's `SafeFile`
(`.tmp`, then `.bak`, then the file). The last 16 activities, oldest first:

```
{"v":1,"app":"HybridXRun","a":[[1791980037,1500,2700,5400,152,171,1],...]}
```

| Field | Meaning |
|---|---|
| `v` | Version, 1. A reader refuses any other. |
| `app` | The app's folder name (1 to 15 characters of `A-Z a-z 0-9 _ -`). Streak hashes it into the same key its folder scan uses, so one activity seen both ways counts once. |
| `a[i][0]` | Start, UTC seconds. At least 1 January 2020. |
| `a[i][1]` | Time-zone offset in minutes at the start, **plus 1440** (UTC-12 to UTC+14). |
| `a[i][2]` | Timer time in seconds (up to 48 hours). This is what Streak counts. |
| `a[i][3]` | Distance in metres, 0 if none (up to 1000 km). |
| `a[i][4]`, `[5]` | Average and maximum heart rate, 0 if none. Average is never above the maximum. |
| `a[i][6]` | Sport: 1 run, 2 ride, 3 walk, 4 strength, 5 workout, 6 hybrid, 7 row, 9 other. Our codes, not FIT's. An unknown code reads as other. |

Local start (what Streak's dedupe uses) is `startUtc + (a[i][1] - 1440) * 60`.

The reader checks every value, because the SDK's reader does not. A file that is
missing, malformed, from another version or holds more than 16 entries reads as
nothing. One bad entry is skipped and counted; the rest are kept. The worst case
file is about 710 bytes (`kMaxBytes` is 1024).

Distance and heart rate are carried for later; Streak ignores them today, so
the link does not change Streak's saved state.

## The code

`hybridx-link/Software/Libs/Link/` is the original (`HybridXLink` namespace). It
has no file I/O: it turns an entry into text and back, keeps the list, and holds
the table of file names. Each app keeps a byte-identical copy in its own
`Software/Libs/Link/`, and `Utilities/check-link-copies.sh` (run by CI) fails if a
copy has drifted. Change the original, then run it with `--sync`.

- `append(oldText, oldLen, app, entry, buf, cap)`: what an app calls. It parses the
  existing file (a bad one starts a fresh list), inserts the entry by start time,
  keeps the newest 16 and writes the new text. The app reads the old file and
  writes the new one with its own `SafeFile`.
- `decode(text, len, file)`: what Streak calls for each file.
- `makeEntry(...)`: an entry from an app's float totals, rounded and bounded (NaN or
  negative becomes 0, the average never above the maximum). A bad start still fails
  `valid()`, so it is not written.
- `calendarSeconds(y, m, d, h, mi, s)` and `tzOffsetMinutes(utc, localSeconds)`: the
  zone offset from the UTC start and the kernel's broken-down local time of it.
- `kSlotPaths`: Run, Race, Trail, Ride, Workout. Ride and Workout are reserved now,
  so those apps work with a Streak that is already released.

Host tests: `cmake -S hybridx-link/Tests/Host -B hybridx-link/build-tests &&
cmake --build hybridx-link/build-tests && hybridx-link/build-tests/hybridx-link-host-tests`.

## Adding an app

1. If its slot is not in `kSlotPaths` (not run, race, trail, ride or workout), add
   it to the end of `Slot` and the table, in the original, bump `kSlotCount` (the
   limit is `kMaxSlots`, 8), and release Streak. Otherwise skip this.
2. Add the app's `Software/Libs/Link` to `APPS` in `check-link-copies.sh` and run
   it with `--sync`. Add the sources to the app's `libs.cmake` and its host tests.
3. In the app's save path, after its `.fit` has closed successfully, build an
   `Entry` with `makeEntry` and call `append`. `Service::writeActivityLink` in
   `hybridx-run/Software/Libs/App/Sources/Service.cpp` is the model: keep the start
   the `.fit` is named from, read the old file (primary, else `.bak`), `append`, write
   with `SafeFile`, use heap buffers, and log a failure as a warning. Use the app's
   folder name as `app`.
4. Test it in the simulator, then on the watch (see "Checks on the watch").

## Phases

| Phase | Work | Status |
|---|---|---|
| 1 | The format, `hybridx-link`, host tests, copy check, CI | Done. Jon to review the format. |
| 2 | Run writes its summary | Done (simulator-checked; see Run NOTES, "Activity link") |
| 3 | Streak reads the summaries (app and glance) | Built (simulator-checked; see Streak NOTES, "Activity link"). Jon's watch test next. |
| 4 | Race, Trail, then the bike and workout apps, one at a time | Race and Trail done (simulator-checked; Race NOTES "Amendment — Race tells Streak", Trail NOTES "Activity link"), and the family check passes. Bike and workout later. |

## Family check (the simulator)

`hybridx-link/docs/experiments/family_check.sh [out-dir]` puts the whole family on one
pretend watch:
- Run, Race and Trail each record and save an activity in their own simulator, at the
  same time, each from its own app folder;
- all three share **one** `SharedData` folder (each app's `../SharedData` is a symlink
  to it, as on a watch every app's is the same folder);
- then Streak opens twice, from its own folder in the same tree.

It checks, with the apps' real writers and Streak's real reader:
1. each app left one entry, under its own name and sport;
2. each entry has its `.fit` session's start and timer time;
3. Streak's first open finds all three (`Summaries: 3 files, 3 entries, 3 new`);
4. its saved state counts each once (Run, Hybrid, Run);
5. the second open finds nothing new and still holds three.

It also reports whether Trail left minutes for Run's VO2max (for information: the
simulator's heart rate is not steady running, so there are usually none).

- **Quick mode** (the default, about 3 minutes): about 80 s each, with Streak's
  minimum set to 0 minutes so the short sessions count.
- **`LONG=1`** (about 14 minutes): 11 minutes each, with Streak's default 10-minute
  minimum.
- `SHOTS=<dir>` saves Streak's screen after each open.

It needs the four simulators built, Xvfb, xdotool and fitdecode. It prints a PASS/FAIL
table and exits non-zero on any FAIL. Run it after any change to a writer, the
library or Streak's reader. It runs by hand, not in CI: CI has no simulators.

Last run (27 October 2026): every check passed in both modes. Streak showed 3 of its 7
dots after the first open, and the same after the second. In `LONG=1`, the 11-minute
sessions counted under Streak's default 10-minute minimum, and Trail also left
`trail_vo2_0.json` for Run's VO2max. Run recorded at the same time, so it had not yet
opened to read the file.

## Checks on the watch (the phase 3 gate: Jon's test)

Nothing here has run on a watch yet. Install the CI builds of Run and Streak (and
Streak's glance if you use it), then:

1. Open Streak once, so it has a saved streak. (Its scan will find nothing: it
   cannot read other apps' folders. That is expected.)
2. Record a Run of at least Streak's minimum minutes (10 by default) and save it.
   A short one is fine for the test: set the minimum to 0 in Streak's settings.
3. Open Streak. This week's count should be up by one, and the toast should say
   "+1 Run". Open it again: the count must not go up a second time.
4. Over USB, `Apps/SharedData/HybridX/act_run.json` should exist and hold the run
   (about 36 bytes per run). Note the start time and the zone: `1440` stored is
   UTC, `1500` is UTC+1.
5. Record another Run and discard it. The file must not change.
6. If you have the glance, check it shows the new count after step 3.
7. Then the same with Trail (and Race if you have it): record and save a Trail run, open
   Streak, and look for "+1 Run" and `act_trail.json`. All the apps' sessions should be
   listed, each once.

Tell me what the file holds and what Streak showed, whichever way it went.

## Not known

- Whether two apps writing different `act_*` files at once is fine on the watch
  (it should be: they are separate files).
- Whether two activity apps can record at the same time. Each writes only when its
  own activity ends, so it should not matter.
- The SDK says the watch has ten open-file slots in all. Streak opens at most
  eight files, one at a time.
- If Streak is not opened for more than about two weeks, older entries fall outside
  its window and are not counted. Its folder scan has the same limit.
- A file in the format is a promise: once apps in the field write `v:1`, a change
  needs `v:2` and a reader that takes both.
