# HybridX Race: getting it to the store and into the competition

Written 9 October 2026. **Competition deadline: Friday 30 October 2026, 4pm UK.**
The email says to build an app, publish it to the store, and enter; judging is
usefulness, originality and UX/UI, each out of 10.

Everything below that can be done without store access is done. What is left is
waiting on UNA (portal access) and on Jon (watch tests, one decision, the entry).

## Where things stand

| | |
|---|---|
| App, fixes, tests | Done on branch `claude/race-final-pass` (PR #17). 100 host tests; CI green on the latest commit |
| Store zip | Built by CI on every run (`race-store-zip` artifact). Dry run until the real App ID is in |
| Store previews, icon, manifest | Done; previews retaken after the title fix |
| Portal access | **Waiting on UNA** |
| App ID | Needs the portal. Today's is a development ID, and the packager refuses a release with it |
| Watch tests T22-T24 | **Jon**, on the CI build. T20 and T23 need the store install |
| Font licence (NOTES 5.5) | **Jon to raise with UNA.** Not a blocker we know of, but unanswered |
| HYROX wording (D1) | **Jon's decision**, see below |

## The day portal access arrives (about an hour of Jon's time)

1. Sign in at apps.unawatch.com, **Add New**, enter the name and description
   below, **Generate**, and copy the **App ID**.
2. Send Claude the App ID. It goes in two places, `CMakeLists.txt` (`APP_ID`)
   and `Resources/app-manifest.json` (`id`); the packager refuses a release where
   they differ.
3. Merge PR #17 (or tell Claude to).
4. Claude updates `CHANGELOG.md` ("Unreleased" becomes 0.1.0) and tags the merge
   commit `v0.1.0` and `apps-v0.1.0`. A tag push is outward-facing, so Claude
   asks first.
5. Wait about five minutes for *Watch builds* on the tag. Download
   **race-store-zip** (`HybridXRace-0.1.0.zip`) and, to test, **watch-apps**.
   The `.uapp` inside both is the same file.
6. Install `HybridXRace_0.1.0.uapp` on the watch by USB and run one race: that is
   the exact binary going to the store.
7. Portal: the app's **Version** tab, **Upload New**, the zip, **Release**,
   **Confirm and Publish**.
8. Install from the store through the UNA app on the phone, and run T20 and T23
   (`ON_WATCH_TESTS.md`).
9. Enter the competition.

If portal access has not come by **Wednesday 14 October**, write to UNA again.
`support@una-watch.dev` is the address their docs give for critical issues, and
the competition email is itself a reason. UNA's docs also describe an
open-source route (a pull request adding the app under `Examples/Apps/` of the
`una-sdk` repository). It needs the app to be MIT-licensed and public, which it
is not yet decided to be (D5), and review time is UNA's. It is a fallback, not a
plan.

## Store text (draft, Jon to edit)

**Name:** HybridX Race

**Short description (portal):**
Race and simulation timer for HYROX-format events. One button per split; every
run and station is recorded as a lap.

**Longer description:**

> Time a HYROX-format race or training simulation from your wrist, with one
> button.
>
> - Full race, or either half (rounds 1-4 or 5-8). Optional Roxzone splitting.
> - One press splits to the next run or station. A lock stops a double press
>   from skipping one, and you can undo the last split or the finish.
> - Pause and resume for training. Paused time is kept out of the segment times.
> - Heart rate on screen with its zone, plus the average and maximum for every
>   segment. A different buzz tells you whether a run, a station or the Roxzone
>   is next.
> - Shortening the runs (100 m to 1 km) turns it into a simulation, and says so.
> - A summary of total, run, station and Roxzone time, with every split.
> - The activity is saved with one lap per segment, each with its distance, and
>   syncs to Strava through the UNA app.
>
> Made by HybridX. Not affiliated with or endorsed by HYROX.

Facts in that text are checked against the code. It deliberately does **not**
claim named laps in Strava or Garmin (unconfirmed, NOTES 5.12) or moving time in
Strava (NOTES 5.19), and "syncs to Strava" rests on the 1 October test.

**D1, the HYROX wording.** It is used as "HYROX-format" and in the last line,
and nowhere in the app name or icon. Whether that line is accurate and wanted
is Jon's call; "HYROX" is somebody's trademark.

## Competition entry (draft, Jon to edit)

The entry form was not in the email text (it is behind a button), so the length
limits are unknown. These are short on purpose.

**Usefulness.** HYROX-style training is done on a watch that has no idea what a
"station" is. Athletes either press lap and remember which was which, or re-key
splits afterwards. Race does the bookkeeping: one button, the right segment
named on screen, a buzz for what is next, and a file that arrives in Strava with
every segment as a lap. It is also useful for coaches: a race simulation at a
shorter run length is a normal test piece, and the app handles it.

**Originality.** Race timing apps exist; one built around a multi-discipline
format with its own segment order, optional Roxzone splitting, undo and a lap
per segment is a different thing. *Jon to check the store for anything similar
before claiming it is the first.*

**UX and UI.** Designed for a sweaty hand at 180 bpm on a round 240 px display:
the segment time is the largest thing on screen, colour separates runs from
stations from the Roxzone, a mistaken press can be undone, and the buttons
mean the same thing everywhere. It was tested on a watch in a real race on
1 October (31 laps, 5.68 km, matching Strava and the UNA app) and the problems
that found are fixed.

## Schedule

| By | What | Whose |
|---|---|---|
| Sun 11 Oct | Install the CI build; run T22, T24 and one full race; upload the round 5 Strava files (NOTES 5.28) | Jon |
| Mon 12 Oct | Decide: merge PR #17; D1 wording; anything the tests turned up | Jon |
| Wed 14 Oct | No portal access yet: write to UNA | Jon |
| Fri 16 Oct | App ID in, tags pushed, store zip uploaded | Jon and Claude |
| Wed 21 Oct | Store install from the phone; T20, T23 | Jon |
| Fri 23 Oct | **Feature freeze.** Bug fixes only | |
| Tue 27 Oct | Competition entry sent | Jon |
| **Fri 30 Oct, 4pm** | **Deadline** | |

If the portal opens later than the 16th, everything after it slides, and the
freeze and the entry stay where they are. UNA's review of an upload, if there is
one, is the unknown: upload as soon as the ID exists.
