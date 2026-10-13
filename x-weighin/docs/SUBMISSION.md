# X Weigh-in: getting it to the store

Written 12 October 2026 (phase W5, NOTES). This follows HybridX Race's
`hybridx-race/docs/SUBMISSION.md` and the SDK's portal route
(`una-sdk/Docs/deploy.md`).

**Two store listings:** the app, **X Weigh-in**, and its glance, **X Weigh-in
Glance**. They are separate because one store listing installs one `.uapp`.
The glance only shows what the app has logged, so its listing says it needs
the app.

Everything that can be done without the portal is done. What is left:
- Jon's week of weigh-ins (Gate W2);
- the two App IDs from the portal;
- the release day below.

## Where things stand

| | |
|---|---|
| App | W0-W3 done. 113 host tests; CI green |
| Glance | Passed on the watch (Gate W3). Its own listing, which needs the app |
| Store zips | Both built by CI on every push, in the `weighin-store-zip` artifact: `XWeighIn-<version>.zip` and `Glance/XWeighInGlance-<version>.zip`. Dry runs until the real App IDs are in and the commit is tagged |
| App: previews, icon, manifest | Done. Five 480×480 screens in `Resources/previews/`, the scale icon, and the manifest with the phone settings |
| Glance: previews, icon, manifest | Done. Three 480×480 previews in `Resources/Glance/previews/`, drawn from the real layout code (the simulator cannot run glances). The same icon. `Resources/Glance/app-manifest.json`, with no phone settings |
| Gate W2 (a week of weigh-ins) | **Running.** Release after it passes |
| App IDs | **Need the portal, one per listing.** Today's are development IDs (app `6E1B68E38137C089`, glance `5543E86906F4A1B3`), and the packager refuses a release with either |
| Version | 1.0.0 for both, from the one tag `weighin-v1.0.0` |

## The release day (about half an hour of Jon's time)

1. **Create both apps on the portal.** Sign in at apps.unawatch.com.
   - **Add New**: enter the app's name, "X Weigh-in", and its short
     description below. Choose **Generate** and copy the **App ID** (16
     characters, 0-9 and A-F).
   - **Add New** again: "X Weigh-in Glance" with the glance's short
     description below. **Generate** and copy its App ID.
2. **Send Claude both App IDs, saying which is which.** Claude:
   - puts the app's ID in the three places that name it:
     `Software/Apps/XWeighIn-CMake/CMakeLists.txt`,
     `Resources/app-manifest.json` and the simulator's `CMakeLists.txt`;
   - puts the glance's ID in its two places:
     `Software/Apps/XWeighInGlance-CMake/CMakeLists.txt` and
     `Resources/Glance/app-manifest.json`;
   - updates `CHANGELOG.md` ("Unreleased" becomes 1.0.0) and NOTES, then
     pushes.

   The packager refuses a release where the places that name an ID
   disagree.
3. **Merge the branch** `claude/x-weighin` into `main` (PR #27), or tell Claude
   to.
4. **Claude tags the merge commit `weighin-v1.0.0`**, after asking: a tag push
   is outward-facing.
5. **Wait about five minutes** for *Watch builds* on the tag. On its run page,
   download **weighin-store-zip** and **watch-apps**.
   - **weighin-store-zip** holds both packages: `XWeighIn-1.0.0.zip`, and
     `XWeighInGlance-1.0.0.zip` in its `Glance` folder.
   - The `.uapp`s inside them are the same files as in watch-apps.
6. **Install both by USB and check them once.** These are the exact binaries
   going to the store.
   - Copy `XWeighIn_1.0.0.uapp` into `Apps/XWeighIn/`. Delete the old `.uapp`
     there, but keep `weighins.csv` and `app_config.json`.
   - Copy `XWeighInGlance_1.0.0.uapp` into `Apps/XWeighInGlance/`, deleting
     the old one.
   - Eject, power-cycle, weigh in, then look at the glance.
7. **Upload the app first.** On the portal, X Weigh-in's **Version** tab:
   **Upload New**, choose `XWeighIn-1.0.0.zip` (the zip itself, not its
   contents), then **Release**, then **Confirm and Publish**.
8. **Then upload the glance.** The same on X Weigh-in Glance's Version tab,
   with `XWeighInGlance-1.0.0.zip`.
9. **Install both from the store** through the UNA app on the phone. Before
   that, remove the USB copies (the `.uapp` files only). Then run Gate W5
   (NOTES): the settings screen, the log carrying on, and the glance.

## Store text (draft, Jon to edit)

**Name:** X Weigh-in

**Short description (portal):**
A weight tracker on your watch. Weigh in, in kg, and see the smoothed trend
beneath the day-to-day swings.

**Longer description:**

> Weigh in on your watch in seconds, and see where your weight is really
> heading.
>
> - Enter your weight in kg, in 0.1 kg steps. It starts from your last
>   weigh-in; hold UP or DOWN to move quickly.
> - A smoothed trend rides over the day-to-day swings of water, salt and late
>   meals, with the change over the last week.
> - Weigh in again the same day and the latest counts, so a typo is fixed by
>   weighing again.
> - A 30-day graph of the trend and each weigh-in, and your last five days.
> - Your BMI, from your height in your UNA profile, and the calories you burn
>   at rest, from your birth year and sex in this app's settings.
> - Your weigh-ins stay on the watch: nothing is sent to the phone, the UNA
>   app or Strava, and they are not recorded as activities.
> - Add **X Weigh-in Glance** to see your trend on the glances screen.
>
> A tracker, not a medical device. Made by HybridX.

**Facts checked against the code:**
- 0.1 kg steps and hold to repeat: `WeighEntry.hpp`.
- The latest weigh-in of a day counts: `Trend`.
- The 20% daily trend: `kTrendPercent`.
- The weekly change shows once the log spans 7 days.
- The graph covers 30 days (`kWindowDays`) and Recent shows 5 rows.
- BMI uses the profile height: `RequestSystemSettings`.
- Resting kcal is Mifflin-St Jeor from the trend weight, height, age and sex.
- No FIT files are written: the decision of 12 October.

**Not claimed:** that the history survives an uninstall. The SharedData copy
should restore it, but Gate W2's optional step 5 has not been run.

## Store text for the glance (draft, Jon to edit)

**Name:** X Weigh-in Glance

**Short description (portal):**
Your X Weigh-in trend on the glances screen. Needs the X Weigh-in app.

**Longer description:**

> See your weight trend at a glance, without opening an app.
>
> - Your trend weight, and the change over the last week.
> - Your last 14 days as bars, one for each day you weighed in, so a few days
>   off show up at once.
>
> **Needs the X Weigh-in app**, where you weigh in. The glance shows what the
> app has logged; on its own it reads "No weigh-ins yet".
>
> A tracker, not a medical device. Made by HybridX.

**Facts checked against the code:**
- It reads the log's SharedData copy, and never writes.
- The trend and weekly change come from the same `Trend` code as the app's
  Home page.
- There are 14 bars (`kGlanceDays`), one per day ending today; a day with no
  weigh-in has none.
- With no log it shows "X Weigh-in" / "No weigh-ins yet".
- It passed on Jon's watch at Gate W3.

## Settings the phone will show (from the manifest)

| Setting | Values | Used for |
|---|---|---|
| Birth year | 0 (unset) or the year | Age, then resting kcal |
| Birth month | 0-12 | Age to the month |
| Sex | 0 unset, 1 female, 2 male | Resting kcal |

Until the app is on the store these come from `Apps/XWeighIn/app_config.json`
over USB. The store install is the first time the UNA app's settings screen
writes that file. That makes it Gate W5's main check.
