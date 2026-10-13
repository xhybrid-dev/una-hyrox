# Changelog

All notable changes to X Weigh-in are recorded here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and
the project uses [semantic versioning](https://semver.org/spec/v2.0.0.html).

Releases are tagged `weighin-vX.Y.Z`. `Software/cmake/weighin-version.cmake`
strips the prefix for the SDK's version script. The steps are in
`docs/SUBMISSION.md`.

## [Unreleased]

Everything below becomes 1.0.0, the first store release, once Gate W2 (a week
of weigh-ins on the watch) has passed and the portal has issued the two App
IDs, one for the app and one for its glance.

### Added

- **Weigh in on the watch, in kg.**
  - The value starts from your last weigh-in, or from your UNA profile's weight
    before the first.
  - UP and DOWN step 0.1 kg; hold either to repeat, faster the longer you hold.
  - It is filed under the local date and time, and refused if the watch's
    clock is not set.
- **The trend.** An exponential moving average that moves 20% of the way to
  each day's weight.
  - The latest weigh-in of a day counts.
  - Missed days are filled in along a straight line.
  - The change over the last week shows once the log spans 7 days.
- **Four pages:**
  - Home (the trend, the week's change, the last weigh-in);
  - Graph (30 days: the trend line and a dot per weigh-in);
  - Recent (the last five days weighed);
  - Body (BMI and resting kcal, with height, age and sex).
- **Phone settings:** birth year, birth month and sex (`app_config.json`), for
  age and resting kcal. Height comes from the UNA profile.
- **The log** is a plain `date,time,kg` file, `Apps/XWeighIn/weighins.csv`,
  readable over USB.
  - A copy in `Apps/SharedData/HybridX/` is kept in step and restores a missing
    log.
  - Nothing is written as an activity, so nothing reaches the UNA app or Strava.
- **X Weigh-in Glance**, a separate store listing (`XWeighInGlance`) that
  needs the app:
  - the trend, the week's change and the last 14 days as bars;
  - kept between the glances list's separator lines;
  - "No weigh-ins yet" until the app has logged one.
