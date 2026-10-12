# X Weigh-in: notes

An on-watch weight tracker in kg. You weigh in on the watch in a few seconds,
see the smoothed trend beneath the day-to-day swings, and work towards a goal.
Started from the SDK's Waypoint tutorial (LVGL). Independent of the other apps.

## Why this app (12 October 2026)

Jon asked for a weight tracker: weigh-ins entered on the watch in kg, with the
athlete's fixed numbers (age from birth year, height) set from the phone. An
ideas pass looked at what the SDK allows and what weight data can tell an
athlete and a coach. The ideas covered:
- the trend, rate and goal;
- BMI and BMR;
- a sweat-rate test;
- a morning check-in with resting HR;
- a shared athlete file;
- syncing the data off the watch.

Jon accepted the recommendations, with the changes below.

## Decisions (Jon, 12 October 2026)

- **Name: X Weigh-in.** Folder `x-weighin/`, `XWeighIn` on the watch, tags
  `weighin-vX.Y.Z`.
- **kg only.** The watch's imperial setting is ignored.
- **Height comes from the UNA profile, not the phone settings.** So does the
  starting weight. See "Where the request and the SDK disagree".
- **Phone settings: birth year and month, and sex.** Goal fields come with the
  goal screen (W2).
- **First release:** weigh-in, smoothed trend, goal, and a glance. The
  sweat-rate test comes next.
- **The trend is a smoothed average** (an exponential moving average), not a
  plain 7-day mean.
- **The data stays on the watch.**
  - No phone app, and no HybridX app for now.
  - No activity FIT files, so nothing reaches UNA's sync or Strava. Jon does
    not want weigh-ins showing up as fake activities.
  - The log is a plain file in the app's folder, readable over USB, with a
    copy in `Apps/SharedData/` (W1) so an uninstall does not lose it.

## What the platform gives us (research pass, 12 October 2026)

Paths are relative to `una-sdk/` at the pinned commit `a7a995a`.

1. **The profile's height and weight reach every app.**
   - `RequestSystemSettings` carries `heightCm` (`uint32_t`, cm) and `weightKg`
     (`float`, kg) (`Libs/Header/SDK/Messages/CommandMessages.hpp:216-217`).
   - The SDK's Workout example uses the weight for calories
     (`Examples/Apps/Workout/Software/Libs/Sources/Service.cpp:553-559`,
     `702-709`).
   - Treadmill uses the height for stride length
     (`Examples/Apps/Treadmill/Software/Libs/Sources/Service.cpp:650-659`).
   - The profile has **no birth date, no sex and no time zone**.
2. **No app can change the profile.** The message catalogue
   (`Libs/Header/SDK/Messages/MessageTypes.hpp`) only reads settings.
   - So an out-of-date profile weight gives wrong calories in every app.
   - Telling the athlete when the trend has moved away from the profile is a
     possible later feature.
3. **Phone settings are config fields** (`Docs/app-config-fields.md`).
   - There are four types (int, float, bool, string), with at most 32 fields.
   - The app reads them when it opens.
   - §1.1 says not to ask again for anything the kernel already owns: height
     is named.
   - The UNA app shows them only once the app is in the store (HybridX Run
     NOTES, R2). Until then they are a JSON file copied over USB.
4. **The watch pairs with heart-rate straps only**
   (`Libs/Header/SDK/Messages/AccessoryMessages.hpp:25-37`). A Bluetooth scale
   could only reach the watch through the phone.
5. **Nothing wakes an app on a schedule** (HybridX Streak NOTES E.2,
   `Docs/service-lifecycle.md`). There is no morning "weigh in now" reminder.
6. **UNA's phone sync takes only activity files.**
   - It downloads `activity_*.fit` and then deletes them (HybridX Streak NOTES,
     "The UNA Android app").
   - Our own files are not activities, so the sync should leave them alone.
     HybridX Run's `vo2.json` is the same test, still to be confirmed.
7. **`Apps/SharedData/` is the one folder apps share** (HybridX Streak NOTES,
   Gate 0). Uninstalling an app deletes its own folder
   (`Docs/app-config-fields.md` §7.4).
