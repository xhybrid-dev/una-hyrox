# The HybridX family on a watch: the test script

The first time the family runs together on a real watch. Every app below has been checked in the
simulator and by host tests (each app's NOTES; the family check, `ACTIVITY_LINK.md`), and every
watch build compiles in CI, but none of the link has run on a watch. This is that test.

**What it shows:** each app saves its activity, leaves its summary for Streak, and shows the
Streak line and the session's training load ("Load 12"); Streak counts each session once, as
the right kind, and adds its load to the Training load screen; a discarded session counts for
nothing; Ride's auto-pause and turbo intervals work; and all of it survives a phone sync.

**The names (2 November 2026).** The watch's app list now shows **X Run, X Trail, X Ride,
X Workout and X Streak** (and X Sauna, X Weigh-in); **HybridX Race** keeps its name. Only the
shown name changed: the folders on the watch are as before, so nothing already saved moves.

**Time:** about two hours, best done in one day (so the week does not roll over midway), with
a short run and a short ride outdoors. Every session can be short: step 3 below lets a
two-minute session count.

**In the family:** X Streak (it counts), and X Run, HybridX Race, X Trail, X Workout and X Ride
(they write).
X Sauna is installed too, but on its own: it stays out of the streak (Jon, 27 October 2026),
so step 13 checks that it does **not** count.

Do the steps in order and stop at the first surprise. Write down what each screen says (a photo
is best).

---

## 1. Before you start

1. **The watch's firmware.** In the UNA app (or the watch's settings), note the firmware version.
   It must be **1.4.0 or newer**; 1.6.2 is what everything was last built for. On anything
   older, stop and tell me.
2. **A heart-rate strap** (optional). If you want to use one, switch **External HR** on in the
   watch's settings and pair the strap there first. The apps say "Strap connected" on their
   Start row when it is working.
3. **Streak's settings, on the phone.** In the UNA app, open X Streak's settings (after
   step 2 has installed it) and set **Shortest session** to **0**, so short test sessions count.
   Leave **Weekly target** as it is (3 by default); note what it is. Put Shortest session back
   to 10 after the test.
4. **Have to hand:** the watch's USB cable and a computer; a phone with the UNA app; somewhere
   to run 100 m and ride for a few minutes.

## 2. Install the apps

All the apps come from one place: the **watch-apps** artifact of a green CI run.

1. On GitHub, open **xhybrid-dev/una-hyrox**, then the **Actions** tab.
2. Choose the newest **Watch builds** run with a green tick on the branch
   `claude/hybridx-race-workspace-setup-kppnmz`.
3. At the bottom of the run's page, under **Artifacts**, download **watch-apps** and unzip it.
   It holds one `.uapp` per app; the number in each name is its version.
4. Connect the watch by USB and wait for its drive to appear (it can take a minute: running
   apps save their data first).
5. For each row below, create the folder on the watch's drive (if it is not there) and copy the
   file into it. Folder names must be exactly as shown.

   | App (as the watch lists it) | File in watch-apps | Folder on the watch |
   |---|---|---|
   | X Streak | `HybridXStreak_*.uapp` | `Apps/HybridXStreak/` |
   | Streak's glance (optional) | `HXStreakGlance_*.uapp` | `Apps/HXStreakGlance/` |
   | X Run | `HybridXRun_*.uapp` | `Apps/HybridXRun/` |
   | HybridX Race | `HybridXRace_*.uapp` | `Apps/HybridXRace/` (see the note) |
   | X Trail | `HybridXTrail_*.uapp` | `Apps/HybridXTrail/` |
   | X Workout | `HybridXWorkout_*.uapp` | `Apps/HybridXWorkout/` |
   | X Ride | `HybridXRide_*.uapp` | `Apps/HybridXRide/` |
   | X Sauna | `XSauna_*.uapp` | `Apps/XSauna/` |

   **Race:** the store's Race (1.0.0) predates the link, so it neither writes a summary nor
   shows the Streak line. If Race came from the store, look over USB for the folder it is in
   and tell me its name before copying anything over it; skip step 10 until we know.
6. **Trail needs a route.** Create `Apps/HybridXTrail/Routes/` and copy a GPX into it: one of
   yours, or one from `hybridx-trail/Tools/TestRoutes/` (any short loop near you will do; the
   test is the save, not the navigation).
7. **A clean shared folder.** Look in `Apps/SharedData/HybridX/`. If there are `act_*.json`
   files from earlier tests, copy them to the computer and then delete them from the watch, so
   this test starts from nothing. Leave any other files there.
