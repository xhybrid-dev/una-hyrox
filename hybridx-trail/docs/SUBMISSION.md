# X Trail 1.1.0: the store update and the November competition

Written 24 November 2026. X Trail 1.0.1 is on the store; 1.1.0 is the first field test's
amendments (NOTES "T4"), packaged for the store and for UNA's November app competition.

**The competition's rules and deadline are not in this repository.** Race's entry (October,
`hybridx-race/docs/SUBMISSION.md`) was judged on usefulness, originality and UX/UI, each out of
10; the entry below follows that shape. Check the November email for its deadline, its form and
whether an update to a listed app can be entered.

## Where things stand

| | |
|---|---|
| The app | Built: wrong way, off course at 20 m, the alert as a cap across the top, the run after the finish, the folder on the route list, and two fixes (NOTES "T4"). 172 host tests, also under ASan + UBSan |
| On a watch | **Not yet.** Every change is checked in the simulator only (NOTES "For Jon on the watch") |
| The package | Ready. `Utilities/pack-store-zip.sh --expect-version 1.1.0` passes locally on a 1.1.0 build: the portal's manifest rules, both SDK validators, the App ID (`1C83FDDE4EA3C880`) the same in all three places, six previews. That local zip was a compile check and is not for upload: CI builds the real one from the tag |
| Previews (in the zip) | Six, from the simulator: routes, a route, the map, off course (the new cap), **wrong way** (new), the climb |
| Gallery (on the listing) | Regenerated in `docs/store/`: the banner, four cards (off course at 20 m, your routes, **wrong way**, the climb) and the family card, now with X Training |
| Store text | The listing's description is updated in `hybridx-link/docs/STORE_RELEASE.md` ("X Trail"); the release notes are below |

## The steps (about 45 minutes of Jon's time, plus a run)

1. **Merge** this branch's pull request into `main` (earlier Trail tags are on `main`'s merge
   commits).
2. **Make the tag on GitHub.** Claude's session cannot push tags.
   1. Open the repository, then **Releases**, then **Draft a new release**.
   2. **Choose a tag:** type `trail-v1.1.0`, choose **Create new tag on publish**.
   3. **Target:** `main`, at the merge commit.
   4. **Title:** "X Trail 1.1.0". Paste the release notes below if you like.
   5. Choose **Publish release**.
3. **Wait about ten minutes** for *Watch builds* on the tag (the **Actions** tab). On its run
   page, download **trail-store-zip** (`HybridXTrail-1.1.0.zip`) and **watch-apps**. On a tag,
   CI refuses to make the zip if the binary's version is not 1.1.0.
4. **Check the exact binary on the watch.** Copy `HybridXTrail_1.1.0.uapp` from the zip into
   `Apps/1C83FDDE4EA3C880/` over USB (the store install's folder; delete the old `.uapp` there,
   keep `Routes/` and the other files), power-cycle, and do the short test: NOTES "For Jon on
   the watch", at least items 1, 2 and 5. Then a run on an out-and-back if you can (item 3).
5. **Upload.** On X Trail's listing: the **Version** tab, **Upload New**, the zip itself (not
   its contents), **Release**, **Confirm and Publish**. The App ID is unchanged, so the store
   matches it to the listing.
6. **The listing:** paste the description from `STORE_RELEASE.md` ("X Trail"), and replace the
   gallery with `docs/store/` (banner first; `docs/store/README.md`).
7. **Install from the store** through the UNA app, then **enter the competition** (below).

**If the portal refuses the zip:** send Claude its message word for word. The fix goes in,
`trail-v1.1.1` is tagged, and that zip goes up instead.

## Release notes (for the GitHub release and the portal, if it asks)

> **X Trail 1.1.0**, after a field test on a real route:
>
> - **Wrong way:** on the route but running it backwards? A different buzz, and an arrow the
>   right way. "Right way" when you are.
> - **Off course at 20 m** from the line (it was 50 m), and back on course within 12 m.
> - **The alert sits across the top of the screen,** so the map stays in view below it.
> - **After the finish,** the map shows your time and distance until you save.
> - **Add routes** tells you the folder to copy GPX files into.
> - Fixed: "Back on course" shows again; turning just short of an out-and-back's turnaround no
>   longer sends "to go" back up.
> - With X Streak: where your form stands, on the Saved screen.

## Competition entry (draft, Jon to edit)

The lengths the form allows are unknown, so these are short.

**Usefulness.** UNA Watch has no route navigation, and trail runners get lost. X Trail puts any
GPX route on the wrist, as exported from OS Maps, Komoot or Strava, with no conversion: a map
with you on the line, distance to go, turn cues and the climb ahead. It buzzes when you are 20 m
off the line, with an arrow back, and when you are on the route but running it the wrong way. It
records the run as well (to the UNA app and Strava) and counts in X Streak. It was field-tested
on a real route, and this version is what that run asked for.

**Originality.** *Jon to check the store for any other route navigation before claiming it is
the first.* It reads the GPX on the watch, a chunk at a time in fixed memory, so a 50 km route
takes a third of a second. It knows where you are *along* the route, not just near it, so loops,
out-and-backs and figure-of-eights count down correctly where "the nearest point" goes wrong.
That is also how it tells the wrong way: from your progress along the route, which is steady,
not the GPS heading, which swings by tens of degrees at running pace.

**UX and UI.** Built for a round 240 px screen read on the move: the alerts sit in a cap across
the top, so the map and your arrow stay in view; you can tell the alerts apart without looking
(off course two long buzzes, wrong way a quick three, left one click, right a double click, back
on a short double); the map zooms in and out on the up and down buttons; and the app tells you
the folder your GPX files go in, which a store install's name hides. The alerts' text is
measured to fit the round face at the height it sits.

*Facts checked against the code and NOTES (T0 for the read time, T1 for the tracker, T3-T3c for
the map, turn cues and buzzes, T4 for this version). Not claimed: the compass (untested on the
watch), and that it is the first.*
