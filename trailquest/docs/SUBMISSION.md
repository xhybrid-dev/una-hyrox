# Trail Quest: getting it to the store

Written 21 October 2026 (phase Q5, NOTES). This follows Knitting's `knitting/docs/SUBMISSION.md`
and X Weigh-in's, which both went through the portal, and the SDK's route
(`una-sdk/Docs/deploy.md`). One listing, one package: the app. Trail Quest has no glance and
no phone settings.

Everything that can be done without Jon is done. What is left is Jon's, below.

## Where things stand

| | |
|---|---|
| App | The whole Ridge: five stages, the pack with a pick list (R1), the moving timing needle, fatigue that carries over (Fuel and huts), a harder second lap, pitched jingles. 200 host tests |
| App ID | **Issued by the portal (21 October 2026) and in place:** `AB98F6B289B6738E`, in `Software/Apps/TrailQuest-CMake/CMakeLists.txt`, `Resources/app-manifest.json` and the simulator's `CMakeLists.txt`. It replaces the development ID `B5FA567FFD85BA6D`, which the packager refuses on a release |
| Package | Built by CI on every push, in the `trailquest-store-zip` artifact: `TrailQuest-<version>.zip`. A dry run until the commit is tagged |
| Manifest | Checked by the packager: the activity keys the portal requires, no `previews` key (the lessons of X Weigh-in 1.0.0), no phone settings, `minKernelVersion` stamped from the SDK's ABI |
| Icon | `Resources/icon_60x60.png` (shipped as `icon.png`): the runner meets the Red Fox, option 2 "The duel" of five (chosen by Jon, 21 October 2026; the others are in `docs/icons/`). Drawn by `Resources/make_icons.py` |
| Previews | Six 480x480 simulator screens in `Resources/previews/`, from `docs/experiments/store_previews.sh` (title, map, duel, pick list, needle, finale) |
| Version | 1.0.0, from the one tag `trailquest-v1.0.0` |
| Firmware | Needs firmware 1.6 or newer for the pitched jingles (Jon: mandatory for everyone). The manifest's `minKernelVersion` is stamped from the SDK by the packager |

## The release day (about thirty minutes of Jon's time)

