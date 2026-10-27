# HybridX Activity Link

The way our activity apps tell HybridX Streak what you did. Each app leaves a
small summary in the watch's shared folder; Streak reads them. Built so a bike
or workout app can join later without touching Streak.

**Status.** Phase 1 (the format, the shared code and its host tests) is done.
Run's writer is phase 2; Streak's reader is phase 3 (see "Phases").

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
- `tzOffsetMinutes(utc, localSeconds)`: the offset from the kernel's two clocks.
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
   `Entry` and call `append` (the Run writer in phase 2 is the model). Use the
   app's folder name as `app`.
4. Test it in the simulator, then on the watch (see "Checks on the watch").

## Phases

| Phase | Work | Status |
|---|---|---|
| 1 | The format, `hybridx-link`, host tests, copy check, CI | Done. Jon to review the format. |
| 2 | Run writes its summary | Not started |
| 3 | Streak reads the summaries (app and glance) | Not started |
| 4 | Race, Trail, then the bike and workout apps, one at a time | Later |

## Checks on the watch (phase 3 gate)

Nothing here has run on a watch yet. When it has: record a short Run activity
that meets Streak's minimum minutes, open Streak, and check this week's count went
up by one. Over USB, check `Apps/SharedData/HybridX/act_run.json` exists and holds
the run. Check a discarded run leaves no entry.

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
