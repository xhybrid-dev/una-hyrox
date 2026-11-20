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
   | X Training (section 7) | `HybridXTraining_*.uapp` | `Apps/HybridXTraining/` (keep its `app_config.json` and `plan.json`) |
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

(For X Training, section 7 says what to send.)

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

## 7. X Training joins the family (X Training's TP4, 15 November 2026)

X Training's runs count in Streak (a sixth file, `act_training.json`) and feed X Run's VO2max
(`training_vo2_0..2.json`), and its Plan screen shows X Run's figure. It needs the new Streak and
the new X Run as well: an older Streak never counts X Training's runs, and an older X Run never
reads them. Install all three from the same Watch builds run (section 2).

19. **Before:** open X Run once and note its VO2max (the start screen's "VO2max 45.2"; none is
    fine). Open X Training, L2 to **Plan**, R1: under the bar, **"VO2max 45.2 in X Run"** (your
    figure). No line if X Run has none yet. In the VO2max plan, the line later reads "VO2max 45.9,
    was 45.2" (the figure when the plan began).
20. **A session:** today's session from **Today** (or a **Free run**), at least 20 minutes of
    steady running outdoors, then Save. The Saved screen has the plan's lines, then the amber
    Streak line, with the session counted ("N of 3 this week").
21. **Streak:** open it. The toast is "+1 Run · N min", and **This week** lists it as
    **"X Training"**. Close it and open it again: no second toast.
22. **X Run:** open it (outdoors is not needed; close it once it has loaded). It reads X Training's
    minutes now, once. If the run had enough steady minutes, its VO2max may move. Open X Run a
    second time: nothing changes.
23. **X Training's Plan screen** again: the figure is X Run's new one.
24. **The files:** copy `Apps/SharedData/HybridX/` over USB. `act_training.json` holds the session,
    and `training_vo2_0.json` holds its minutes (its `"id"` is the run's start). Also copy
    `Apps/HybridXRun/vo2.json`: it is version 5, and its `"training"` mark is that same start.

Send: the photos of steps 19 to 23, and the files from step 24.

## 8. X Training's glance (X Training's TP6, 16 November 2026)

The glance (`HXTrainingGlance_<version>.uapp`, from the same Watch builds run as the app) shows today's
session on the glances screen. It reads `training_today.json`, which the app writes, so the app has to be a
build from TP6 on and must have been opened once with a plan.

25. **Install and add it:** copy `HXTrainingGlance_<version>.uapp` into `Apps/HXTrainingGlance/` on the
    watch (as step 2 of section 2), power-cycle, and add **X Training** to the glances screen. Open the
    app once (Today), then look at the glances screen.
26. **A session day:** line one reads **"Today: <the session>"** in white, line two its "what" and pace
    ("4 x 4 min @ 4:18/km"; an easy run just its time). Under them, seven small blocks for Monday to Sunday
    (today white, to come grey, rest dark, done green) and a bar to the right of them. Photograph it.
27. **Done:** run and save today's session, then look at the glance again (no need to open the app): it
    reads **"Done: <the session>"** in green, the next session on line two ("Tomorrow: Easy" or "Wed: Easy 45
    min"), and today's block is green.
28. **Other days, if they come up:** a rest day ("Rest day", then "Tomorrow: ..."); a session you skipped
    (amber); the plan paused from **Changes** (**"Plan paused"**, "Resume in the app"); and, after a week or two
    without opening the app, it still shows the right day. After 14 days it says **"Open the app"**.
    Also check that the text sits inside the face (not clipped at the sides, not touching the blocks) and
    that nothing is cut off at the 60 px height.

Send: a photo of steps 26 and 27, a photo of any other state, and `Apps/SharedData/HybridX/training_today.json`
(over USB) if the glance showed something wrong.

## 9. A planned session on a treadmill (X Training, 16 November 2026)

With the main menu's **Treadmill** switch on, today's planned session runs as UNA's treadmill mode does:
no GPS, speed and distance from your cadence, the console's distance asked for after Save. Where a step has
a designated speed, the watch **assumes the belt is at it** (and says so as the target, in km/h or mph), so
set the belt to the speed shown for each rep. You need the CI build of the last push of X Training.

29. **The switch:** in X Training's main menu, L2 twice from Today to **Treadmill**, R1 to turn it on (the
    switch shows on). Open **Today**: the target now reads as a belt speed ("14.0 km/h" for intervals,
    "easy 9.8-11.0 km/h" for an easy run), where it read a pace.
30. **Start it:** R1 on Today, R1 to start. It should go straight to the countdown **without waiting for a
    GPS fix**, and the GPS icon shows absent. The step cards show the belt speed. Set the treadmill to it for
    each rep, and to a comfortable easy speed for the easy parts.
31. **A rep by distance or by time:** a rep that says "400 m" ends by itself after the time that distance
    takes at the belt speed shown (400 m at 14.0 km/h: 103 s); a timed rep ends on its time. A step that
    ends on a lap press (a warm-up before a time trial, the walk down after a hill) still does. Note whether
    the lap changes at about the right moment; if the belt was not really at that speed, say so.
32. **No pace buzzes:** you will not hear "too fast" or "too slow" on a treadmill (the speed is assumed, or
    a cadence estimate too rough to buzz on). The heart-rate buzzes still work if easy by heart rate is on.
33. **Save:** hold R1 and Save as usual. The Saved screen says Done (or Part done) and the next session; then
    the console's distance: enter what the treadmill shows (L2 up by 0.10, R1 for hundredths, R1 to save; R2
    keeps the watch's own total). The summary's map page says "No map on a treadmill".
34. **Afterwards:** Today says **Done today**, the glance (if installed) says Done, and X Streak counts it as
    a Run (the toast "+1 Run" the next time you open it). The treadmill file is in `Apps/HybridXTraining/Activity/`
    (copy it over USB if anything looked wrong).
35. **An easy run:** do the same with an easy day. It is timed; the distance is the watch's cadence estimate
    until you enter the console's, and the entries teach the stride model (it learns from entered runs of 2 km or
    more, after outdoor runs have taught the shared stride table).