0. **Before anything: play the current build** (CI's `watch-apps`, `TrailQuest_*.uapp`) for a
   full run and a second lap: the fatigue and hut changes, the pick list, the moving needle
   and the jingles were all tuned by simulator, not on a watch. If anything feels wrong, say so
   and do not release; the numbers to adjust are in `BALANCE.md`.
1. **The app is created on the portal** and its App ID is in place (see above). Nothing to do.
2. **Merge the branch** into the default branch, or tell Claude to.
3. **Make the tag `trailquest-v1.0.0` on GitHub** (steps at the end). A tag push is
   outward-facing and Claude's session cannot push tags, so this is Jon's.
4. **Wait about five minutes** for *Watch builds* on the tag. On its run page, download
   **trailquest-store-zip** and **watch-apps**.
   - **trailquest-store-zip** holds `TrailQuest-1.0.0.zip`.
   - The `.uapp` inside it is the same file as `TrailQuest_1.0.0.uapp` in watch-apps.
   - If the run fails at the store package step, the message says why: most likely the
     binary is not 1.0.0 (a stale tag) or the App ID differs.
5. **Install that binary by USB and check it once.** These are the exact bytes going to the
   store. Copy `TrailQuest_1.0.0.uapp` into `Apps/TrailQuest/`, delete the older
   `TrailQuest_0.0.0-dev.uapp` there, eject safely, power-cycle. Check the icon in the
   launcher, a new game's opening, a duel, the pick list, a hut, and that quitting and reopening
   resumes where you were. (A development install keeps your saved game: delete the app's
   `save.txt` for a fresh start if you want one.)
6. **Upload.** On the portal, Trail Quest's **Version** tab: **Upload New**, choose
   `TrailQuest-1.0.0.zip` (the zip itself, not its contents), then **Release**, then
   **Confirm and Publish**.
7. **Install from the store** through the UNA app on the phone. Remove the USB copy first
   (the `.uapp` only). Then check it opens, plays and saves.

## Store text (draft, Jon to edit)

**Name:** Trail Quest

**Short description (portal):**
A tiny trail-running adventure: cross five stages, beat the Trail Spirits, and reach the summit.

**Longer description:**

> Cross The Ridge, one short stage at a time.
>
> - Five stages from the valley to the summit. At each fork choose your way: the
>   shorter route, or a detour to a hut and its supplies.
> - Meet the Trail Spirits: a stitch, the bonk, cramp, self-doubt, the wind. Each duel
>   is a few turns. Pick a move, then press as the needle reaches the green. The needle
>   changes with the move and the stage, and you can turn it off.
> - Beat a spirit and it may join your pack. In a duel pick which of your creatures'
>   moves to play against the spirit in front of you.
> - Fatigue carries on from fight to fight. Fuel up with a Gel, Water or a Salt tab,
>   or take the detour to a hut to rest.
> - A boss waits at the end of every stage and warns you before it charges or guards.
> - Finish the summit and run the Ridge again, harder.
> - Short jingles for hits, wins and warnings. Your game is saved after every step, so
>   it opens where you left it.
> - Your game stays on the watch: nothing is sent to the phone, the UNA app or Strava,
>   and it is not recorded as an activity.
>
> Needs watch firmware 1.6 or newer. Made by HybridX.

**Facts checked against the code:**
- Five stages with a boss each (`data/Stages.hpp`); forks, with the hut a detour at the second
  place of every stage; a cache beside each hut.
- Duels: Surge, Settle, Skill (R1's pick list) and Fuel; the needle and its shapes
  (`Beat.hpp`); the Timing option on the title turns it off.
- A beaten spirit offers to join, up to three; bosses never join.
- Fuel: Gel, Water, Salt tab (`data/Items.hpp`); trail recovery is a small breather, a hut
  fills stamina (`Run.cpp`).
- Saved after every step that ends on the map (`Run`, `SaveStore`); no FIT file; no phone
  settings.
- A second lap from the summit's finale ("Again").
- Jingles use the buzzer's pitch (SDK v1.6.0); the watch's own vibration/sound settings
  may silence them, so the text says "short jingles", not that they always play.

**Not claimed:**
- Daily Trail, rare spirits, trail events, both moves per creature (planned updates);
- that the game survives an uninstall (the app's folder goes with it);
- any link to running data (steps, heart rate, or a logged run).

## Making the tag on GitHub

Claude's session cannot push tags, so the tag `trailquest-v1.0.0` is made on GitHub, as a release:
1. Open the repository on GitHub, then **Releases** (right-hand column), then **Draft a new
   release**.
2. **Choose a tag**: type `trailquest-v1.0.0` and choose **Create new tag on publish**.
3. **Target**: the default branch, after the merge (or the session's branch if not yet
   merged). Its newest commit must hold the release: the App ID from the portal in place,
   and `CHANGELOG.md` saying 1.0.0.
4. **Title**: `Trail Quest 1.0.0`. **Publish release.**
5. The new tag starts *Watch builds* under **Actions**, in about five minutes. Its
   **trailquest-store-zip** holds `TrailQuest-1.0.0.zip`.

## If the portal refuses the zip

X Weigh-in's 1.0.0 was refused for a manifest the portal checks more strictly than the SDK's
tools (missing activity keys, an extra `previews` key); the packager checks for both. If a
new message comes back, send it to Claude with the key it names. Fix the manifest, tag
`trailquest-v1.0.1` and upload again: the portal matches versions by the App ID, so nothing
else changes.

## After release: updates

The optional extras become updates (1.1 and on): Daily Trail first (the seed code exists),
then rare spirits, trail events, both moves per creature and more on the finale. Tag each
`trailquest-vX.Y.Z` and upload the new zip on the Version tab.
