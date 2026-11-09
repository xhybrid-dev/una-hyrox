# Draft note to UNA: naming an activity, and the cards it shows

Status: **draft, not sent.** Jon to check the wording before sending, and to add
point 3 only once he has confirmed it (tap the right-hand end of the heart-rate
chart of the 8 November session). Evidence: `NOTES.md`, "Gate SA3: the first real
sauna". A short forum version comes first; the full note follows.

---

## Forum post (short, non-technical)

**Title:** Letting a watch app name its activity ("Sauna", not "Training")

> Hi all. I've built X Sauna for the UNA Watch: sauna and cold-plunge rounds,
> with heart rate, recovery and the watch's temperature. It saves an ordinary
> activity, and it syncs to the UNA app nicely. Every lap, the heart rate and
> the extra data all come through.
>
> The one thing an app can't do is name its activity. Every session shows up
> as "Training" in the UNA app and "training - UNA Watch" on Strava, with
> 0.00 km, a 0'00" pace and 0 m of climb. That's because there's no sauna sport
> to pick, and the app ignores the name the file gives.
>
> Could the UNA app use the activity name a watch app writes in the file, when
> there is one? And could it hide distance, pace and elevation when an activity
> has none? That would help every indoor app: saunas, gym sessions, yoga.
>
> Thanks, Jon (HybridX)

---

## Full note

**Subject:** Feature request: activity names from watch apps, and no zero distance

Hi UNA team,

I'm Jon Lee, of HybridX, building apps for the UNA Watch with your SDK. X Sauna
records sauna and cold-plunge sessions as a normal activity: sport Training
(10), sub-sport Generic, laps for each phase, heart rate each second, and a few
developer fields. They all sync through the UNA app and show well. Thank you
for making developer fields visible: that is where the watch temperature and
heart-rate recovery appear.

**1. Use the activity name the file gives (the main request).**
FIT has no sport for a sauna or a cold plunge, so the activity shows as
"Training", and the export and the Strava upload follow it
("una-training-20261108-0937.fit", "training - UNA Watch"). The file already
names it in the standard place:
- `session.sport_profile_name` (session message 18, field 110) = "Sauna".

When that field is present and not empty, could the UNA app use it as the
activity's title, and send it to Strava as the activity name ("Sauna" or
"Sauna - UNA Watch")? Nothing changes for files without it, so the built-in
apps are unaffected. If you'd prefer another field, either of these works for
us too:
- `workout.wkt_name` (message 26, field 8; already in the SDK's
  `FitProfile.hpp`);
- an `activity_name` (or a title) in the `.json` summary the SDK examples write
  beside each `.fit`.

What we found: X Sauna has written `sport_profile_name` "Sauna" since 22 October,
and HybridX Race writes `wkt_name` "HYROX-format full race". Both were ignored.
The title and the export name follow the sport.

**2. Hide distance, pace and elevation when an activity has none.**
A sauna session (like a gym session) has no GPS and no distance anywhere in the
file: no distance field in the records, laps or session. The activity page
still shows 0.00 km, an average pace of 0'00" and 0 m of elevation. The laps
table shows 0.00 km and 0'00"/km for every lap, and "Best lap" is the shortest
lap (here a one-minute plunge). Could these be hidden when the file has no
distance? An activity type for saunas and cold plunges, with its own page, would
be even better.

**3. Charts of sessions longer than an hour** *(Jon to confirm before sending)*.
In a 97-minute session the heart-rate and watch-temperature charts seem to end
at about 60 minutes. Their maximum and average are still the whole session's
(43.9 and 36). Is the chart limited to the first hour?

**4. Lap-level developer fields** *(nice to have)*.
A lap-level developer field (our `phase`: 0 sauna, 1 plunge, 2 rest) is drawn
as a line over time, with a maximum and an average. Shown as a column in the
laps table, it would say which lap was which.

**Questions:**
1. Would you consider point 1? It needs no SDK change.
2. If you add sauna as an activity type, which FIT sport and sub-sport would
   you want apps to write?

Happy to send sample files or test a beta.

Thanks,
Jon Lee, HybridX
