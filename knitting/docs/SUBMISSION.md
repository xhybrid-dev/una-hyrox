# Knitting: getting it to the store

Written 16 October 2026 (phase K5, NOTES). This follows X Weigh-in's
`x-weighin/docs/SUBMISSION.md`, which went through the portal, and the SDK's
route (`una-sdk/Docs/deploy.md`). One listing, one package: the app. Knitting
has no glance and no phone settings.

Everything that can be done without Jon is done. What is left:
- Jon's check of the holds on the watch (Gate K2), if not already done;
- the release day below.

## Where things stand

| | |
|---|---|
| App | K0 (counter), the saved count, and the holds (reset, undo) are built. 44 host tests |
| App ID | **Issued by the portal (16 October 2026) and in place:** `CDA5282FFDCE502F`, in `Software/Apps/Knitting-CMake/CMakeLists.txt`, `Resources/app-manifest.json` and the simulator's `CMakeLists.txt`. It replaces the development ID `E7D056DCC90561F1`, which the packager refuses on a release |
| Package | Built by CI on every push, in the `knitting-store-zip` artifact: `Knitting-<version>.zip`. A dry run until the commit is tagged |
| Manifest | Checked by the packager: the activity keys the portal requires, no `previews` key (the lessons of X Weigh-in 1.0.0), no phone settings, `minKernelVersion` stamped from the SDK's ABI |
| Icon | `Resources/icon_60x60.png` (shipped as `icon.png`), lime needles over a ball of yarn |
| Previews | Four 480×480 simulator screens in `Resources/previews/`, from `docs/experiments/store_previews.sh` (a made-up project: 6 rows, Set 3, 22 in all) |
| Store images | In `docs/store/`: the four previews, each screen on a teal and a white UNA watch (8 mock-ups), five feature cards and a banner. Made by `docs/experiments/store_images.py`; see `docs/store/README.md`. Only the previews go in the zip |
| Version | 1.0.0, from the one tag `knitting-v1.0.0` |

## The release day (about half an hour of Jon's time)

0. **Before anything: check the holds on the watch** (Gate K2, NOTES).
   Hold-to-reset is the one feature the simulator cannot prove: the real
   watch's button codes are only documented, not seen. If a hold does nothing on
   your watch, say so and do not release.
1. **Merge the branch** `claude/x-rows` (it carries the holds and this
   packaging) into the default branch, or tell Claude to.
2. **Make the tag `knitting-v1.0.0` on GitHub** (steps at the end). A tag push is
   outward-facing and Claude's session cannot push tags, so this is Jon's.
3. **Wait about five minutes** for *Watch builds* on the tag. On its run page,
   download **knitting-store-zip** and **watch-apps**.
   - **knitting-store-zip** holds `Knitting-1.0.0.zip`.
   - The `.uapp` inside it is the same file as `Knitting_1.0.0.uapp` in
     watch-apps.
   - If the run fails at the store package step, the message says why:
     most likely the binary is not 1.0.0 (a stale tag) or the ID differs.
4. **Install that binary by USB and check it once.** These are the exact bytes
   going to the store.
   - Copy `Knitting_1.0.0.uapp` into `Apps/Knitting/`. **Delete the old
     `Knitting_0.0.0-dev.uapp` there.** Keep `state.txt`: it is your count.
   - Eject safely and power-cycle.
   - Count some rows, start a set, hold R1 (undo), hold L2 (reset), close and
     reopen. Check the app's icon in the launcher.
5. **Upload.** On the portal, Knitting's **Version** tab: **Upload New**, choose
   `Knitting-1.0.0.zip` (the zip itself, not its contents), then **Release**,
   then **Confirm and Publish**.
6. **Install from the store** through the UNA app on the phone. Remove the USB
   copy first (the `.uapp` only). Then run Gate K5 (NOTES): the app opens,
   counts, and keeps its count.

## Store text (draft, Jon to edit)

**Name:** Knitting

**Short description (portal):**
Count your knitting rows and sets on your watch, one press per row.

**Longer description:**

> Count rows without losing your place.
>
> - Press once for every row. The rows in this set show big; the set you are on
>   and your total show beneath.
> - Finished a set? One press starts the next, and the rows begin again from
>   zero. "8 rows, four times" is just eight presses, a press, and again.
> - Every button is named on the screen, so there is nothing to remember: knit
>   row, new set, and the two holds.
> - Pressed too often? Hold the row button for a moment to take the last row
>   back.
> - Want to start again? Hold the reset button. It needs a hold, so a stray tap
>   cannot wipe your project.
> - A short buzz tells you when a row has been taken back or the count reset, so
>   you can feel it without looking.
> - Your count is saved every time it changes. Close the app, or switch the
>   watch off, and it opens exactly where you left off.
> - Your count stays on the watch: nothing is sent to the phone, the UNA app or
>   Strava, and it is not recorded as an activity.
>
> Made by HybridX.

**Facts checked against the code:**
- The four labels (New set, Knit row, Hold: undo, Hold: reset): `MainScreen.cpp`.
- A row counts on a press of R1 (a slow press too): `HoldTimer`, `MainScreen`.
- The next set starts on L1, and only if the current set has a row, so a double
  press does not leave an empty set: `RowCounter::nextSet`.
- Holding R1 for 0.8 s takes back one row, and stays inside the set: `undoRow`.
- Holding L2 for 1.5 s resets to 0 rows, set 1; a tap only shows a hint.
- The limits are 999 rows in a set, 999 sets and 99,999 rows in all.
- The count is written after every change and read at start: `StateStore`.
- A short buzz on undo and reset: `Service::buzz`. The watch's own vibration
  setting may silence it, so the text says "a short buzz", not that it always
  buzzes.
- No FIT file is written and no phone setting is read: the decision of
  15 October.

**Not claimed:**
- that the count survives an uninstall (the app's folder is deleted with it;
  there is no copy in the shared folder yet);
- a target for rows per set, a buzz when a set is complete, a glance, or several
  projects (all possible later);
- that a hold works the same on every firmware (it follows the kernel's press
  and release codes, as Race's hold-to-finish does).

## Making the tag on GitHub

Claude's session cannot push tags, so the tag `knitting-v1.0.0` is made on
GitHub, as a release:
1. Open the repository on GitHub, then **Releases** (right-hand column), then
   **Draft a new release**.
2. **Choose a tag**: type `knitting-v1.0.0` and choose **Create new tag on
   publish**.
3. **Target**: the default branch, after the merge (or the branch
   `claude/x-rows` if not yet merged). Its newest commit must hold the release
   (1.0.0 in the CHANGELOG).
4. **Title**: `Knitting 1.0.0`. **Publish release.**
5. The new tag starts *Watch builds* under **Actions**, in about five minutes.
   Its **knitting-store-zip** holds `Knitting-1.0.0.zip`.

## If the portal refuses the zip

X Weigh-in's 1.0.0 was refused for a manifest the portal checks more strictly
than the SDK's tools (missing activity keys, an extra `previews` key); the
packager now checks for both. If a new message comes back, send it to Claude
with the key it names. Fix the manifest, tag `knitting-v1.0.1` and upload again:
the portal matches versions by the App ID, so nothing else changes.