8. Eject the drive **safely**, unplug, then **power-cycle** the watch (off and on). Check every
   app is in the list.

## 3. The sessions, in order

For each: do it, save it, read the **Saved** screen's amber lines and its **Load** (a number
when the watch had your heart rate; none without it), then open **Streak** and read its toast ("+1 Run · 3 min": the kind, then the session's minutes). The counts below assume
a target of 3 and that nothing else counted this week; if Streak already had sessions this week,
add them to every number. "N of 3 this week" goes on past 3 ("4 of 3"). A second line gives the
streak once there is one: from step 4 if you had none ("1-week streak"), from step 2 if you had
one. On the session that meets the target, Streak's own screen also says the week is banked.

| Step | Session | The Saved screen says | Streak's toast |
|---|---|---|---|
| 1 | Open Streak once, close it | (none) | its week as it was |
| 2 | **Run** outdoors, 3 minutes | "1 of 3 this week" | "+1 Run · 3 min" |
| 3 | **Workout, Strength** (below) | "2 of 3 this week" | "+1 Strength · ..." |
| 4 | **Workout, EMOM** (below) | "3 of 3 this week", "1-week streak" | "+1 Workout · 5 min" |
| 5 | **Workout, Circuit** (below) | "4 of 3 this week", ... | "+1 Hybrid · ..." |
| 6 | **Ride**, outdoors (or indoors), 3 minutes | "5 of 3 this week", ... | "+1 Ride · 3 min" |
| 7 | **Trail**, 3 minutes on your route | "6 of 3 this week", ... | "+1 Run · 3 min" |
| 8 | **Race**, a half with 100 m runs | "7 of 3 this week", ... | "+1 Hybrid · ..." |

How to run each (one at a time):

**Step 2, Run.** Open X Run outdoors, wait for the GPS, Start, run 3 minutes, then R1
(the menu), L2 twice to **Save**, and hold R1.

**Step 3, Workout, Strength.** This is the one to feel the buzzes on.
1. Open X Workout. L2 to **Type**, R1 until it says **Strength**.
2. L2 to **Settings**, R1. On **Sets**, press R1 until it shows 2 (it steps up through the
   choices and round to 1). L2 to **Rest**, R1 until it shows 30 s. R2 back to the menu: the
   Settings row now says "2 sets, 30 s rest".
3. L1 up to **Start**, R1. Do a set (anything: 10 squats). Press **R2** when the set is done:
   the screen says "Rest" in blue and counts down 30 s.
4. Let the rest run out. In its last 3 seconds the number turns amber and the watch ticks three
   times (3, 2, 1); then a different buzz, and "Set 2".
5. Do set 2, then **R2**. The plan is over: a long buzz and "Done".
6. **R1** (the menu opens on End workout), and **hold R1** until it saves.
7. Look at the summary (L2 turns the pages): titled "Strength", "2 sets", and laps named
   "Set 1", "Rest 1", "Set 2".

**Step 4, Workout, EMOM.** Type: **EMOM**; in Settings, R1 on **Minutes** until it shows 5 min.
Start. The button does
nothing here (its hint is gone): the minutes run themselves, with the 3-2-1 ticks before each.
After 5 minutes it says "Done": R1, hold R1.

**Step 5, Workout, Circuit.** Type: **Circuit**; in Settings, **Rounds** to 1 and **Run** to
100 m (one R1 each takes them round from 8 and 1 km). Start:
walk or jog 100 m, then **R2**: "SkiErg" in amber with "1000 m". Do a minute of anything, then
**R2**: "Done". R1, hold R1. The summary says "1 station".

**Step 6, Ride, outdoors with a stop.** Open X Ride. Wait for "GPS ready" on the Start row.
The main menu's last row says **Auto-pause On** (leave it on). Start, ride a minute, then
**stop** (a junction, or just stand with the bike) for 20 seconds: within a few seconds the
watch buzzes **once** and the screen says **"Auto-paused"** in amber, with the speed grey.
Ride off: after a few metres it buzzes **twice** and the timer runs again. Ride another
minute, then R1, L2 L2 to End, hold R1.

**Step 7, Trail.** Open X Trail, choose your route, Start, 3 minutes, then save as for Run.

**Step 8, Race.** Open HybridX Race and choose a half (either) and a run length of 100 m on its
main menu and in its settings. Start race (R1, and R1 again on "On your marks"), press R2 for
each split (eight in a half; any pace: the split lock is a few seconds), then R1 on the finish
screen to save.

## 4. The checks after the sessions

9. **Counted once.** Close Streak and open it again: no toast, and the week's count is unchanged.
10. **A discarded session counts for nothing.** Open Workout (any type), Start, then R1 (the
    menu), L2 three times to **Discard**, and hold R1. Open Streak: no toast, no change.
11. **The files.** Connect USB and copy `Apps/SharedData/HybridX/` to the computer (all of it).
    Also copy each app's `Activity/` folder (they hold the `.fit` files; a phone sync deletes
    them). Workout's `act_workout.json` should hold three entries; each other app's one.
