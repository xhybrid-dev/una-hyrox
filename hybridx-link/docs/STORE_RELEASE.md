# The X family on the UNA store: the release

Written 3 November 2026 (phase P). It follows X Weigh-in's release (`x-weighin/docs/SUBMISSION.md`,
the portal's rules it taught) and the SDK's portal route (`una-sdk/Docs/deploy.md`).

**What goes up:** seven new listings and one update.
- The apps: **X Streak, X Run, X Trail, X Workout and X Ride** (1.0.0).
- Two glances, each on its own listing because one listing installs one `.uapp`:
  **X Streak Glance** and **VO2max Glance** (1.0.0).
- An update: **HybridX Race 1.1.0**. It is on the store at 1.0.0, from before the family.

**9 November 2026:** Jon created the seven listings, and their App IDs are in every place that
names them (release-day step 2, done). Each CHANGELOG now has 1.0.0 (Race 1.1.0), dated
9 November. CI still builds each package as a dry run on every push: the binaries say
`0.0.0-dev` until their tags. **What is left needs Jon:** the tags (step 3), then the check and
the uploads (steps 4 to 7).

**Checked on 9 November**, before the tags: all eight binaries were built locally as their tags
will build them (`-DBUILD_VERSION=1.0.0`, Race `1.1.0`), and each packager was run with
`--expect-version`. Each passed:
- the portal form;
- both SDK validators;
- the App ID matching in every place (`app_merging.py` logs it in each binary);
- the release checks.

