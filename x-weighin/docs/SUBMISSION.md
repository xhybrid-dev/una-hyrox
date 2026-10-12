# X Weigh-in: getting it to the store

Written 12 October 2026 (phase W5, NOTES). This follows HybridX Race's
`hybridx-race/docs/SUBMISSION.md` and the SDK's portal route
(`una-sdk/Docs/deploy.md`).

Everything that can be done without the portal is done. What is left is Jon's
week of weigh-ins (Gate W2), the App ID from the portal, and the release day
below.

## Where things stand

| | |
|---|---|
| App | W0-W3 done. 113 host tests; CI green |
| Glance | Passed on the watch (Gate W3). **Not in the store package**: it stays a USB install until UNA says how one listing installs two `.uapp`s (HybridX Streak NOTES S5.1) |
| Store zip | Built by CI on every push (`weighin-store-zip` artifact). A dry run until the real App ID is in and the commit is tagged |
| Previews, icon, manifest | Done: five 480×480 screens in `Resources/previews/`, the scale icon, the manifest with the phone settings |
| Gate W2 (a week of weigh-ins) | **Running.** Release after it passes |
| App ID | **Needs the portal.** Today's is a development ID, and the packager refuses a release with it |
| Version | 1.0.0, from the tag `weighin-v1.0.0` |

## The release day (about half an hour of Jon's time)

1. **Create the app on the portal.** Sign in at apps.unawatch.com, choose
   **Add New**, enter the name and the short description below, choose
   **Generate**, and copy the **App ID** (16 characters, 0-9 and A-F).
2. **Send Claude the App ID.** Claude puts it in the three places that name it
   (`Software/Apps/XWeighIn-CMake/CMakeLists.txt`, `Resources/app-manifest.json`
   and the simulator's `CMakeLists.txt`; the packager refuses a release where
   they differ), updates `CHANGELOG.md` ("Unreleased" becomes 1.0.0) and NOTES,
   and pushes. The glance keeps its development ID.
3. **Merge the branch** `claude/x-weighin` into `main` (PR #27), or tell Claude
   to.
4. **Claude tags the merge commit `weighin-v1.0.0`**, after asking: a tag push
   is outward-facing.
5. **Wait about five minutes** for *Watch builds* on the tag. On its run page,
   download **weighin-store-zip** (`XWeighIn-1.0.0.zip`) and **watch-apps**.
   The `XWeighIn_1.0.0.uapp` inside both is the same file.
6. **Install it by USB and weigh in once.** Copy `XWeighIn_1.0.0.uapp` into
   `Apps/XWeighIn/` (delete the old `.uapp` there; keep `weighins.csv` and
   `app_config.json`), eject, power-cycle, open the app, weigh in. That is the
   exact binary going to the store.
7. **Upload.** On the portal, the app's **Version** tab: **Upload New**, choose
   `XWeighIn-1.0.0.zip` (the zip itself, not its contents), then **Release**,
   then **Confirm and Publish**.
8. **Install from the store** through the UNA app on the phone, then Gate W5
   (NOTES): the settings screen, and the log carrying on.

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

**Not claimed:**
- that the history survives an uninstall. The SharedData copy should restore
  it, but Gate W2's optional step 5 has not been run;
- anything about the glance, which is not in the package.

## Settings the phone will show (from the manifest)

| Setting | Values | Used for |
|---|---|---|
| Birth year | 0 (unset) or the year | Age, then resting kcal |
| Birth month | 0-12 | Age to the month |
| Sex | 0 unset, 1 female, 2 male | Resting kcal |

Until the app is on the store these come from `Apps/XWeighIn/app_config.json`
over USB. The store install is the first time the UNA app's settings screen
writes that file. That makes it Gate W5's main check.