8. **Glance limits** (HybridX Streak NOTES S4.2, S4.3):
   - the area is 240×60;
   - only text and filled rectangles draw (line controls don't);
   - fonts are printable ASCII only, and Medium 10 has digits only.
9. **LVGL in the SDK's build** (`Libs/Header/SDK/Port/LVGL/lv_conf.h:208-236`):
   - roller and line are on;
   - chart and spinbox are off.

   So the trend graph will be drawn with lines, and the weigh-in entry will be a
   roller or our own control.
10. **The simulator** (`Libs/Source/Simulator/App/KernelMessageDispatcher.cpp:119-140`):
    - Its mock kernel answers `RequestSystemSettings` but leaves height and
      weight at 0. The simulator always shows them as not set; the watch is
      where they are checked.
    - Its resting HR comes from the training heart-rate simulator, so it reads
      about 120, and only after one sensor period.
11. **The clock.** `time(nullptr)` gives UTC. Before 1 January 2026 the clock
    is taken as not set (HybridX Streak's floor), and the app shows no age.

## Where the request and the SDK disagree

**Height.** Jon asked for height in the phone settings. The SDK says not to ask
again for data the kernel owns, and names height (`Docs/app-config-fields.md`
§1.1): two copies would drift apart. Following the SDK, height comes from the
UNA profile. Jon was told and agreed (12 October 2026).

## Phases

| Phase | Work | Gate |
|---|---|---|
| **W0** | Identity, layout, CI build. The first look: the profile's height and weight, the daily resting HR, age and sex, on one screen | W0: do the profile's values reach the app on the watch? |
| W1 | The core, pure and host-tested: a weigh-in log (an append-only file in the app's folder, with a copy in SharedData), the smoothed trend, the rate per week, goal projection, BMI, BMR | Jon sets the coaching thresholds |
| W2 | The screens: weigh-in entry (starts at the last weight, 0.1 kg steps), the saved summary, trend graph, goal, history. Goal phone settings | A week of weigh-ins on the watch |
| W3 | The glance, a second `.uapp`: weight, weekly change, 14 days as bars | The glance on the watch |
| W4 | The sweat-rate test: weigh before and after a session, enter what you drank; L/h and % of body mass lost | A training session |

**Sync is deferred.** The data stays on the watch.

## W0: identity, layout and the first look (12 October 2026)

- **Verbatim copy first** (as HybridX Run did with RunLVGL): the LVGL variant
  of the SDK's Waypoint tutorial, committed unchanged.
  - Waypoint is a small Utility app with `SDK::AppConfig` already wired in.
  - RunLVGL is an activity app: GPS, FIT files and laps, none of which a
    weigh-in needs.
  - `assets/assets.json` was left out: its font paths point into the TouchGFX
    GUI, which wasn't copied. The generated font `.c` files are committed, as in
    the other apps.
- **Identity:**
  - `XWeighIn`, type `Utility`, "X Weigh-in" on the watch (10 bytes).
  - Development APP_ID `6E1B68E38137C089`, the first 16 hex of
    md5("XWeighIn"), as for Streak and Run.
  - Version from `weighin-v*` tags (`Software/cmake/weighin-version.cmake`,
    Streak's script with the prefix changed).
  - Icon (`Resources/make_icons.py`, Run's method): a white bathroom scale with
    a lime needle on a dark disc.
- **Layout:**
  - `Software/Libs/Core`: pure C++, host-tested. Anything the GUI uses is
    header-only.
  - `Software/Libs/App`: the service, the message contract, the config table.
  - `Software/Apps/LVGL-GUI`: the GUI and simulator.
  - `Software/Apps/XWeighIn-CMake`: the watch build.
- **Phone settings** (`Resources/app-manifest.json`,
  `Software/Libs/App/Sources/AppConfigFields.cpp`; CI checks they agree):
  - `birthYear` 0-2020 and `birthMonth` 0-12, the same ids and bounds as
    HybridX Run's;
  - `sex` 0-2: 0 not set, 1 female, 2 male. Config fields have no enum type.
  - Each is 0 for "not set".
- **The service** (Waypoint's lifecycle, GPS removed):
  - It reads the settings once, at start.
  - Each time the screen opens, it asks for `RequestSystemSettings`. A weight
    changed in the UNA app should then show without restarting the watch.
  - It connects `HEART_RATE_METRICS_DAILY` at a 60 s period, as HybridX Run
    does.
  - It sends one fixed-size `FirstLook` message of plain integers, with the
    weight in hundredths of a kg. It sends it again when the resting HR changes.
  - It logs the raw values.
- **The core** (`WeighProfile`, `WeighText`):
  - `ageAt`, copied from HybridX Run;
  - the clock floor;
  - `kgToHundredths`: rounds to the nearest 0.01 kg. 0 means not set, for NaN,
    zero or less, and above 500 kg;
  - `formatKg` and `sexLabel`: integer formatting only, bounds-checked.
- **The screen:** height, weight to two decimals (so the profile's resolution
  shows), resting HR, age and sex. A grey status line names the first thing
  missing ("No profile yet", "No phone settings"). R2 exits.

![First look, no settings](screens/w0-first-look-no-settings.png)
![First look, with settings](screens/w0-first-look-settings.png)
![First look, after the resting HR](screens/w0-first-look-resting-hr.png)

The simulator, left to right: with no phone settings, with settings (born
May 1986, male), and a minute later with the resting HR. The mock kernel leaves
height and weight at 0, so they show as not set.

### W0 verified (12 October 2026)

- **Host tests:** 13 pass, covering:
  - the age at birthday and year boundaries, with month 0, unset, future and
    over 120;
  - the clock floor;
  - kg rounding, including 80.7f, which truncation gets wrong, plus NaN,
    infinity and out-of-range values;
  - formatting at 0, 1 and 2 decimals, with small buffers;
  - the sex labels.
- **`validate_app_config.py --check --check-bounds`:** passes (3 fields).
- **Watch target, local compile check:**
  - Ubuntu's `arm-none-eabi-gcc` with Race's syscall stubs, so the result is
    **not installable**. The watch copy comes from CI.
  - `XWeighIn_0.0.0-dev.uapp`, 192,736 bytes. No warnings.
- **Simulator:** builds and runs (`docs/experiments/sim_run.sh`), with and
  without `app_config.json`.
  - The settings are read: "birth 5/1986, sex 2" gives "Age 40, male".
  - The resting HR arrives after one period (60 s).
  - R2 closes the app.

## Gate W0: what Jon tests on the watch

One question: **does the UNA profile's height and weight reach the app, and
how precisely?** Steps:

1. **Install.**
   - On GitHub, Actions, take the newest green **Watch builds** run on the
     branch `claude/x-weighin` and download **watch-apps**.
   - Over USB, create `Apps/XWeighIn/` on the watch and copy
     `XWeighIn_*.uapp` into it.
   - Eject safely, then power-cycle the watch (as for the other apps; see the
     README).
2. **Open X Weigh-in.** Check height and weight against your profile in the UNA
   app.
   - Note exactly what the weight shows, both decimals. For example, 82.30
     means the profile holds tenths.
   - Photograph the screen.
3. **Wait up to two minutes on the screen.** Does "Resting HR" fill in, and
   roughly how long does it take?
4. **The phone settings, over USB.** Copy a file called `app_config.json`
   into `Apps/XWeighIn/`, containing (with your own birth year and month):

   ```json
   {"schema":1,"values":{"birthYear":1986,"birthMonth":5,"sex":2}}
   ```

   Eject, open the app: does it show your age and "male"?
5. **A profile change.**
   - Change your weight in the UNA app's profile and let the watch sync.
   - Open X Weigh-in again: does it show the new weight?
   - If not, power-cycle the watch and try once more, and note which worked.

What the answers decide:
- **If height and weight both arrive:** W1 builds on them as planned.
- **If the weight arrives only in whole kg, or not at all:** the starting weight
  becomes a W2 entry on the watch instead.
- **If the height is missing:** it becomes a phone setting after all, with the
  conflict noted.

### Gate W0, first run (12 October 2026)

Jon installed the CI build (run 92) and his own `app_config.json`. The photo
is in the conversation. The watch showed:

| Row | Watch | Reading |
|---|---|---|
| Height | 182 cm | **The profile's height reaches the app** |
| Weight | 82.00 kg | **The profile's weight reaches the app** |
| Resting HR | 40 | The daily metric arrives (how soon, not yet noted) |
| Age | 40, male | **The phone settings file works on the watch** |
| Status | First look | Nothing missing |

What this settles:
- Height comes from the profile as planned. No phone setting is needed, so the
  height conflict stays resolved the SDK's way.
- W1 can take the starting weight from the profile.
- The layout fits the real display, clear of the BACK (R2) marker.

Still open from the gate:
- **Resolution.** 82.00 cannot tell whole kg from tenths.
- **A profile change (step 5).** Does a new weight show without a power-cycle?
  Both are answered at once by entering a weight with a decimal (82.4, say) in
  the UNA app, syncing, and reopening.
- **The resting HR.** Does 40 match the UNA app's figure for today, and how
  long did it take to appear?

## Open, for later phases

- **Coaching thresholds (W1):**
  - what weekly rate of loss or gain, as % of body mass per week, the app
    should flag;
  - whether it should flag at all.

  Jon's call as the coach. Nothing is invented.
- **The trend's smoothing (W1):** the Hacker's Diet method (J. Walker) moves
  the trend 10% of the way to each new weigh-in. To confirm with Jon.
- **BMR (W1):** the Mifflin-St Jeor equation (Mifflin et al., 1990) needs
  weight, height, age and sex. The phone settings now supply the last two.
- **Goal settings (W2):** config fields have no date type. A goal date needs
  its own encoding: year, month and day fields, or weeks from the start.
- **The `sex` field's encoding** is fixed from here on. Changing a field's type
  later discards the user's value (`Docs/app-config-fields.md` §7.3).