Those local zips are compile checks (Ubuntu's compiler and stubs), never for upload. **Only
the zips CI builds from the tags go to the portal.**

## Where things stand

| Listing | Package (CI artifact) | Tag | App ID |
|---|---|---|---|
| X Streak | `HybridXStreak-<v>.zip` (`streak-store-zip`) | `streak-v1.0.0` | the portal's `BF63291BD26E20AC` |
| X Streak Glance | `Glance/HXStreakGlance-<v>.zip` (`streak-store-zip`) | the same tag | the portal's `BC8BFA0808314DB6` |
| X Run | `HybridXRun-<v>.zip` (`run-store-zip`) | `run-v1.0.0` | the portal's `D0A3ADE4AF694DB2` |
| VO2max Glance | `Glance/HybridXRunGlance-<v>.zip` (`run-store-zip`) | the same tag | the portal's `A38E5C0975D2394A` |
| X Trail | `HybridXTrail-<v>.zip` (`trail-store-zip`) | `trail-v1.0.0` | the portal's `1C83FDDE4EA3C880` |
| X Workout | `HybridXWorkout-<v>.zip` (`workout-store-zip`) | `workout-v1.0.0` | the portal's `B9171EBAF7066A30` |
| X Ride | `HybridXRide-<v>.zip` (`ride-store-zip`) | `ride-v1.0.0` | the portal's `58F141AE44428F51` |
| HybridX Race | `HybridXRace-<v>.zip` (`race-store-zip`) | `v1.1.0` and `apps-v1.1.0` | the portal's `5C4D6F49335E5E17` |

Each zip holds four things: the `.uapp`, `app-manifest.json`, `icon.png` and `previews/`.
They are made by the app's own `Utilities/pack-store-zip.sh` (`--glance` for a glance). That
script refuses to write a zip if:
- the manifest is not in the portal's form: the nine activity keys must be there, and there
  must be no `previews` key;
- either SDK validator fails;
- the APP_ID differs between the CMake file, the manifest and the simulator;
- on a tag, the binary's version is not the tag's, or the ID is still the development one.

The previews (480 × 480) come from the simulator. Each app's
`docs/experiments/store_previews.sh` runs its own walkthrough and keeps the screens. A
glance's previews come from `glance_preview.py --store`, because the simulator cannot run
glances.

## The release day (about an hour of Jon's time)

1. **Create the seven listings on the portal.**
   - Sign in at apps.unawatch.com.
   - For each listing below, in this order: X Streak, X Streak Glance, X Run, VO2max Glance,
     X Trail, X Workout, X Ride.
     - Choose **Add New**.
     - Enter the **Name** and the **Short description** from its section below.
     - Choose **Generate**, then copy the **App ID** (16 characters, 0-9 and A-F).
   - Race needs nothing here: its listing exists.
2. **Send Claude the seven App IDs, saying which is which.** Claude then:
   - puts each ID in its places (the app's `CMakeLists.txt`, its `Resources/app-manifest.json`
     and its simulator's `CMakeLists.txt`; a glance's in its CMake file and
     `Resources/Glance/app-manifest.json`);
   - turns each CHANGELOG's "Unreleased" into the version, and pushes.

   CI then builds every package again: still dry runs, but now with the real IDs.
3. **Make the tags on GitHub.** Claude's session cannot push tags. For each tag:
   1. Open the repository, then **Releases** (right-hand column), then **Draft a new release**.
   2. **Choose a tag:** type the tag and choose **Create new tag on publish**.
   3. **Target:** the branch Claude names, at its newest commit.
   4. **Title:** e.g. "X Ride 1.0.0".
   5. Choose **Publish release**.

   The tags: `streak-v1.0.0`, `run-v1.0.0`, `trail-v1.0.0`, `workout-v1.0.0`, `ride-v1.0.0`,
   and for Race both `v1.1.0` and `apps-v1.1.0` (on the same commit).
4. **Wait about ten minutes** for *Watch builds* on each tag (the **Actions** tab). On each
   tag's run page, download its `*-store-zip` and, once, **watch-apps**.
5. **Check the exact binaries once, by USB.**
   - Copy each `.uapp` from the zips into its folder on the watch (`Apps/HybridXStreak/` and so
     on, as in `FAMILY_WATCH_TEST.md` section 2). Delete the old `.uapp` there; keep the data
     files.
   - Power-cycle the watch.
   - Do the short family test: one session in Ride or Workout, then open Streak (its "+1" and
     the Training load screen), and look at both glances.
6. **Upload the apps first, then the glances,** which need their apps. For each listing:
   1. Open its **Version** tab and choose **Upload New**.
   2. Choose the zip itself, not its contents.
   3. Choose **Release**, then **Confirm and Publish**.

   Race's update goes on its existing listing's Version tab.
7. **Install from the store** through the UNA app on the phone.
   - Before that, remove the USB copies (the `.uapp` files only).
   - Check that the phone shows Streak's and Run's settings screens.
   - Then repeat step 5's short test.

**If the portal refuses a zip:** send Claude its message, word for word. Weigh-in's first
refusal named every key it wanted. The fix goes in, a patch version is tagged (for example
`ride-v1.0.1`), and that zip is uploaded instead.

---

## Store text (drafts, Jon to edit)

House style, as Weigh-in's:
- each listing names the family once ("Counts in X Streak");
- a glance says it needs its app;
- nothing is claimed that the code does not do.

The short description is what the portal asks for at **Add New**. The longer description goes
on the listing.

### X Streak

**Short description:** A weekly training streak for the X family of apps, with your fitness,
fatigue and form from their heart rate.

**Longer description:**

> Set a weekly target and keep the streak going.
>
> - Sessions saved by **X Run, X Trail, X Ride, X Workout and HybridX Race** count by themselves:
>   open Streak and they are there. Anything else you did can be logged by hand, today or
>   yesterday.
> - Every week you hit your target is a step up the mountain: five summits, from Arthur's Seat at
>   4 weeks to Everest at two years, and badges at 10, 50, 100 and 250 sessions.
> - Miss a week? Shields, earned every four good weeks, save the streak.
> - **Training load:** each session's heart-rate score becomes your fitness, fatigue and form,
>   with a word for where you are (Fresh, Steady, Building or Overreaching) after two weeks.
> - Choose your target, the day your week starts, what counts, the shortest session and one
>   per day, in the UNA app.
> - Add **X Streak Glance** to see your week on the glances screen.
>
> Made by HybridX.

**Facts checked against the code:**
- **What it reads:** the family's summaries, `LinkReader` and the `kSlotPaths` slots. Manual logs
  are for today or yesterday (Log a session).
- **Summits** are Arthur's Seat (4 weeks), Snowdon (12), Ben Nevis (26), Mont Blanc (52) and
  Everest (104), then a repeat ascent every 52 weeks. The session badges are at 10, 50, 100
  and 250. Both are in `Core/Header/Summits.hpp`.
- **Shields:** one per four achieved weeks, at most two (CHANGELOG).
- **Training load** is `LoadHistory.hpp`: 42 and 7 days. The word shows after 14 days of
  history (`kWarmUpDays`); the bands are provisional.
- **The phone settings** are the five `configFields`, checked against `AppConfigFields` by the
  packager.

**Not claimed:**
- that UNA's own apps count: Streak cannot read their folders on a watch;
- reminders: nothing runs on a schedule on this platform.

### X Streak Glance

**Short description:** Your X Streak week on the glances screen. Needs the X Streak app.

**Longer description:**

> See your streak without opening an app.
>
> - Your streak in weeks, this week's sessions against your target, and what is next: the
>   next summit, "Banked" once the week is done, or how many more you need and the days left.
> - When the week is on track and Streak has two weeks of history, your form in a word.
>
> **Needs the X Streak app**, which counts your sessions. The glance shows what the app has
> saved, projected to today.
>
> Made by HybridX.

**Facts checked against the code:** `GlanceLayout` and its host tests, including the widths. The
form line shows when on track and after 14 days (`GlanceLayoutTest`). The previews come from the
layout code (`glance_preview.py --store`).

### X Run

**Short description:** Record your runs and get a VO2max estimate worked out on the watch.
Counts in X Streak.

**Longer description:**

> A running app with a VO2max estimate of its own.
>
> - Distance, pace, heart rate, laps and climb with GPS, saved as a run that syncs to the UNA
>   app and Strava.
> - **VO2max**, estimated on the watch from your steady running: pace and gradient against your
>   heart-rate reserve. Your maximum heart rate is entered, or estimated from your age.
>   The figure, its trend, and the runs behind it.
> - **Run detail:** cadence and step length, time in each heart-rate zone, best 1 km, mile and
>   5 km, and how evenly you paced it.
> - **Treadmill mode:** speed and distance from your cadence; enter the console's distance
>   after the run.
> - Counts in **X Streak**, with its training load. The Saved screen shows your week.
> - Add the **VO2max Glance** to see your figure on the glances screen.
>
> VO2max here is an estimate for tracking your own trend, not a lab test. Made by HybridX.

**Facts checked against the code:**
- the method is in Run NOTES (the ACSM running equation, %HRR ≈ %VO2R, Tanaka);
- the treadmill uses the SDK's `CadenceStrideModel`;
- the detail faces are checked by `fit_detail_report.py` and `fit_form_report.py`;
- the phone settings are the five `configFields`.

**Not claimed:** accuracy against a lab or another watch. That is what the app was built to
find out (Run NOTES).

### VO2max Glance

**Short description:** Your X Run VO2max on the glances screen. Needs the X Run app.

**Longer description:**

> - Your VO2max as X Run shows it, the change at your latest run, and your last 10 runs as bars.
>
> **Needs the X Run app**, which works the figure out; the glance shows what the app has saved.
>
> Made by HybridX.

**Facts checked against the code:** `Libs/Glance`, which reads `vo2max.json`. The previews come
from the layout code.

### X Trail

**Short description:** Follow a GPX route on your wrist, with a buzz when you go off course.
Counts in X Streak.

**Longer description:**

> Run a route without looking at your phone.
>
> - Load a GPX route and see it on a map with you on the line: heading-up or north-up, zoomed
>   from 300 m to 3 km with one button.
> - Off by 25 m or more? A buzz, and an arrow back to the route. "Back on course" when you are.
> - Turn cues before each turn, distance to go, and an elevation profile for routes with heights.
> - It records the run too, with distance, pace, heart rate and laps, and it syncs to the UNA app
>   and Strava.
> - Counts in **X Streak**, with its training load. Your steady minutes also count towards
>   X Run's VO2max.
>
> **Routes:** copy GPX files to the app's Routes folder with the watch connected by USB.
>
> Made by HybridX.

**Facts checked against the code:** Trail NOTES T3, T3b and T3c (zoom 300 m, 750 m, 1.5 km and
3 km; off course from 25 m; turn cues; the way back; elevation), and "Run link".

**Not claimed:** routes sent from the phone. The Route Sender is a test and is not part of
this release.

### X Workout

**Short description:** Gym, HIIT and Hyrox-style sessions with timers, sets and heart-rate
zones. Counts in X Streak.

**Longer description:**

> One app for the sessions that are not a run or a ride.
>
> - **Seven kinds:** Free (laps), Strength (sets with a rest timer), EMOM, AMRAP, Tabata,
>   Intervals, and Hyrox-style Circuits of runs and stations.
> - Each step is timed for you: a countdown, three ticks before it ends, and a different buzz
>   for work, rest and done.
> - Heart-rate zones and calories, each step saved as a lap, and strength sets as sets, synced to
>   the UNA app and Strava.
> - Counts in **X Streak** as Strength, Hybrid or Workout, with its training load.
>
> Made by HybridX.

**Facts checked against the code:**
- the kinds are `SessionType` (7);
- the cues are `PlanRunner`, and the buzzes `buzzCue`;
- FIT `set` messages for strength (Workout NOTES W2);
- the link sports in `writeActivityLink`.

**Not claimed:** rep counting (Jon, 2 November 2026: no).

### X Ride

**Short description:** Rides with GPS, auto-pause and laps outdoors, and intervals on the
turbo. Counts in X Streak.

**Longer description:**

> Outdoors or on the turbo.
>
> - **Outdoors:** speed, distance and climb with GPS and the barometer, with heart rate.
>   **Auto-pause** stops the clock at the lights and starts it as you ride off. Auto laps go by
>   time or by distance (1 to 25 km).
> - **Indoors:** time and heart rate, free or as **Intervals** (your work, rest and rounds)
>   or **Tabata**, with a countdown and buzzes.
> - Heart rate from the wrist, or from a strap paired in the watch's settings.
> - Saved as a ride (or an indoor ride) that syncs to the UNA app and Strava.
> - Counts in **X Streak**, with its training load.
>
> Bike sensors (power, cadence, speed) are not yet supported by the watch; Ride will use them
> when they are. Made by HybridX.

**Facts checked against the code:**
- **Auto-pause** is `AutoPause.hpp`, from the SDK Cycling example.
- **Distance laps** are `kAutoLapKm`.
- **The plans** are `RidePlan.hpp`.
- **The sub-sports** are Generic and IndoorCycling (`finish`).
- **Bike sensors:** the SDK connects heart-rate straps only (Gate B0).

**Not claimed:** power, cadence or wheel speed.

### HybridX Race 1.1.0 (the update)

**What's new** (the listing's release notes, if the portal asks):

> HybridX Race joins the X family. Each saved race now counts in **X Streak**. The Saved
> screen shows your week and your streak, and the race's training load ("Load 39"), which
> X Streak turns into fitness, fatigue and form.

Race's description stays as it is on the store (`hybridx-race/docs/SUBMISSION.md`). One line
can be added to it: "Counts in X Streak."