Send: photos of steps 29, 30 and 33 (the Saved screen and the distance screen), what you set the belt to and
how the lap changes lined up (step 31), and the console's distance against the watch's own.

## 10. The plan's memory (X Training's TP5, in 1.1.0)

The watch now notices what you do and do not do. These need a plan under way and a few days: do the first
two steps whenever you like, the rest as they come up.

36. **Skip and pause:** in X Training's main menu choose **Changes** (it sits after Plan). **Skip today**: Today's
    title says CHANGED and the day reads Skipped; This week shows it. **Pause plan**: Today says "Plan paused";
    **Resume plan** (the same item) starts it again. Nothing is marked missed while it is paused.
37. **Moving a session:** on a day with a session, **Tomorrow's** (do it today) or **Today's** (do it tomorrow). If
    that would put two hard days together the watch makes the nearer one easy instead, and says so; a move it
    will not make reads NOT ALLOWED.
38. **A day you miss:** leave a planned session undone and open the app the next day. This week shows it in
    orange. If it was a long run and today is easy or a rest day, Today offers it ("Long run missed, Do it today?");
    a missed key session is offered when it would otherwise be lost. Say yes or no: it is not offered twice.
39. **A weak week:** if less than half of a week was done, the next week is the same one again (or, near a race, it
    takes the next week's place). A week with nothing done makes the next one an easier week.
40. **A time trial or race:** run the plan's 5K time trial (outdoors). The Saved screen says what the plan did:
    "5K 21:41, new paces" (faster: used from your next run), "paces kept", or "ask in Today" (slower by more than 2%,
    or a jump of more than about 6%: Today asks Update or Keep mine).

Send: photos of Today and This week after steps 36 to 38, and what the Saved screen said after step 40.

## 11. How it went, and how you felt (X Training's TP8, in 1.2.0)

New in 1.2.0, not yet on a watch. It needs a planned session that counts (half its planned time, or more),
outdoors with a GPS fix: an easy run is enough for steps 41 to 43. Steps 44 and 45 need three sessions, so they
come up over the days.

41. **After a planned session that counts:** Saved shows the plan's lines as before, with one more: "On pace 82%",
    or "Too fast, 40% on pace" / "Too slow, 40% on pace". An easy run is judged only on "too fast" (a slower jog
    is not a miss), so an easy run kept easy reads "On pace 100%". Then the **Feel** screen: Fresh, Good, Steady,
    Tired, Spent, or Skip (L1/L2 move, R1 chooses, R2 skips). Choose one.
42. **The summary:** L2 through its faces: after Pacing comes **How it went** (only after a judged session): the
    share of the time in the target band in big figures, "5 of 6 reps in band", "Misses 6 s/km fast", and, after
    four or more reps, "Later reps 7 s/km slower". Check that the figures match how the run went: an interval
    session run too fast should say fast.
43. **Form:** Plan (main menu), then **L2**. The last eight reviewed sessions as bars (green on pace, yellow too
    fast, cyan too slow, grey not judged), a dot under each for how you felt (teal fresh or good, white steady,
    orange tired or spent), and a line on the pattern once there are three off pace.
44. **Three sessions too slow in a row** (a hill or heat can do it: the watch judges by GPS speed): Today asks
    "Hold the plan?" with how you felt ("and you were tired" or "though you felt fresh"). **Yes** makes next week
    run this week's plan again; **Not now** changes nothing and does not ask again until three more sessions. Near
    a race, or when the plan has grown as far as it can, it says why it cannot (Today's title NOT ALLOWED, and the
    reason under it). Three sessions **too fast** are only mentioned on Form: the plan is never changed for them.
45. **A treadmill session** is not judged (no How it went face, no review line), but still asks how you felt.

Send: photos of Saved, the Feel screen, How it went and Form after steps 41 to 43; and, if you see it, Today's
question and what it said about how you felt (step 44). Say whether the percentages seem fair for the run you did.

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
