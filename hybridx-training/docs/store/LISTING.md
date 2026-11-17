# X Training 1.1.0: the store listing, paste-ready

For Jon, to copy into the portal. The source of this text, and the checks behind each claim, is
`hybridx-link/docs/STORE_RELEASE.md` ("X Training"). Written 17 November 2026, **before** Jon's watch test of the plan's
memory and the treadmill: read it against what the watch does.

## The upload (what goes where)

| Portal | What | From |
|---|---|---|
| The app's zip (Version tab, Upload New) | `HybridXTraining-1.1.0.zip` | CI's `training-store-zip`, from the tag `training-v1.1.0` |
| Icon and screenshots | Inside that zip: `icon.png` and `previews/` (nine 480 x 480 screens) | built by the packager from `Resources/` |
| Listing images (if the portal asks for them) | The banner first, then the squares | this folder: `01-banner.png` (1920 x 1080), `02` to `07` (1080 x 1080) and `08-the-x-family.png` |
| Release notes ("what's new") | the text below | |
| Name, short and long description | the text below | (the name and short description were set when the listing was made) |

## Name

X Training

## Short description

Training plans on your wrist: a 10K, half marathon or VO2max block from your 5K time, outdoors or on a treadmill. Counts in X Streak.

## Longer description

Your plan, on your wrist.

- **Three plans:** a 10K (10 weeks), a half marathon (12) and a VO2max block (8), each at three levels, on the days you run. Give it a recent 5K time in the UNA app, and a race date if you have one.
- **Today's session is ready:** its target pace, distance and time, and what it builds. Press R1 and it runs step by step: a pace gauge, a buzz when you drift out of range, lead-in beeps, and each short rep judged on the next step's card.
- **The plan learns from what you do:** a session you miss is noted. A missed key session takes a later quality session's place, a week you mostly missed is repeated, and a missed long run is offered back the next day. **Changes** moves tomorrow's session to today (or today's to tomorrow), skips a session, pauses the plan, or ends it.
- **Time trials and races update your paces:** after a 5K time trial or a race the watch works out the 5K it means. Faster times are used from your next run; slower ones, or a big jump, ask first.
- **On a treadmill too:** with the Treadmill switch on, today's session runs as UNA's treadmill mode does: no GPS, speed and distance from your cadence, and the console's distance asked for after you save. Where a step has a set speed the watch assumes the belt is at it, so a 400 m rep ends after the time that takes; targets show as belt speeds (km/h or mph).
- **Paces from your 5K:** easy, threshold, interval and repetition, with predicted 10K and half times. Worked out with Daniels' running tables, and checked against them.
- **This week and the whole plan:** seven days from Monday with their marks, the weeks by phase, and your progress.
- Records every run as a normal run: GPS (or the treadmill), heart rate, laps (one per step), cadence and step length, and the run detail pages. Syncs to the UNA app and Strava.
- Counts in **X Streak**, with its training load. Shows X Run's VO2max on the plan screen, and feeds X Run's VO2max with your runs.

Made by HybridX. Training advice is general, not medical: stop if something hurts.

## What's new in 1.1.0 (release notes)

- **The plan learns from what you do.** A session you miss is noted; a missed key session takes a later quality session's place, a week you mostly missed is repeated, and a missed long run is offered the next day. A new **Changes** item on the main menu moves tomorrow's session to today (or today's to tomorrow), skips a session, pauses the plan, or ends it.
- **Time trials and races update your paces.** After a 5K time trial or a race the watch works out the 5K it means: faster times update your paces from your next run; slower ones, or a big jump, ask you first.
- **Treadmill sessions.** With the Treadmill switch on, today's session runs on the treadmill as UNA's treadmill mode does: no GPS, speed and distance from your cadence, and the console's distance asked for after you save. Where a step has a set speed the watch assumes the belt is at it, so a rep ends after the time its distance takes; targets show as belt speeds (km/h or mph).

## The nine screenshots (in the zip, in this order)

1. `1-today` Today's session: the target, the distance, what it builds
2. `2-gauge` The pace gauge, with the time left in the step
3. `3-rep` A short rep's result on the next step's card
4. `4-week` This week, seven rows from Monday
5. `5-paces` The paces from your 5K
6. `6-changes` Changes: move, skip, pause or end the plan
7. `7-missed` A missed long run, offered back
8. `8-treadmill` Today on a treadmill: the target as a belt speed
9. `9-treadmill-rep` A 400 m rep at 15.1 km/h on a treadmill

All are real simulator captures (`docs/experiments/store_previews.sh`).

## Not to claim

That the cadence-derived treadmill speed matches your belt (it is an estimate until you enter the console's distance); that a
treadmill time trial or race changes your paces (it does not); anything about the glance, which is a separate listing that
does not exist yet.