12. **A phone sync.** Let the UNA app sync. Then:
    - in the UNA app, look at each activity: its sport and title (Workout's strength session
      should be strength training, the EMOM a HIIT one, the circuit cardio training), its laps,
      and for strength whether the sets show;
    - if Strava is linked, the same there;
    - open Streak again: the week's count must not change (the summaries stay on the watch when
      the `.fit` files go).
13. **X Sauna stays out.** Open X Sauna, start a session (on a table, indoors), wait a minute,
    swap phase once, then end and save it. Open Streak: **no** toast, no change. (Do not wear
    the watch in a sauna or water for this: that is a separate test, and only if UNA's
    published limits allow it.)
14. **The glance** (if installed): it shows the week's count from step 8. (Its form line,
    "Form +5 fresh", only appears after two weeks of history, when you are on track.)

## 5. Ride on the turbo, and the training load

These sessions count too (each adds one to the week); that is fine.

15. **Ride, Tabata on the turbo** (or anywhere still: the watch only needs your heart rate).
    Open X Ride. L2 to the **Ride** row and R1 to **Indoor**. The rows below change: L2 to
    **Type**, R1 until it says **Tabata**; the next row says **Settings, 8 x 20 s / 10 s**.
    L1 up to **Start**, R1. The screen shows **"Work 1/8"** in lime and counts 20 seconds down;
    in the last 3 seconds the number turns amber and the watch ticks three times; then a
    double buzz and **"Rest 1/8"** in blue, 10 seconds. After 8 rounds (4 minutes): a long
    buzz and **"Done"**. R1 opens the menu on **End ride**: hold R1. The Saved screen shows
    the Streak line and the load; the summary is titled **Tabata**, "8 of 8 rounds", with
    laps "Work 1", "Rest 1", ... in their colours. R2 does nothing during the plan (no lap
    button: the steps end by themselves).
16. **Intervals** (optional): as 15, with Type **Intervals**; its **Settings** opens Work,
    Rest and Rounds (R1 steps each; default 40 s / 20 s x 6).
17. **A distance lap** (optional, outdoors): on the main menu, R1 on **Auto lap** steps
    past the minutes to **1 km**, 5 km and so on. Ride over 1 km: a buzz and "Auto lap".
18. **Training load in Streak.** Open X Streak, L2 to **Training load** (before Settings):
    its hint says "Learning: day 1 of 14". R1 opens it: **Form**, then **Fitness**,
    **Fatigue**, **This week** and **Today**. Today's load should be the sum of the "Load"
    numbers on today's Saved screens. The word under Form stays "Learning" for the first two
    weeks; after that it says Fresh, Steady, Building or Overreaching (the bands are a first
    guess, to tune from your weeks).

## 6. What to send me

- The photos (or what each Saved screen and Streak toast said), against the table in section 3.
- The files from step 11, before the sync if you can.
- For step 3: did the buzzes feel different from each other (the work buzz, the rest buzz, the
  three ticks, the end)? Were the colours easy to read (lime work, blue rest, amber in the last
  3 seconds)?
- For step 12: what the UNA app and Strava called each Workout session, and what they showed
  for its laps and sets; and for Ride, whether they show the Tabata's rest laps as rest, and
  the auto-pause as a pause.
- For step 6: how quickly it paused and resumed, and whether it ever paused while you were
  moving slowly (on a climb, say).
- For step 18: the Saved screens' loads and Streak's Today; whether the numbers feel sensible
  for the effort (Edwards' score: 30 minutes mostly in zones 3 and 4 is about 90 to 120).
- Anything that was awkward on the wrist.

## What each result decides

- **Everything as in the tables:** the family's link works on a watch; next are the releases
  (App IDs from the portal for Workout, Ride and X Sauna, then store packages).
- **A Saved screen without the amber lines:** that app could not read Streak's summary. Send
  the files from step 11.
- **Streak shows no toast after a save:** it did not find the summary. Send the files.
- **A session counted twice, or a discarded one counted:** a bug; send the files and the order
  of what you did.
- **The UNA app or Strava shows a Workout session as something odd (or rejects it):** the new
  sport numbers (Workout NOTES "W2") go back to generic for that kind.
